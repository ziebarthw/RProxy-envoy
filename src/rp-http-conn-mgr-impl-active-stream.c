/*
 * rp-http-conn-mgr-impl-active-stream.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_http_conn_mgr_impl_active_stream_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_http_conn_mgr_impl_active_stream_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "router/rp-route-config-impl.h"
#include "stream_info/rp-stream-info-impl.h"
#include "rp-codec.h"
#include "rp-http-conn-manager-impl.h"
#include "rp-downstream-filter-manager.h"
#include "rp-headers.h"
#include "rp-header-utility.h"
#include "rp-http-utility.h"
#include "rp-http-conn-mgr-impl-active-stream.h"

struct state_s {
    bool m_codec_saw_local_complete : 1;
    bool m_codec_encode_complete : 1;
    bool m_on_reset_stream_called : 1;
    bool m_is_zombie_stream : 1;
    bool m_successful_upgrade : 1;

    bool m_is_internally_destroyed : 1;
    bool m_is_internally_created : 1;

    bool m_is_tunneling : 1;

    bool m_decorated_propagate : 1;

    bool m_deferred_to_next_io_iteration : 1;
    bool m_deferred_end_stream : 1;
};

static inline struct state_s
state_ctor(void)
{
    struct state_s self = {
        .m_codec_saw_local_complete = false,
        .m_codec_encode_complete = false,
        .m_on_reset_stream_called = false,
        .m_is_zombie_stream = false,
        .m_successful_upgrade = false,
        .m_is_internally_destroyed = false,
        .m_is_internally_created = false,
        .m_is_tunneling = false,
        .m_decorated_propagate = true,
        .m_deferred_to_next_io_iteration = false,
        .m_deferred_end_stream = false
    };
    return self;
}

struct _RpHttpConnMgrImplActiveStream {
    GObject parent_instance;

    RpHttpConnectionManagerImpl* m_connection_manager;

    UNIQUE_PTR(evhtp_headers_t) m_request_headers;
    UNIQUE_PTR(evhtp_headers_t) m_request_trailers;

    //evhtp_headers_t* m_informational_headers;
    UNIQUE_PTR(evhtp_headers_t) m_response_headers;
    UNIQUE_PTR(evhtp_headers_t) m_response_trailers;

    RpDownstreamFilterManager* m_filter_manager;

    RpResponseEncoder* m_response_encoder;
    RpClusterInfo* m_cached_cluster_info;

    RpRouteConfig* m_snapped_route_config;
    RpRoute* m_cached_route;

    GArray* m_cleared_cached_routes;

    evbuf_t* m_reply_body;
    evbuf_t* m_deferred_data;
    evhtp_headers_t* m_deferred_request_trailers;

    guint64 m_stream_id;

    guint32 m_idle_timeout_ms;
    guint32 m_buffer_limit;

    bool m_route_cache_blocked;

    struct state_s m_state;
};

static void stream_callbacks_iface_init(RpStreamCallbacksInterface* iface);
static void codec_event_callbacks_iface_init(RpCodecEventCallbacksInterface* iface);
static void stream_decoder_iface_init(RpStreamDecoderInterface* iface);
static void request_decoder_iface_init(RpRequestDecoderInterface* iface);
static void filter_manager_callbacks_iface_init(RpFilterManagerCallbacksInterface* iface);
static void downstream_stream_filter_callbacks_iface_init(RpDownstreamStreamFilterCallbacksInterface* iface);
static void route_cache_iface_init(RpRouteCacheInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpHttpConnMgrImplActiveStream, rp_http_conn_mgr_impl_active_stream, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_CALLBACKS, stream_callbacks_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_CODEC_EVENT_CALLBACKS, codec_event_callbacks_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_DECODER, stream_decoder_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_REQUEST_DECODER, request_decoder_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_FILTER_MANAGER_CALLBACKS, filter_manager_callbacks_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_DOWNSTREAM_STREAM_FILTER_CALLBACKS, downstream_stream_filter_callbacks_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_ROUTE_CACHE, route_cache_iface_init)
)

static inline void
block_route_cache(RpHttpConnMgrImplActiveStream* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_route_cache_blocked = true;
    g_clear_object(&self->m_snapped_route_config);//REVISIT?
}

static inline bool
route_cache_blocked(RpHttpConnMgrImplActiveStream* self)
{
    NOISY_MSG_("(%p)", self);
    return self->m_route_cache_blocked;
}

static void
refresh_cached_route_internal(RpHttpConnMgrImplActiveStream* self, RpRouteCallback cb)
{
    NOISY_MSG_("(%p, %p)", self, cb);
    if (route_cache_blocked(self))
    {
        NOISY_MSG_("route cache blocked");
        return;
    }

NOISY_MSG_("request headers %p", self->m_request_headers);

    RpRoute* route = NULL;
    if (self->m_request_headers)
    {
        RpHttpConnectionManagerImpl* connection_manager = self->m_connection_manager;
        RpConnectionManagerConfig* config = rp_http_connection_manager_impl_connection_manager_config_(connection_manager);
#if 0
        if (rp_connection_manager_config_is_routable(config))
        {

        }
#endif//0
        RpRouteConfigProvider* route_config_provider = rp_connection_manager_config_route_config_provider(config);
        RpRouteConfig* route_config = rp_route_config_provider_config_cast(route_config_provider);
        RpDispatcher* dispatcher = rp_http_connection_manager_impl_dispatcher_(connection_manager);

        rp_route_config_impl_set_dispatcher(RP_ROUTE_CONFIG_IMPL(route_config), dispatcher);

        route = rp_route_config_route(route_config,
                                        NULL,
                                        self->m_request_headers,
                                        rp_filter_manager_stream_info(RP_FILTER_MANAGER(self->m_filter_manager)),
                                        0/*stream_id_*/);
        NOISY_MSG_("route %p", route);
    }
