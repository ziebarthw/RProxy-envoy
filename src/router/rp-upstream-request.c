/*
 * rp-upstream-request.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_upstream_request_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_upstream_request_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "router/rp-default-upstream-http-filter-chain-factory.h"
#include "router/rp-router-filter.h"
#include "router/rp-router-filter-interface.h"
#include "router/rp-upstream-filter-manager.h"
#include "router/rp-upstream-request-fmc.h"
#include "stream_info/rp-filter-state-impl.h"
#include "stream_info/rp-stream-info-impl.h"
#include "stream_info/rp-upstream-info-impl.h"
#include "rp-filter-manager.h"
#include "rp-http-filter.h"
#include "rp-http-utility.h"
#include "rp-upstream-request.h"

#define STREAM_INFO(s) RP_STREAM_INFO(RP_UPSTREAM_REQUEST(s)->m_stream_info)
#define STREAM_DECODER_FILTER_CALLBACKS(s) \
    rp_router_filter_interface_callbacks(RP_UPSTREAM_REQUEST(s)->m_parent)
#define CALLBACKS(s) RP_STREAM_FILTER_CALLBACKS(STREAM_DECODER_FILTER_CALLBACKS(s))
#define FILTER_MANAGER_CALLBACKS(s) RP_FILTER_MANAGER_CALLBACKS(RP_UPSTREAM_REQUEST(s)->m_filter_manager_callbacks)
#define FILTER_MANAGER(s) RP_FILTER_MANAGER(RP_UPSTREAM_REQUEST(s)->m_filter_manager)
#define UPSTREAM_INFO(s) rp_stream_info_upstream_info(STREAM_INFO(s))
#define UPSTREAM_TIMING(s) rp_upstream_info_upstream_timing(UPSTREAM_INFO(s))

struct _RpUpstreamRequest {
    GObject parent_instance;

    RpRouterFilterInterface* m_parent;
    RpGenericConnPool* m_conn_pool;

    UNIQUE_PTR(RpGenericUpstream) m_upstream;
    RpHostDescription* m_upstream_host;
    RpStreamInfoImpl* m_stream_info;
    evhtp_headers_t* m_upstrea_headers;
    evhtp_headers_t* m_upstream_trailers;
    RpUpstreamToDownstream* m_upstream_interface;
    GSList/*<UpstreamCallbacks>*/* m_upstream_callbacks;

    RpUpstreamRequestFmc* m_filter_manager_callbacks;
    RpFilterManager* m_filter_manager;

    guint64 m_response_headers_size;
    gsize m_response_headers_count;
    gsize m_deferred_read_disabling_count;

    RpStreamResetReason_e m_deferred_reset_reason;

    bool m_can_send_early_data : 1;
    bool m_can_use_http3 : 1;
    bool m_enable_half_close : 1;
    bool m_reset_stream : 1;
    bool m_paused_for_connect : 1;
    bool m_paused_for_websocket : 1;
    bool m_router_sent_end_stream : 1;
    bool m_encode_trailers : 1;
    bool m_awaiting_headers : 1;
    bool m_had_upstream : 1;
    bool m_cleaned_up : 1;

};

enum
{
    PROP_0, // Reserved.
    PROP_PARENT,
    PROP_CONN_POOL,
    PROP_CAN_SEND_EARLY_DATA,
    PROP_CAN_USE_HTTP3,
    PROP_ENABLE_HALF_CLOSE,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void stream_callbacks_iface_init(RpStreamCallbacksInterface* iface);
static void upstream_to_downstream_iface_init(RpUpstreamToDownstreamInterface* iface);
static void stream_decoder_iface_init(RpStreamDecoderInterface* iface);
static void response_decoder_iface_init(RpResponseDecoderInterface* iface);
static void generic_connection_pool_callbacks_iface_init(RpGenericConnectionPoolCallbacksInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpUpstreamRequest, rp_upstream_request, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_CALLBACKS, stream_callbacks_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_DECODER, stream_decoder_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_RESPONSE_DECODER, response_decoder_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_UPSTREAM_TO_DOWNSTREAM, upstream_to_downstream_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_GENERIC_CONNECTION_POOL_CALLBACKS, generic_connection_pool_callbacks_iface_init)
)

static void
on_reset_stream_i(RpStreamCallbacks* self, RpStreamResetReason_e reason, const char* transport_failure_reason)
{
    NOISY_MSG_("(%p, %d, %p(%s))", self, reason, transport_failure_reason, transport_failure_reason);
    RpUpstreamRequest* me = RP_UPSTREAM_REQUEST(self);
    rp_upstream_request_clear_request_encoder(me);
    me->m_awaiting_headers = false;

    rp_stream_info_set_response_flag(RP_STREAM_INFO(me->m_stream_info),
        rp_router_filter_stream_reset_reason_to_response_flag(reason));
    rp_router_filter_interface_on_upstream_reset(me->m_parent, reason, transport_failure_reason, me);
}

static void
stream_callbacks_iface_init(RpStreamCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_reset_stream = on_reset_stream_i;
}

static void
decode_data_i(RpStreamDecoder* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    //TODO...resetPerTryIdleTimeout()
    RpUpstreamRequest* me = RP_UPSTREAM_REQUEST(self);
    //TODO...stream_info_.addBytesReceived(data.length())
    rp_router_filter_interface_on_upstream_data(me->m_parent, data, me, end_stream);
}

static void
stream_decoder_iface_init(RpStreamDecoderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->decode_data = decode_data_i;
}

static void
maybe_handle_deferred_read_disable(RpUpstreamRequest* self)
{
    NOISY_MSG_("(%p)", self);
    for (; self->m_deferred_read_disabling_count > 0; --self->m_deferred_read_disabling_count)
    {
        //TODO...parent_.cluster()->trafficStats()...
        rp_stream_read_disable(RP_STREAM(self->m_upstream), true);
    }
}

static void
decode_1xx_headers_i(RpResponseDecoder* self, evhtp_headers_t* response_headers)
{
    NOISY_MSG_("(%p, %p)", self, response_headers);
    RpUpstreamRequest* me = RP_UPSTREAM_REQUEST(self);
    maybe_handle_deferred_read_disable(me);
    rp_router_filter_interface_on_upstream_1xx_headers(me->m_parent, response_headers, me);
}

static void
decode_headers_i(RpResponseDecoder* self, evhtp_headers_t* response_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, response_headers, end_stream);
    //TODO...resetPerTryIdleTimer();
    RpUpstreamRequest* me = RP_UPSTREAM_REQUEST(self);
    evhtp_res response_code = http_utility_get_response_status(response_headers);
    //TODO...is1xx(response_code)...

    me->m_awaiting_headers = false;
    rp_stream_info_set_response_code(RP_STREAM_INFO(me->m_stream_info), response_code);

    maybe_handle_deferred_read_disable(me);

    rp_router_filter_interface_on_upstream_headers(me->m_parent, response_code, response_headers, me, end_stream);
}

static void
decode_trailers_i(RpResponseDecoder* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    RpUpstreamRequest* me = RP_UPSTREAM_REQUEST(self);
    rp_router_filter_interface_on_upstream_trailers(me->m_parent, trailers, me);
}

static void
response_decoder_iface_init(RpResponseDecoderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->decode_1xx_headers = decode_1xx_headers_i;
    iface->decode_headers = decode_headers_i;
    iface->decode_trailers = decode_trailers_i;
}

static void
clear_request_encoder(RpUpstreamRequest* me)
{
    NOISY_MSG_("(%p)", me);
    if (me->m_upstream)
    {
        //TODO...
    }
    g_clear_object(&me->m_upstream);
}

static RpNetworkConnection*
connection_i(RpUpstreamToDownstream* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_stream_filter_callbacks_connection(CALLBACKS(self));
}

static RpRoute*
route_i(RpUpstreamToDownstream* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_stream_filter_callbacks_route(CALLBACKS(self));
}

static RpHttpConnPoolInstStreamOptions
upstream_stream_options_i(RpUpstreamToDownstream* self)
{
    NOISY_MSG_("(%p)", self);
    RpUpstreamRequest* me = RP_UPSTREAM_REQUEST(self);
    return rp_http_conn_pool_inst_stream_options_ctor(me->m_can_send_early_data, me->m_can_use_http3);
}