else
{
    route = rp_stream_info_route(
                rp_filter_manager_stream_info(RP_FILTER_MANAGER(self->m_filter_manager)));
    NOISY_MSG_("route %p", route);
}

    rp_downstream_stream_filter_callbacks_set_route(RP_DOWNSTREAM_STREAM_FILTER_CALLBACKS(self), route);
}

static inline GArray*
ensure_cleared_cached_routes(RpHttpConnMgrImplActiveStream* self)
{
    NOISY_MSG_("(%p)", self);
    if (self->m_cleared_cached_routes)
    {
        NOISY_MSG_("pre-allocated cleared cached routes %p", self->m_cleared_cached_routes);
        return self->m_cleared_cached_routes;
    }
    self->m_cleared_cached_routes = g_array_sized_new(false, false, sizeof(GObject*), 3);
    NOISY_MSG_("allocated cleared cached routes %p", self->m_cleared_cached_routes);
    return self->m_cleared_cached_routes;
}

static inline void
set_cached_route(RpHttpConnMgrImplActiveStream* self, RpRoute* route)
{
    NOISY_MSG_("(%p, %p)", self, route);

    if (rp_route_cache_has_cached_route(RP_ROUTE_CACHE(self)))
    {
        GArray* cleared_cached_routes = ensure_cleared_cached_routes(self);
        g_array_append_val(cleared_cached_routes, route);
    }
    self->m_cached_route = g_steal_pointer(&route);
}

static inline evbuf_t*
ensure_reply_body(RpHttpConnMgrImplActiveStream* self)
{
    NOISY_MSG_("(%p)", self);
    if (self->m_reply_body)
    {
        NOISY_MSG_("pre-allocated reply body %p", self->m_reply_body);
        evbuffer_drain(self->m_reply_body, -1);
        return self->m_reply_body;
    }
    self->m_reply_body = evbuffer_new();
    g_assert(self->m_reply_body);
    NOISY_MSG_("allocated reply body %p", self->m_reply_body);
    return self->m_reply_body;
}

static inline evbuf_t*
ensure_deferred_data(RpHttpConnMgrImplActiveStream* self)
{
    NOISY_MSG_("(%p)", self);
    if (self->m_deferred_data)
    {
        NOISY_MSG_("pre-allocated deferred data %p", self->m_deferred_data);
        evbuffer_drain(self->m_deferred_data, -1);
        return self->m_deferred_data;
    }
    self->m_deferred_data = evbuffer_new();
    g_assert(self->m_deferred_data);
    NOISY_MSG_("allocated deferred data %p", self->m_deferred_data);
    return self->m_deferred_data;
}

static bool
has_cached_route_i(RpRouteCache* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self)->m_cached_route != NULL;
}

static void
refresh_cached_route_i(RpRouteCache* self)
{
    NOISY_MSG_("(%p)", self);
    refresh_cached_route_internal(RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self), NULL);
}

static void
route_cache_iface_init(RpRouteCacheInterface* iface)
{
    LOGD("(%p)", iface);
    iface->has_cached_route = has_cached_route_i;
    iface->refresh_cached_route = refresh_cached_route_i;
}

static void
on_reset_stream_i(RpStreamCallbacks* self, RpStreamResetReason_e reason, const char* transport_failure_reason)
{
    NOISY_MSG_("(%p, %d, %p(%s))",
        self, reason, transport_failure_reason, transport_failure_reason);
    //TODO...response_encoder_->getStream().responseDetails();
    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    struct state_s* state = &me->m_state;
    state->m_on_reset_stream_called = true;

    //TODO...
    //TODO...
    //TODO...
    rp_filter_manager_on_downstream_reset(RP_FILTER_MANAGER(me->m_filter_manager));
    rp_http_connection_manager_impl_do_deferred_stream_destroy(me->m_connection_manager, me);
}

static void
stream_callbacks_iface_init(RpStreamCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_reset_stream = on_reset_stream_i;
}

static void
on_codec_encode_complete_i(RpCodecEventCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    me->m_state.m_codec_encode_complete = true;

    //TODO...
    rp_downstream_timing_on_last_downstream_tx_byte_sent(
        rp_stream_info_downstream_timing(
            rp_filter_manager_stream_info(RP_FILTER_MANAGER(me->m_filter_manager))));


    if (me->m_state.m_is_zombie_stream)
    {
NOISY_MSG_("todo...");
        //TODO...connection_manager_.doDeferredStreamDestroy(*this);
    }
}

static void
codec_event_callbacks_iface_init(RpCodecEventCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_codec_encode_complete = on_codec_encode_complete_i;
}

static void
maybe_record_last_byte_received(RpHttpConnMgrImplActiveStream* self, bool end_stream)
{
    NOISY_MSG_("(%p, %u)", self, end_stream);
    if (end_stream &&
        !rp_downstream_filter_manager_has_last_downstream_byte_received(self->m_filter_manager))
    {
        NOISY_MSG_("calling rp_downstream_timing_on_last_downstream_rx_byte_received()");
        rp_downstream_timing_on_last_downstream_rx_byte_received(
            rp_stream_info_downstream_timing(
                rp_filter_manager_stream_info(RP_FILTER_MANAGER(self->m_filter_manager))));
        NOISY_MSG_("request end stream timestamp recorded");
    }
    NOISY_MSG_("done");
}