static void
upstream_to_downstream_iface_init(RpUpstreamToDownstreamInterface* iface)
{
    LOGD("(%p)", iface);
    iface->connection = connection_i;
    iface->route = route_i;
    iface->upstream_stream_options = upstream_stream_options_i;
}

static inline RpStreamResetReason_e
get_reset_reason(RpPoolFailureReason_e reason)
{
    switch (reason)
    {
        case RpPoolFailureReason_Overflow:
            return RpStreamResetReason_Overflow;
        case RpPoolFailureReason_RemoteConnectionFailure:
            return RpStreamResetReason_RemoteConnectionFailure;
        case RpPoolFailureReason_LocalConnectionFailure:
            return RpStreamResetReason_LocalConnectionFailure;
        case RpPoolFailureReason_Timeout:
            return RpStreamResetReason_ConnectionTimeout;
        default:
            return RpStreamResetReason_ProtocolError;
    }
}

static void
on_pool_failure_i(RpGenericConnectionPoolCallbacks* self, RpPoolFailureReason_e reason, const char* transport_failure_reason, RpHostDescription* host)
{
    NOISY_MSG_("(%p, %d, %p(%s), %p)", self, reason, transport_failure_reason, transport_failure_reason, host);
    //TODO...recordConnectionPoolCallbackLatency(...)
#if 0
    RpStreamResetReason_e reset_reason = get_reset_reason(reason);
#endif//0

    RpUpstreamRequest* me = RP_UPSTREAM_REQUEST(self);
    RpUpstreamInfo* upstream_info = rp_stream_info_upstream_info(RP_STREAM_INFO(me->m_stream_info));
    rp_upstream_info_set_upstream_transport_failure_reason(upstream_info, transport_failure_reason);

    // Mimic an upstream reset.
    rp_upstream_request_on_upstream_host_selected(me, host, false);
    rp_stream_callbacks_on_reset_stream(RP_STREAM_CALLBACKS(self), reason, transport_failure_reason);
}

static void
on_pool_ready_i(RpGenericConnectionPoolCallbacks* self, UNIQUE_PTR(RpGenericUpstream) upstream, RpHostDescription* host,
                RpConnectionInfoProvider* address_provider, RpStreamInfo* info, evhtp_proto protocol)
{
    NOISY_MSG_("(%p, %p, %p, %p, %p, %d)", self, upstream, host, address_provider, info, protocol);
    RpUpstreamRequest* me = RP_UPSTREAM_REQUEST(self);
    me->m_upstream = g_steal_pointer(&upstream);
    me->m_had_upstream = true;

    if (protocol != EVHTP_PROTO_INVALID)
    {
        rp_stream_info_set_protocol(RP_STREAM_INFO(me->m_stream_info), protocol);
    }
    else
    {
        NOISY_MSG_("unpausing...");
        me->m_paused_for_connect = false;
        me->m_paused_for_websocket = false;
    }

    RpUpstreamInfo* upstream_info = rp_stream_info_upstream_info(STREAM_INFO(self));
    if (rp_stream_info_upstream_info(info))
    {
        RpUpstreamTiming* upstream_timing = rp_upstream_info_upstream_timing(rp_stream_info_upstream_info(info));
        UPSTREAM_TIMING(self)->m_upstream_connect_start = upstream_timing->m_upstream_connect_start;
        UPSTREAM_TIMING(self)->m_upstream_connect_complete = upstream_timing->m_upstream_connect_complete;
        UPSTREAM_TIMING(self)->m_upstream_handshake_complete = upstream_timing->m_upstream_handshake_complete;
        rp_upstream_info_set_upstream_num_streams(upstream_info,
            rp_upstream_info_upstream_num_streams(rp_stream_info_upstream_info(info)));
    }

    RpFilterState* filter_state = rp_stream_info_filter_state(info);
    if (!filter_state)
    {
        rp_upstream_info_set_upstream_filter_state(upstream_info,
            RP_FILTER_STATE(rp_filter_state_impl_new(NULL, RpFilterStateLifeSpan_Request)));
    }
    else
    {
        rp_upstream_info_set_upstream_filter_state(upstream_info, filter_state);
    }
    rp_upstream_info_set_upstream_local_address(upstream_info,
        rp_connection_info_provider_local_address(address_provider));
    rp_upstream_info_set_upstream_remote_address(upstream_info,
        rp_connection_info_provider_remote_address(address_provider));
    rp_upstream_info_set_upstream_ssl_connection(upstream_info,
        rp_connection_info_provider_ssl_connection(address_provider));

    rp_upstream_request_on_upstream_host_selected(me, host, true);

    //TODO...

    if (protocol != EVHTP_PROTO_INVALID)
    {
        rp_upstream_info_set_upstream_protocol(upstream_info, protocol);
    }

    //TODO...

#if 0
    RpRoute* route_entry = rp_upstream_to_downstream_route(RP_UPSTREAM_TO_DOWNSTREAM(me));
#endif//0
    //TODO...if (route_entry->autoHostRewrite())...

    rp_stream_info_set_request_headers(RP_STREAM_INFO(me->m_stream_info),
        rp_router_filter_interface_downstream_headers(me->m_parent));

    //TODO...

    for (GSList* entry = me->m_upstream_callbacks; entry; entry = entry->next)
    {
        RpUpstreamCallbacks* callbacks = RP_UPSTREAM_CALLBACKS(entry->data);
        rp_upstream_callbacks_on_upstream_connection_established(callbacks);
    }
}

static RpUpstreamToDownstream*
upstream_to_downstream_i(RpGenericConnectionPoolCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_UPSTREAM_REQUEST(self)->m_upstream_interface;
}

static void
generic_connection_pool_callbacks_iface_init(RpGenericConnectionPoolCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_pool_failure = on_pool_failure_i;
    iface->on_pool_ready = on_pool_ready_i;
    iface->upstream_to_downstream = upstream_to_downstream_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_PARENT:
            g_value_set_object(value, RP_UPSTREAM_REQUEST(obj)->m_parent);
            break;
        case PROP_CONN_POOL:
            g_value_set_object(value, RP_UPSTREAM_REQUEST(obj)->m_conn_pool);
            break;
        case PROP_CAN_SEND_EARLY_DATA:
            g_value_set_boolean(value, RP_UPSTREAM_REQUEST(obj)->m_can_send_early_data);
            break;
        case PROP_CAN_USE_HTTP3:
            g_value_set_boolean(value, RP_UPSTREAM_REQUEST(obj)->m_can_use_http3);
            break;
        case PROP_ENABLE_HALF_CLOSE:
            g_value_set_boolean(value, RP_UPSTREAM_REQUEST(obj)->m_enable_half_close);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_PARENT:
            RP_UPSTREAM_REQUEST(obj)->m_parent = g_value_get_object(value);
            break;
        case PROP_CONN_POOL:
            RP_UPSTREAM_REQUEST(obj)->m_conn_pool = g_value_get_object(value);
            break;
        case PROP_CAN_SEND_EARLY_DATA:
            RP_UPSTREAM_REQUEST(obj)->m_can_send_early_data = g_value_get_boolean(value);
            break;
        case PROP_CAN_USE_HTTP3:
            RP_UPSTREAM_REQUEST(obj)->m_can_use_http3 = g_value_get_boolean(value);
            break;
        case PROP_ENABLE_HALF_CLOSE:
            RP_UPSTREAM_REQUEST(obj)->m_enable_half_close = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_upstream_request_parent_class)->constructed(obj);

    RpUpstreamRequest* self = RP_UPSTREAM_REQUEST(obj);
    RpHostDescription* upstream_host = rp_generic_conn_pool_host(RP_GENERIC_CONN_POOL(self->m_conn_pool));
    RpStreamInfo* downstream_stream_info = rp_stream_filter_callbacks_stream_info(CALLBACKS(self));
    self->m_stream_info = rp_stream_info_impl_new(EVHTP_PROTO_INVALID, /*NULL*/rp_stream_info_downstream_address_provider(downstream_stream_info), RpFilterStateLifeSpan_FilterChain, NULL);

    rp_stream_info_set_upstream_info(STREAM_INFO(self),
        RP_UPSTREAM_INFO(rp_upstream_info_impl_new()));
    rp_stream_info_impl_set_route_(self->m_stream_info,
        rp_stream_filter_callbacks_route(CALLBACKS(self)));
    rp_upstream_info_set_upstream_host(
        rp_stream_info_upstream_info(STREAM_INFO(self)), upstream_host);
    rp_stream_info_set_upstream_info(
        rp_stream_filter_callbacks_stream_info(CALLBACKS(self)),
            rp_stream_info_upstream_info(STREAM_INFO(self)));

    RpClusterInfo* cluster_info = rp_stream_info_upstream_cluster_info(
        rp_stream_filter_callbacks_stream_info(CALLBACKS(self)));
    if (cluster_info)
    {
        rp_stream_info_set_upstream_cluster_info(STREAM_INFO(self), cluster_info);
    }

    self->m_filter_manager_callbacks = rp_upstream_request_fmc_new(self);
    self->m_filter_manager = RP_FILTER_MANAGER(
        rp_upstream_filter_manager_new(FILTER_MANAGER_CALLBACKS(self),
                                    rp_stream_filter_callbacks_dispatcher(CALLBACKS(self)),
                                    rp_upstream_to_downstream_connection(RP_UPSTREAM_TO_DOWNSTREAM(self)),
                                    rp_stream_filter_callbacks_stream_id(CALLBACKS(self)),
                                    true,
                                    rp_stream_decoder_filter_callbacks_decoder_buffer_limit(STREAM_DECODER_FILTER_CALLBACKS(self)),
                                    self));

    struct RpCreateChainResult res = rp_filter_manager_create_filter_chain(FILTER_MANAGER(self),
                                        RP_FILTER_CHAIN_FACTORY(rp_router_filter_interface_cluster(self->m_parent)));
    if (!res.m_created)
    {
        res = rp_filter_manager_create_filter_chain(FILTER_MANAGER(self),
                RP_FILTER_CHAIN_FACTORY(rp_router_filter_interface_config(self->m_parent)));
    }

    if (!res.m_created)
    {
        NOISY_MSG_("creating default upstream http filter chain");
        res = rp_filter_manager_create_filter_chain(FILTER_MANAGER(self),
                RP_FILTER_CHAIN_FACTORY(rp_default_upstream_http_filter_chain_factory_new()));
    }