static void
decode_data_i(RpStreamDecoder* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)",
        self, data, data ? evbuffer_get_length(data) : 0, end_stream);

    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    struct state_s* state = &me->m_state;
    maybe_record_last_byte_received(me, end_stream);
    //TODO...filter_manager_.streamInfo().addBytesReceived(data.length());
    if (!state->m_deferred_to_next_io_iteration)
    {
        rp_filter_manager_decode_data(RP_FILTER_MANAGER(me->m_filter_manager), data, end_stream);
    }
    else
    {
        evbuf_t* deferred_data = ensure_deferred_data(me);
        evbuffer_add_buffer(deferred_data, data);
        state->m_deferred_end_stream = end_stream;
    }
}

static void
stream_decoder_iface_init(RpStreamDecoderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->decode_data = decode_data_i;
}

static inline char*
int_to_string(int val, char* buf, gsize buf_size)
{
    snprintf(buf, buf_size, "%d", val);
    return buf;
}

static evhtp_headers_t*
continue_header(void)
{
    char buf[128];
    evhtp_headers_t* headers = evhtp_headers_new();
    evhtp_headers_add_header(headers,
        evhtp_header_new(RpHeaderValues.Status, int_to_string(EVHTP_RES_CONTINUE, buf, sizeof(buf)), 0, 1));
    return headers;
}

struct OriginalIPRejectRequestOption {
    evhtp_res response_code;
    const char* body;
};

static inline struct OriginalIPRejectRequestOption
original_ip_reject_request_option_ctor(void)
{
    struct OriginalIPRejectRequestOption self = {
        .response_code = EVHTP_RES_OK,
        .body = ""
    };
    return self;
}

struct MutateRequestHeadersResult {
    struct sockaddr_storage final_remote_address;
    struct OriginalIPRejectRequestOption reject_request;
};

static inline struct MutateRequestHeadersResult
mutate_request_headers_result_ctor(void)
{
    struct MutateRequestHeadersResult self = {
        .reject_request = original_ip_reject_request_option_ctor()
    };
    return self;
}

static inline const char*
conn_scheme(RpNetworkConnection* connection)
{
    NOISY_MSG_("(%p)", connection);
    return rp_network_connection_ssl(connection) ?
                                        RpHeaderValues.SchemeValues.Https :
                                        RpHeaderValues.SchemeValues.Http;
}

static inline const char*
get_scheme(const char* forwarded_proto, bool is_ssl)
{
    NOISY_MSG_("(%p(%s), %u)", forwarded_proto, forwarded_proto, is_ssl);
    if (http_utility_scheme_is_valid(forwarded_proto))
    {
        return forwarded_proto;
    }
    return is_ssl ? RpHeaderValues.SchemeValues.Https : RpHeaderValues.SchemeValues.Http;
}

static inline struct MutateRequestHeadersResult
mutate_request_headers(evhtp_headers_t* request_headers, RpNetworkConnection* connection,
                        RpConnectionManagerConfig* config, RpRouteConfig* route_config,
                        RpLocalInfo* local_info, RpStreamInfo* stream_info)
{
    NOISY_MSG_("(%p, %p, %p, %p, %p, %p)",
        request_headers, connection, config, route_config, local_info, stream_info);

    if (!http_utility_is_upgrade(request_headers))
    {
        evhtp_header_rm_and_free(request_headers,
            evhtp_headers_find_header(request_headers, RpHeaderValues.Connection));
        evhtp_header_rm_and_free(request_headers,
            evhtp_headers_find_header(request_headers, RpHeaderValues.Upgrade));

        //TODO...sanitizeTEHeader(request_headers);
    }

    evhtp_header_rm_and_free(request_headers,
        evhtp_headers_find_header(request_headers, RpHeaderValues.KeepAlive));
    evhtp_header_rm_and_free(request_headers,
        evhtp_headers_find_header(request_headers, RpHeaderValues.ProxyConnection));
    evhtp_header_rm_and_free(request_headers,
        evhtp_headers_find_header(request_headers, RpHeaderValues.TransferEncoding));

    //TODO...sanitize referer field if exits....

    //TODO...If we are "using remote address"...

    if (!evhtp_header_find(request_headers, RpHeaderValues.ForwardedProto))
    {
        evhtp_headers_add_header(request_headers,
            evhtp_header_new(RpHeaderValues.ForwardedProto, conn_scheme(connection), 0, 0));
    }

    //TODO...if (config.appencXForwardedPort())

    //TODO...if (config.schemeToSet())

    if (!evhtp_header_find(request_headers, RpHeaderValues.Scheme))
    {
        evhtp_headers_add_header(request_headers,
            evhtp_header_new(RpHeaderValues.Scheme,
                get_scheme(evhtp_header_find(request_headers, RpHeaderValues.ForwardedProto), rp_network_connection_ssl(connection) != NULL), 0, 0));
    }
    g_autofree gchar* scheme_value = g_ascii_strdown(evhtp_header_find(request_headers, RpHeaderValues.Scheme), -1);
    evhtp_header_rm_and_free(request_headers, evhtp_headers_find_header(request_headers, RpHeaderValues.Scheme));
    evhtp_headers_add_header(request_headers,
        evhtp_header_new(RpHeaderValues.Scheme, scheme_value, 0, 1));

    //TODO...


    return mutate_request_headers_result_ctor();
}

static void
decode_headers_i(RpRequestDecoder* self, evhtp_headers_t* request_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, request_headers, end_stream);

    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    RpHttpConnectionManagerImpl* connection_manager = me->m_connection_manager;
    RpDownstreamFilterManager* filter_manager = me->m_filter_manager;
    RpStreamInfo* stream_info = rp_filter_manager_stream_info(RP_FILTER_MANAGER(filter_manager));
    struct state_s* state = &me->m_state;

    if (!rp_downstream_filter_manager_has_last_downstream_byte_received(filter_manager))
    {
        rp_downstream_timing_on_last_downstream_rx_byte_received(
            rp_stream_info_downstream_timing(stream_info));
    }

    me->m_request_headers = g_steal_pointer(&request_headers);
    rp_filter_manager_request_headers_initialized(RP_FILTER_MANAGER(filter_manager));
    //TODO...if (request_header_timer_) ...

    RpHttpServerConnection* codec_ = rp_http_connection_manager_impl_codec_(connection_manager);
    evhtp_proto protocol = rp_http_connection_protocol(RP_HTTP_CONNECTION(codec_));
    if (!rp_http_connection_manager_impl_should_keep_alive_(connection_manager))
    {
        NOISY_MSG_("closing...");
        rp_stream_info_set_should_drain_connection_upon_completion(stream_info, true);
    }

    rp_stream_info_set_protocol(stream_info, protocol);

    maybe_record_last_byte_received(me, end_stream);

    //TODO...if (!validateHeaders()) {...}?????

    RpConnectionManagerConfig* config = rp_http_connection_manager_impl_connection_manager_config_(connection_manager);
    if (rp_connection_manager_config_is_routable(config))
    {
        RpRouteConfigProvider* route_config_provider = rp_connection_manager_config_route_config_provider(config);
        if (route_config_provider != NULL)
        {
            me->m_snapped_route_config = rp_route_config_provider_config_cast(route_config_provider);
        }
        else
        {
            //TODO...?????
        }
    }
    else
    {
        me->m_snapped_route_config = rp_route_config_provider_config_cast(
                                        rp_connection_manager_config_route_config_provider(config));
    }

    //TODO...drop_request_due_to_overload...

    //TODO...if (drop_request_due_to_overload) { sendLocalReply(...) }

    const char* expect_value;
    if (rp_connection_manager_config_proxy_100_continue(config) &&
        (expect_value = evhtp_header_find(me->m_request_headers, RpHeaderValues.Expect)) &&
        g_ascii_strcasecmp(expect_value, RpHeaderValues.ExpectValues._100Continue) == 0)
    {
        evhtp_headers_t* headers = continue_header();
        rp_filter_manager_callbacks_charge_stats(RP_FILTER_MANAGER_CALLBACKS(self), headers);
        rp_response_encoder_encode_1xx_headers(me->m_response_encoder, headers);
        evhtp_kv_rm_and_free(me->m_request_headers,
                evhtp_headers_find_header(me->m_request_headers, RpHeaderValues.Expect));
        evhtp_headers_free(headers); //REVISIT - not sure about this.(?)
    }

    //TODO...connection_manager_.user_agent_.initializeFromHeaders(*request_headers_, ...);

//rp_request_decoder_send_local_reply(self, EVHTP_RES_BADREQ, ensure_reply_body(me), NULL, "", NULL);
//return;
    if (!evhtp_header_find(me->m_request_headers, RpHeaderValues.HostLegacy))
    {
        LOGE("missing host header");
        rp_request_decoder_send_local_reply(self, EVHTP_RES_BADREQ, ensure_reply_body(me), NULL, "", NULL);
        return;
    }

    //TODO...apply request header sanity checks????

    const char* path_value = evhtp_header_find(me->m_request_headers, RpHeaderValues.Path);
NOISY_MSG_("path value %p(%s), headers %p", path_value, path_value, me->m_request_headers);
    if ((!rp_header_utility_is_connect(me->m_request_headers) || path_value) && !path_value[0])
    {
        LOGE("missing path");
        rp_request_decoder_send_local_reply(self, EVHTP_RES_NOTFOUND, ensure_reply_body(me), NULL, "missing_path_rejected", NULL);
        return;
    }

    //TODO...udp-connect????

NOISY_MSG_("path value %p(%s), headers %p", path_value, path_value, me->m_request_headers);
    if (path_value && path_value[0] && path_value[0] != '/')
    {
        LOGE("relative path");
        //TODO...connection_manager_.stats_....
        rp_request_decoder_send_local_reply(self, EVHTP_RES_NOTFOUND, ensure_reply_body(me), NULL, "absolute_path_rejected", NULL);
        return;
    }

    //TODO...auto optional_port...

    //!!!!!!!!!!!!!
    //TODO...if (!state_.is_internally_created_) { mutateRequestHeaders() }
    //!!!!!!!!!!!!!
    if (!state->m_is_internally_created)
    {
        struct MutateRequestHeadersResult mutate_result = mutate_request_headers(me->m_request_headers,
            rp_http_connection_manager_impl_connection_(connection_manager),
            config,
            me->m_snapped_route_config,
            rp_http_connection_manager_impl_local_info_(connection_manager),
            rp_filter_manager_stream_info(RP_FILTER_MANAGER(filter_manager)));

        if (mutate_result.reject_request.response_code != EVHTP_RES_OK)
        {
            //TODO....sendLocalReply(...)
        }

        //TODO...filter_manager_.setDownstreamRemoteAddress(mutate_result.final_remote_address);
    }

    //TODO...

    rp_route_cache_refresh_cached_route(RP_ROUTE_CACHE(self));

    rp_stream_info_set_request_headers(stream_info, me->m_request_headers);
    struct RpCreateChainResult create_chain_result =
        rp_downstream_filter_manager_create_downstream_filter_chain(filter_manager);
    if (RpCreateChainResult_upgrade_accepted(&create_chain_result))
    {
        NOISY_MSG_("successful upgrade");
        //TODO...stats
        state->m_successful_upgrade = true;
    }

    //TODO...flush log on new reuqest...

    //TODO...traceRequest();


    if (!rp_http_connection_manager_impl_should_defer_request_proxying_to_next_io_cycle(connection_manager))
    {
        rp_filter_manager_decode_headers(RP_FILTER_MANAGER(filter_manager), me->m_request_headers, end_stream);
    }
    else
    {
        state->m_deferred_to_next_io_iteration = true;
        state->m_deferred_end_stream = end_stream;
    }

    rp_filter_manager_callbacks_reset_idle_timer(RP_FILTER_MANAGER_CALLBACKS(self));
}