NOISY_MSG_("res.m_created %u", res.m_created);
}

#if 0
static void
clean_up(RpUpstreamRequest* self)
{
    NOISY_MSG_("(%p)", self);

    if (self->m_cleaned_up)
    {
        LOGD("already cleaned up");
        return;
    }
    self->m_cleaned_up = true;

    rp_filter_manager_destroy_filters(RP_FILTER_MANAGER(self->m_filter_manager));

    //TODO...if(span_ != nullptr)...

    //TODO...timers

    clear_request_encoder(self);

    //TODO...stats

    rp_stream_info_on_request_complete(RP_STREAM_INFO(self->m_stream_info));
    //TODO...upstreamLog(...)

    //TODO..while (downstream_data_disabled_ != 0) {...}

    rp_dispatcher_deferred_delete(
        rp_stream_filter_callbacks_dispatcher(
            RP_STREAM_FILTER_CALLBACKS(
                rp_router_filter_interface_callbacks(self->m_parent))), G_OBJECT(g_steal_pointer(&self->m_filter_manager_callbacks)));
}
#endif//0

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpUpstreamRequest* self = RP_UPSTREAM_REQUEST(obj);
    rp_filter_manager_destroy_filters(RP_FILTER_MANAGER(self->m_filter_manager));
    g_clear_object(&self->m_filter_manager);
    g_clear_object(&self->m_upstream);

    G_OBJECT_CLASS(rp_upstream_request_parent_class)->dispose(obj);
}

void
rp_upstream_request_reset_stream(RpUpstreamRequest* self)
{
    LOGD("(%p)", self);
    RpUpstreamRequest* me = RP_UPSTREAM_REQUEST(self);
    if (rp_generic_conn_pool_cancel_any_pending_stream(me->m_conn_pool))
    {
        LOGD("cancelled pool request");
    }

    if (rp_upstream_timing_last_upstream_tx_byte_sent(UPSTREAM_TIMING(self)) &&
        rp_upstream_timing_last_upstream_rx_byte_received(UPSTREAM_TIMING(self)))
    {
        NOISY_MSG_("returning");
        return;
    }

    if (me->m_upstream)
    {
        LOGD("resetting pool request");
        rp_generic_upstream_reset_stream(me->m_upstream);
        clear_request_encoder(me);
    }
    me->m_reset_stream = true;
}