static void
decode_trailers_i(RpRequestDecoder* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);

    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    struct state_s* state = &me->m_state;

    rp_filter_manager_callbacks_reset_idle_timer(RP_FILTER_MANAGER_CALLBACKS(self));

    //TODO...if (!validateTrailers(*trailers)) {...}

    maybe_record_last_byte_received(me, true);
    if (!state->m_deferred_to_next_io_iteration)
    {
        me->m_request_trailers = g_steal_pointer(&trailers);
        rp_filter_manager_decode_trailers(RP_FILTER_MANAGER(me->m_filter_manager), me->m_request_trailers);
    }
    else
    {
        me->m_deferred_request_trailers = trailers;
    }
}

static void
send_local_reply_i(RpRequestDecoder* self, evhtp_res code, evbuf_t* body, modify_headers_cb modify_headers, const char* details, void* arg)
{
    NOISY_MSG_("(%p, %d, %p(%zu), %p, %p(%s), %p)",
        self, code, body, body ? evbuffer_get_length(body) : 0, modify_headers,
        details, details, arg);
    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    rp_filter_manager_send_local_reply(RP_FILTER_MANAGER(me->m_filter_manager),
                                        code,
                                        body,
                                        modify_headers,
                                        details,
                                        arg);
}

static void
request_decoder_iface_init(RpRequestDecoderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->decode_headers = decode_headers_i;
    iface->decode_trailers = decode_trailers_i;
    iface->send_local_reply = send_local_reply_i;
}

static void
end_stream_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    me->m_state.m_codec_saw_local_complete = true;
    rp_http_connection_manager_impl_do_end_stream(me->m_connection_manager, me, true/*????*/);
}

static void
disarm_request_timeout_i(RpFilterManagerCallbacks* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    //TODO...if (request_timer_) request_timer_->disableTimer();
}

static void
reset_idle_timer_i(RpFilterManagerCallbacks* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    //TODO...if (stream_idle_timer_) stream_idle_timer_->enableTimer(idle_timeout_ms_);
}

static void
reset_stream_i(RpFilterManagerCallbacks* self, RpStreamResetReason_e reset_reason, const char* transport_failure_reason)
{
    NOISY_MSG_("(%p, %d, %p(%s))",
        self, reset_reason, transport_failure_reason, transport_failure_reason);
    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    rp_http_connection_manager_impl_do_end_stream(me->m_connection_manager, me, true);
}

static void
on_local_reply_i(RpFilterManagerCallbacks* self, evhtp_res code)
{
    NOISY_MSG_("(%p, %d)", self, code);
//TODO...
#if 0
    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    if (code == EVHTP_RES_BADREQ &&
        rp_connection_manager_impl_get_protocol(me->m_connection_manager) <= EVHTP_PROTO_11/*EVHTP_PROTO_2*/ &&
        !response_encoder_->streamErrorOnInvalidHttpMessage())
    {}
#endif//0
}

static RpDownstreamStreamFilterCallbacks *
downstream_callbacks_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_DOWNSTREAM_STREAM_FILTER_CALLBACKS(self);
}

static void
mutate_response_headers(evhtp_headers_t* response_headers, evhtp_headers_t* request_headers, RpConnectionManagerConfig* config, RpStreamInfo* stream_info, bool clear_hop_by_hop)
{
    NOISY_MSG_("(%p, %p, %p, %p, %u)",
        response_headers, request_headers, config, stream_info, clear_hop_by_hop);

    if (request_headers && http_utility_is_upgrade(request_headers) && http_utility_is_upgrade(response_headers))
    {
        bool no_body = (!evhtp_header_find(response_headers, RpHeaderValues.TransferEncoding) &&
                            !evhtp_header_find(response_headers, RpHeaderValues.ContentLength));

        //TODO:bool is_1xx ...
    }
    else
    {
        if (clear_hop_by_hop)
        {
            evhtp_header_rm_and_free(response_headers,
                evhtp_headers_find_header(response_headers, RpHeaderValues.Connection));
            evhtp_header_rm_and_free(response_headers,
                evhtp_headers_find_header(response_headers, RpHeaderValues.Upgrade));
        }
    }
    if (clear_hop_by_hop)
    {
        evhtp_header_rm_and_free(response_headers,
            evhtp_headers_find_header(response_headers, RpHeaderValues.TransferEncoding));
        evhtp_header_rm_and_free(response_headers,
            evhtp_headers_find_header(response_headers, RpHeaderValues.KeepAlive));
        evhtp_header_rm_and_free(response_headers,
            evhtp_headers_find_header(response_headers, RpHeaderValues.ProxyConnection));
    }

    //TODO...
}

static void
encode_1xx_headers(RpFilterManagerCallbacks* self, evhtp_headers_t* response_headers)
{
    NOISY_MSG_("(%p, %p)", self, response_headers);
}

static void
encode_headers_i(RpFilterManagerCallbacks* self, evhtp_headers_t* response_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, response_headers, end_stream);

    //TODO...if (!header.Date())...

    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    RpHttpConnectionManagerImpl* connection_manager = me->m_connection_manager;
    RpStreamInfo* stream_info = rp_filter_manager_stream_info(RP_FILTER_MANAGER(me->m_filter_manager));
    RpConnectionManagerConfig* config = rp_http_connection_manager_impl_connection_manager_config_(connection_manager);
    struct state_s* state = &me->m_state;

    mutate_response_headers(response_headers, me->m_request_headers, config, stream_info, true);

    if (rp_http_connection_manager_impl_get_protocol(connection_manager) == EVHTP_PROTO_10)
    {
        if (!evhtp_header_find(response_headers, RpHeaderValues.ContentLength))
        {
            NOISY_MSG_("calling rp_stream_info_set_should_drain_connection_upon_completion(%p, %u)", stream_info, true);
            rp_stream_info_set_should_drain_connection_upon_completion(stream_info, true);
        }
        if (!rp_stream_info_should_drain_connection_upon_completion(stream_info))
        {
            NOISY_MSG_("adding keep alive request header");
            evhtp_header_rm_and_free(response_headers,
                evhtp_headers_find_header(response_headers, RpHeaderValues.Connection));
            evhtp_headers_add_header(response_headers,
                evhtp_header_new(RpHeaderValues.Connection, RpHeaderValues.ConnectionValues.KeepAlive, 0, 0));
        }
    }

    if (rp_http_connection_manager_impl_get_drain_state(connection_manager) == RpDrainState_NotDraining &&
        rp_stream_info_should_drain_connection_upon_completion(stream_info))
    {
        NOISY_MSG_("closing connection due to connection close header");
        rp_http_connection_manager_impl_set_drain_state(connection_manager, RpDrainState_Closing);
    }

    if (!rp_downstream_filter_manager_has_last_downstream_byte_received(me->m_filter_manager))
    {
        if (rp_http_connection_manager_impl_get_protocol(connection_manager) <= EVHTP_PROTO_11)
        {
            rp_http_connection_manager_impl_set_drain_state(connection_manager, RpDrainState_Closing);
        }
    }

    //TODO...if (Utility::isUpgrade(headers))...state_.is_tunneling_ = true;
    if (http_utility_is_upgrade(response_headers) ||
        rp_header_utility_is_connect_response(me->m_request_headers, response_headers))
    {
        NOISY_MSG_("tunneling");
        state->m_is_tunneling = true;
    }

    block_route_cache(me);

    if (rp_http_connection_manager_impl_get_drain_state(connection_manager) != RpDrainState_NotDraining &&
        rp_http_connection_manager_impl_get_protocol(connection_manager) <= EVHTP_PROTO_11)
    {
        if (!state->m_is_tunneling)
        {
            evhtp_header_rm_and_free(response_headers,
                evhtp_headers_find_header(response_headers, RpHeaderValues.Connection));
            evhtp_headers_add_header(response_headers,
                evhtp_header_new(RpHeaderValues.Connection, RpHeaderValues.ConnectionValues.Close, 0, 0));
        }
    }

    //TODO...tracing...

    rp_filter_manager_callbacks_charge_stats(self, response_headers);

    if (!state->m_is_tunneling /*&& flushAccessLog...*/)
    {
        //TODO...log(successful tunnel)
    }

    rp_downstream_timing_on_first_downstream_tx_byte_sent(rp_stream_info_downstream_timing(stream_info));

    //TODO...if (header_validator_) ...

    rp_response_encoder_encode_headers(me->m_response_encoder, response_headers, end_stream);
}

static void
encode_data_i(RpFilterManagerCallbacks* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)",
        self, data, data ? evbuffer_get_length(data) : 0, end_stream);

    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    //TODO...filter_manager_.streamInfo().addBytesSent(data.length());
    rp_stream_encoder_encode_data(RP_STREAM_ENCODER(me->m_response_encoder), data, end_stream);
}

static void
encode_trailers_i(RpFilterManagerCallbacks* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    rp_response_encoder_encode_trailers(me->m_response_encoder, trailers);
}

static void
set_request_trailers_i(RpFilterManagerCallbacks* self, evhtp_headers_t* request_trailers)
{
    NOISY_MSG_("(%p, %p)", self, request_trailers);
    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    me->m_request_trailers = g_steal_pointer(&request_trailers);
}

static void
set_response_headers_i(RpFilterManagerCallbacks* self, evhtp_headers_t* response_headers)
{
    NOISY_MSG_("(%p, %p)", self, response_headers);
    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    me->m_response_headers = g_steal_pointer(&response_headers);
}

static void
set_response_trailers_i(RpFilterManagerCallbacks* self, evhtp_headers_t* response_trailers)
{
    NOISY_MSG_("(%p, %p)", self, response_trailers);
    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    me->m_response_trailers = g_steal_pointer(&response_trailers);
}

static void
charge_stats_i(RpFilterManagerCallbacks* self G_GNUC_UNUSED, evhtp_headers_t* response_headers G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p)", self, response_headers);
    evhtp_res response_code = http_utility_get_response_status(response_headers);
    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    rp_stream_info_set_response_code(
        rp_filter_manager_stream_info(RP_FILTER_MANAGER(me->m_filter_manager)), response_code);

    //TODO...
}

static evhtp_headers_t*
request_headers_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self)->m_request_headers;
}

static evhtp_headers_t*
request_trailers_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self)->m_request_trailers;
}

static evhtp_headers_t*
response_headers_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self)->m_response_headers;
}