void
rp_upstream_request_accept_headers_from_router(RpUpstreamRequest* self, bool end_stream)
{
    LOGD("(%p, %u)", self, end_stream);

    RpUpstreamRequest* me = RP_UPSTREAM_REQUEST(self);
    me->m_router_sent_end_stream = end_stream;

    rp_generic_conn_pool_new_stream(me->m_conn_pool, RP_GENERIC_CONNECTION_POOL_CALLBACKS(self));

    rp_filter_manager_request_headers_initialized(me->m_filter_manager);
    RpStreamInfo* stream_info = rp_filter_manager_stream_info(me->m_filter_manager);
    evhtp_headers_t* headers = rp_router_filter_interface_downstream_headers(me->m_parent);
    const char* method_value = evhtp_header_find(headers, RpHeaderValues.Method);
    if (g_ascii_strcasecmp(method_value, RpHeaderValues.MethodValues.Connect) == 0)
    {
        NOISY_MSG_("CONNECT");
        me->m_paused_for_connect = true;
    }
    rp_stream_info_set_request_headers(stream_info, headers);
    rp_filter_manager_decode_headers(me->m_filter_manager, headers, end_stream);
}

void
rp_upstream_request_accept_data_from_router(RpUpstreamRequest* self, evbuf_t* data, bool end_stream)
{
    LOGD("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);

    RpUpstreamRequest* me = RP_UPSTREAM_REQUEST(self);
    me->m_router_sent_end_stream = end_stream;

    rp_filter_manager_decode_data(me->m_filter_manager, data, end_stream);
}

static void
rp_upstream_request_class_init(RpUpstreamRequestClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    obj_properties[PROP_PARENT] = g_param_spec_object("parent",
                                                    "Parent",
                                                    "Parent RouterFilterInterface Instance",
                                                    RP_TYPE_ROUTER_FILTER_INTERFACE,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_CONN_POOL] = g_param_spec_object("conn-pool",
                                                    "Conn pool",
                                                    "GenericConnPool Instance",
                                                    RP_TYPE_GENERIC_CONN_POOL,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_CAN_SEND_EARLY_DATA] = g_param_spec_boolean("can-send-early-data",
                                                    "Can send early data",
                                                    "Can Send Early Data Flag",
                                                    false,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_CAN_USE_HTTP3] = g_param_spec_boolean("can-use-http3",
                                                    "Can use http3",
                                                    "Can Use Http3 Flag",
                                                    false,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_ENABLE_HALF_CLOSE] = g_param_spec_boolean("enable-half-close",
                                                    "Enable half close",
                                                    "Enable Half Close Flag",
                                                    false,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_upstream_request_init(RpUpstreamRequest* self)
{
    NOISY_MSG_("(%p)", self);

    RpUpstreamRequest* me = RP_UPSTREAM_REQUEST(self);
    me->m_router_sent_end_stream = false;
    me->m_encode_trailers = false;
    me->m_awaiting_headers = true;
    me->m_paused_for_connect = false;
    me->m_paused_for_websocket = false;
    me->m_reset_stream = false;
    me->m_cleaned_up = false;
    me->m_had_upstream = false;
    me->m_deferred_read_disabling_count = 0;
}

RpUpstreamRequest*
rp_upstream_request_new(RpRouterFilterInterface* parent, RpGenericConnPool* conn_pool, bool can_send_early_data, bool can_use_http3, bool enable_half_close)
{
    LOGD("(%p, %p, %u, %u, %u)", parent, conn_pool, can_send_early_data, can_use_http3, enable_half_close);
    g_return_val_if_fail(RP_IS_ROUTER_FILTER_INTERFACE(parent), NULL);
    g_return_val_if_fail(RP_IS_GENERIC_CONN_POOL(conn_pool), NULL);
    return g_object_new(RP_TYPE_UPSTREAM_REQUEST,
                        "parent", parent,
                        "conn-pool", conn_pool,
                        "can-send-early-data", can_send_early_data,
                        "can-use-http3", can_use_http3,
                        "enable-half-close", enable_half_close,
                        NULL);
}

RpRouterFilterInterface*
rp_upstream_request_parent_(RpUpstreamRequest* self)
{
    LOGD("(%p)", self);
    return RP_UPSTREAM_REQUEST(self)->m_parent;
}

GSList**
rp_upstream_request_upstream_callbacks_(RpUpstreamRequest* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_UPSTREAM_REQUEST(self), NULL);
    return &RP_UPSTREAM_REQUEST(self)->m_upstream_callbacks;
}

bool
rp_upstream_request_paused_for_connect_(RpUpstreamRequest* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_UPSTREAM_REQUEST(self), false);
    return RP_UPSTREAM_REQUEST(self)->m_paused_for_connect;
}

void
rp_upstream_request_set_paused_for_connect_(RpUpstreamRequest* self, bool value)
{
    LOGD("(%p, %u)", self, value);
    g_return_if_fail(RP_IS_UPSTREAM_REQUEST(self));
    RP_UPSTREAM_REQUEST(self)->m_paused_for_connect = value;
}

bool
rp_upstream_request_paused_for_websocket_upgrade_(RpUpstreamRequest* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_UPSTREAM_REQUEST(self), false);
    return RP_UPSTREAM_REQUEST(self)->m_paused_for_websocket;
}