static evhtp_headers_t*
response_trailers_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self)->m_response_trailers;
}

static bool
is_half_close_enabled_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    //TODO...connection_manager_.allow_upstream_half_close_;
    return false;
}

static void
send_go_away_and_close_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);

}

static lztq*
rules_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    RpHttpConnectionManagerImpl* connection_manager = me->m_connection_manager;
    RpConnectionManagerConfig* config = rp_http_connection_manager_impl_connection_manager_config_(connection_manager);
    return rp_connection_manager_config_rules(config);
}

static void
filter_manager_callbacks_iface_init(RpFilterManagerCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->end_stream = end_stream_i;
    iface->disarm_request_timeout = disarm_request_timeout_i;
    iface->reset_idle_timer = reset_idle_timer_i;
    iface->reset_stream = reset_stream_i;
    iface->on_local_reply = on_local_reply_i;
    iface->downstream_callbacks = downstream_callbacks_i;
    iface->encode_1xx_headers = encode_1xx_headers;
    iface->encode_headers = encode_headers_i;
    iface->encode_data = encode_data_i;
    iface->encode_trailers = encode_trailers_i;
    iface->set_request_trailers = set_request_trailers_i;
    iface->set_response_headers = set_response_headers_i;
    iface->set_response_trailers = set_response_trailers_i;
    iface->charge_stats = charge_stats_i;
    iface->request_headers = request_headers_i;
    iface->request_trailers = request_trailers_i;
    iface->response_headers = response_headers_i;
    iface->response_trailers = response_trailers_i;
    iface->is_half_close_enabled = is_half_close_enabled_i;
    iface->send_go_away_and_close = send_go_away_and_close_i;
    iface->rules = rules_i;
}

static RpRoute*
route_i(RpDownstreamStreamFilterCallbacks* self, RpRouteCallback cb)
{
    NOISY_MSG_("(%p, %p)", self, cb);
    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    if (me->m_cached_route)
    {
        NOISY_MSG_("cached route");
        return me->m_cached_route;
    }
    refresh_cached_route_internal(me, cb);
    return me->m_cached_route;
}

static void
set_route_i(RpDownstreamStreamFilterCallbacks* self, RpRoute* route)
{
    NOISY_MSG_("(%p, %p)", self, route);

    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    if (route_cache_blocked(me))
    {
        NOISY_MSG_("route cache blocked");
        return;
    }

    set_cached_route(me, route);
    if (!route || !rp_route_route_entry(route))
    {
        NOISY_MSG_("clearing cached cluster info %p", me->m_cached_cluster_info);
        g_clear_object(&me->m_cached_cluster_info);
//        me->m_cached_cluster_info = NULL;
    }
    else
    {
        RpThreadLocalCluster* cluster = rp_cluster_manager_get_thread_local_cluster(
            rp_http_connection_manager_impl_cluster_manager_(me->m_connection_manager),
                                                                rp_route_entry_cluster_name(rp_route_route_entry(route)));
        me->m_cached_cluster_info = cluster ? rp_thread_local_cluster_info(cluster) : NULL;
    }

    RpStreamInfo* stream_info = rp_filter_manager_stream_info(RP_FILTER_MANAGER(me->m_filter_manager));
    rp_stream_info_impl_set_route_(RP_STREAM_INFO_IMPL(stream_info), route);
    rp_stream_info_set_upstream_cluster_info(stream_info, me->m_cached_cluster_info);

    //TODO...refreshIdleTimeout();
}

static void
clear_route_cache_i(RpDownstreamStreamFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self);
    if (route_cache_blocked(me))
    {
        NOISY_MSG_("route cache blocked");
        return;
    }

    set_cached_route(me, NULL);

}

static void
downstream_stream_filter_callbacks_iface_init(RpDownstreamStreamFilterCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->route = route_i;
    iface->set_route = set_route_i;
    iface->clear_route_cache = clear_route_cache_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpHttpConnMgrImplActiveStream* me = RP_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(obj);
    g_clear_pointer(&me->m_deferred_data, evbuffer_free);
    g_clear_pointer(&me->m_reply_body, evbuffer_free);
    g_clear_object(&me->m_filter_manager);
    g_clear_object(&me->m_response_encoder);
    g_clear_object(&me->m_cached_route);
    g_clear_pointer(&me->m_request_headers, evhtp_headers_free);
    g_clear_pointer(&me->m_request_trailers, evhtp_headers_free);
    g_clear_pointer(&me->m_response_headers, evhtp_headers_free);
    g_clear_pointer(&me->m_response_trailers, evhtp_headers_free);

    G_OBJECT_CLASS(rp_http_conn_mgr_impl_active_stream_parent_class)->dispose(obj);
}