void
rp_upstream_request_set_paused_for_websocket_upgrade_(RpUpstreamRequest* self, bool value)
{
    LOGD("(%p, %u)", self, value);
    g_return_if_fail(RP_IS_UPSTREAM_REQUEST(self));
    RP_UPSTREAM_REQUEST(self)->m_paused_for_websocket = value;
}

bool
rp_upstream_request_enable_half_close_(RpUpstreamRequest* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_UPSTREAM_REQUEST(self), false);
    return RP_UPSTREAM_REQUEST(self)->m_enable_half_close;
}

RpGenericUpstream*
rp_upstream_request_upstream_(RpUpstreamRequest* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_UPSTREAM_REQUEST(self), false);
    return RP_UPSTREAM_REQUEST(self)->m_upstream;
}

RpUpstreamToDownstream**
rp_upstream_request_upstream_interface_(RpUpstreamRequest* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_UPSTREAM_REQUEST(self), false);
    return &RP_UPSTREAM_REQUEST(self)->m_upstream_interface;
}

void
rp_upstream_request_accept_trailers_from_router(RpUpstreamRequest* self, evhtp_headers_t* trailers)
{
    LOGD("(%p, %p)", self, trailers);
    g_return_if_fail(RP_IS_UPSTREAM_REQUEST(self));
    RpUpstreamRequest* me = RP_UPSTREAM_REQUEST(self);
    me->m_router_sent_end_stream = true;
    me->m_encode_trailers = true;

    rp_filter_manager_decode_trailers(me->m_filter_manager, trailers);
}

RpStreamInfo*
rp_upstream_request_stream_info(RpUpstreamRequest* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_UPSTREAM_REQUEST(self), NULL);
    return RP_STREAM_INFO(self->m_stream_info);
}

void
rp_upstream_request_on_upstream_host_selected(RpUpstreamRequest* self, RpHostDescription* host, bool pool_success)
{
    LOGD("(%p, %p, %u)", self, host, pool_success);
    g_return_if_fail(RP_IS_UPSTREAM_REQUEST(self));
    RpUpstreamInfo* upstream_info = rp_stream_info_upstream_info(rp_upstream_request_stream_info(self));
    rp_upstream_info_set_upstream_host(upstream_info, host);
    self->m_upstream_host = host;
    rp_router_filter_interface_on_upstream_host_selected(self->m_parent, host, pool_success);
}

void
rp_upstream_request_clear_request_encoder(RpUpstreamRequest* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_UPSTREAM_REQUEST(self));
    if (self->m_upstream)
    {
//TODO...        rp_stream_filter_callbacks_remove_downstream_watermark_callbacks(
//            rp_router_filter_interface_callbacks(self->m_parent));
    }
    self->m_upstream = NULL;//upstream_.reset() ?????
}

bool
rp_upstream_request_encode_complete(RpUpstreamRequest* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_UPSTREAM_REQUEST(self), false);
    return self->m_router_sent_end_stream;
}

bool
rp_upstream_request_awaiting_headers(RpUpstreamRequest* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_UPSTREAM_CALLBACKS(self), false);
    return self->m_awaiting_headers;
}