static void
rp_http_conn_mgr_impl_active_stream_class_init(RpHttpConnMgrImplActiveStreamClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_http_conn_mgr_impl_active_stream_init(RpHttpConnMgrImplActiveStream* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_state = state_ctor();
    self->m_route_cache_blocked = false;
}

static inline RpHttpConnMgrImplActiveStream*
constructed(RpHttpConnMgrImplActiveStream* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_stream_id = g_random_int();

    RpHttpConnectionManagerImpl* connection_manager_ = self->m_connection_manager;
    RpNetworkConnection* connection = rp_network_filter_callbacks_connection(
                                        RP_NETWORK_FILTER_CALLBACKS(
                                            rp_http_connection_manager_impl_read_callbacks_(connection_manager_)));
    RpConnectionManagerConfig* config = rp_http_connection_manager_impl_connection_manager_config_(connection_manager_);
    RpFilterChainFactory* filter_factory = rp_connection_manager_config_filter_factory(config);
    RpLocalReply* local_reply = rp_connection_manager_config_local_reply(config);
    RpDispatcher* dispatcher = rp_http_connection_manager_impl_dispatcher_(connection_manager_);
    RpHttpServerConnection* codec_ = rp_http_connection_manager_impl_codec_(connection_manager_);
    bool proxy_100_continue = rp_connection_manager_config_proxy_100_continue(config);
    self->m_filter_manager = rp_downstream_filter_manager_new(RP_FILTER_MANAGER_CALLBACKS(self),
                                                                dispatcher,
                                                                connection,
                                                                self->m_stream_id,
                                                                proxy_100_continue,
                                                                self->m_buffer_limit,
                                                                filter_factory,
                                                                local_reply,
                                                                rp_http_connection_protocol(RP_HTTP_CONNECTION(codec_)),
                                                                rp_stream_info_filter_state(
                                                                    rp_network_connection_stream_info(connection)));
    return self;
}

RpHttpConnMgrImplActiveStream*
rp_http_conn_mgr_impl_active_stream_new(RpHttpConnectionManagerImpl* connection_manager, guint32 buffer_limit)
{
    LOGD("(%p, %u)", connection_manager, buffer_limit);
    g_return_val_if_fail(RP_IS_HTTP_CONNECTION_MANAGER_IMPL(connection_manager), NULL);
    RpHttpConnMgrImplActiveStream* self = g_object_new(RP_TYPE_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM, NULL);
    self->m_connection_manager = connection_manager;
    self->m_buffer_limit = buffer_limit;
    return constructed(self);
}

RpResponseEncoder*
rp_http_conn_mgr_impl_active_stream_response_encoder(RpHttpConnMgrImplActiveStream* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self), NULL);
    return self->m_response_encoder;
}

RpDownstreamFilterManager*
rp_http_conn_mgr_impl_active_stream_filter_manager_(RpHttpConnMgrImplActiveStream* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self), NULL);
    return self->m_filter_manager;
}

bool
rp_http_conn_mgr_impl_active_stream_get_codec_saw_local_complete(RpHttpConnMgrImplActiveStream* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self), false);
    return self->m_state.m_codec_saw_local_complete;
}

void
rp_http_conn_mgr_impl_active_stream_set_codec_saw_local_complete(RpHttpConnMgrImplActiveStream* self, bool v)
{
    NOISY_MSG_("(%p, %u)", self, v);
    g_return_if_fail(RP_IS_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self));
    self->m_state.m_codec_saw_local_complete = v;
}

void
rp_http_conn_mgr_impl_active_stream_set_is_internally_created(RpHttpConnMgrImplActiveStream* self, bool v)
{
    NOISY_MSG_("(%p, %u)", self, v);
    g_return_if_fail(RP_IS_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self));
    self->m_state.m_is_internally_created = v;
}

void
rp_http_conn_mgr_impl_active_stream_set_response_encoder(RpHttpConnMgrImplActiveStream* self, RpResponseEncoder* response_encoder)
{
    NOISY_MSG_("(%p, %p)", self, response_encoder);
    g_return_if_fail(RP_IS_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self));
    g_return_if_fail(RP_IS_RESPONSE_ENCODER(response_encoder));
    self->m_response_encoder = g_object_ref(response_encoder);
}

guint32
rp_http_conn_mgr_impl_active_stream_get_idle_timeout(RpHttpConnMgrImplActiveStream* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self), false);
    return self->m_idle_timeout_ms;
}

bool
rp_http_conn_mgr_impl_active_stream_can_destroy_stream(RpHttpConnMgrImplActiveStream* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self), false);
    struct state_s* state = &self->m_state;
    return state->m_on_reset_stream_called ||
            state->m_codec_encode_complete ||
            state->m_is_internally_destroyed;
}

bool
rp_http_conn_mgr_impl_active_stream_is_internally_destroyed(RpHttpConnMgrImplActiveStream* self)
{
    g_return_val_if_fail(RP_IS_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self), false);
    return self->m_state.m_is_internally_destroyed;
}

void
rp_http_conn_mgr_impl_active_stream_set_is_zombie_stream(RpHttpConnMgrImplActiveStream* self, bool v)
{
    g_return_if_fail(RP_IS_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self));
    self->m_state.m_is_zombie_stream = v;
}

void
rp_http_conn_mgr_impl_active_stream_complete_request(RpHttpConnMgrImplActiveStream* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_if_fail(RP_IS_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self));
    rp_stream_info_on_request_complete(
        rp_filter_manager_stream_info(RP_FILTER_MANAGER(self->m_filter_manager)));

    //TODO...

    //TODO...
}

evhtp_headers_t*
rp_http_conn_mgr_impl_active_stream_response_headers_(RpHttpConnMgrImplActiveStream* self)
{
    g_return_val_if_fail(RP_IS_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self), false);
    return self->m_response_headers;
}

evhtp_headers_t*
rp_http_conn_mgr_impl_active_stream_request_headers_(RpHttpConnMgrImplActiveStream* self)
{
    g_return_val_if_fail(RP_IS_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self), false);
    return self->m_request_headers;
}

RpHttpConnectionManagerImpl*
rp_http_conn_mgr_impl_active_stream_get_connection_manager(RpHttpConnMgrImplActiveStream* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self), NULL);
    return self->m_connection_manager;
}

RpClusterManager*
rp_http_conn_mgr_impl_active_stream_get_cluster_manager(RpHttpConnMgrImplActiveStream* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(self), NULL);
    return rp_http_connection_manager_impl_cluster_manager_(self->m_connection_manager);
}
