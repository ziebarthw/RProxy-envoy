/*
 * rp-upstream-request-fmc.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_upstream_request_fmc_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_upstream_request_fmc_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "router/rp-router-filter-interface.h"
#include "rp-filter-manager.h"
#include "rp-upstream-request-fmc.h"

// UpstreamRequestFilterManagerCallbacks
struct _RpUpstreamRequestFmc {
    GObject parent_instance;

    evhtp_headers_t* m_request_trailers;
    evhtp_headers_t* m_response_headers;
    evhtp_headers_t* m_response_trailers;
    RpUpstreamRequest* m_upstream_request;
};

enum
{
    PROP_0, // Reserved.
    PROP_UPSTREAM_REQUEST,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void filter_manager_callbacks_iface_init(RpFilterManagerCallbacksInterface* iface);
static void upstream_stream_filter_callbacks_iface_init(RpUpstreamStreamFilterCallbacksInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpUpstreamRequestFmc, rp_upstream_request_fmc, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_FILTER_MANAGER_CALLBACKS, filter_manager_callbacks_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_UPSTREAM_STREAM_FILTER_CALLBACKS, upstream_stream_filter_callbacks_iface_init)
)

static void
encode_headers_i(RpFilterManagerCallbacks* self, evhtp_headers_t* response_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, response_headers, end_stream);
    RpUpstreamRequestFmc* me = RP_UPSTREAM_REQUEST_FMC(self);
    RpResponseDecoder* response_decoder = RP_RESPONSE_DECODER(me->m_upstream_request);
    rp_response_decoder_decode_headers(response_decoder, response_headers, end_stream);
}

static void
encode_1xx_headers_i(RpFilterManagerCallbacks* self, evhtp_headers_t* response_headers)
{
    NOISY_MSG_("(%p, %p)", self, response_headers);
    RpUpstreamRequestFmc* me = RP_UPSTREAM_REQUEST_FMC(self);
    RpResponseDecoder* response_decoder = RP_RESPONSE_DECODER(me->m_upstream_request);
    rp_response_decoder_decode_1xx_headers(response_decoder, response_headers);
}

static void
encode_data_i(RpFilterManagerCallbacks* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)",
        self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    RpUpstreamRequestFmc* me = RP_UPSTREAM_REQUEST_FMC(self);
    RpStreamDecoder* stream_decoder = RP_STREAM_DECODER(me->m_upstream_request);
    rp_stream_decoder_decode_data(stream_decoder, data, end_stream);
}

static void
encode_trailers_i(RpFilterManagerCallbacks* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    RpUpstreamRequestFmc* me = RP_UPSTREAM_REQUEST_FMC(self);
    RpResponseDecoder* response_decoder = RP_RESPONSE_DECODER(me->m_upstream_request);
    rp_response_decoder_decode_trailers(response_decoder, trailers);
}

static void
set_request_trailers_i(RpFilterManagerCallbacks* self, evhtp_headers_t* request_trailers)
{
    NOISY_MSG_("(%p, %p)", self, request_trailers);
    RP_UPSTREAM_REQUEST_FMC(self)->m_request_trailers = request_trailers;
}

static void
set_response_headers_i(RpFilterManagerCallbacks* self, evhtp_headers_t* response_headers)
{
    NOISY_MSG_("(%p, %p)", self, response_headers);
    RP_UPSTREAM_REQUEST_FMC(self)->m_response_headers = response_headers;
}

static void
set_response_trailers_i(RpFilterManagerCallbacks* self, evhtp_headers_t* response_trailers)
{
    NOISY_MSG_("(%p, %p)", self, response_trailers);
    RP_UPSTREAM_REQUEST_FMC(self)->m_response_trailers = response_trailers;
}

static evhtp_headers_t*
request_headers_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpUpstreamRequest* upstream_request = RP_UPSTREAM_REQUEST_FMC(self)->m_upstream_request;
NOISY_MSG_("upstream_request %p", upstream_request);
    RpRouterFilterInterface* parent_ = rp_upstream_request_parent_(upstream_request);
NOISY_MSG_("parent %p", parent_);
    return rp_router_filter_interface_downstream_headers(parent_);
}

static evhtp_headers_t*
request_trailers_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpUpstreamRequestFmc* me = RP_UPSTREAM_REQUEST_FMC(self);
    RpUpstreamRequest* upstream_request = me->m_upstream_request;
    RpRouterFilterInterface* parent_ = rp_upstream_request_parent_(upstream_request);
    evhtp_headers_t* trailers = rp_router_filter_interface_downstream_trailers(parent_);
    if (trailers)
    {
        return trailers;
    }
    if (me->m_request_trailers)
    {
        return me->m_request_trailers;
    }
    return NULL;
}

static void
reset_stream_i(RpFilterManagerCallbacks* self, RpStreamResetReason_e reset_reason, const char* transport_failure_reason)
{
    NOISY_MSG_("(%p, %d, %p(%s))",
        self, reset_reason, transport_failure_reason, transport_failure_reason);
    bool is_codec_error;
    //TODO...else {
    is_codec_error = g_ascii_strcasecmp(transport_failure_reason, "codec_error") == 0;
    RpUpstreamRequest* upstream_request = RP_UPSTREAM_REQUEST_FMC(self)->m_upstream_request;
    if (reset_reason == RpStreamResetReason_LocalReset && !is_codec_error)
    {
        RpRouterFilterInterface* parent_ = rp_upstream_request_parent_(upstream_request);
        RpStreamDecoderFilterCallbacks* callbacks = rp_router_filter_interface_callbacks(parent_);
        rp_stream_filter_callbacks_reset_stream(RP_STREAM_FILTER_CALLBACKS(callbacks), reset_reason, transport_failure_reason);
    }
    else
    {
        rp_stream_callbacks_on_reset_stream(RP_STREAM_CALLBACKS(upstream_request), reset_reason, transport_failure_reason);
    }
}

static RpClusterInfo*
cluster_info_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpUpstreamRequestFmc* me = RP_UPSTREAM_REQUEST_FMC(self);
    RpUpstreamRequest* upstream_request = me->m_upstream_request;
    RpRouterFilterInterface* parent_ = rp_upstream_request_parent_(upstream_request);
    RpStreamDecoderFilterCallbacks* callbacks = rp_router_filter_interface_callbacks(parent_);
    return rp_stream_filter_callbacks_cluster_info(RP_STREAM_FILTER_CALLBACKS(callbacks));
}

static void
on_decoder_filter_below_write_buffer_low_water_mark_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpUpstreamRequest* upstream_request = RP_UPSTREAM_REQUEST_FMC(self)->m_upstream_request;
    rp_stream_callbacks_on_below_write_buffer_low_watermark(RP_STREAM_CALLBACKS(upstream_request));
}

static void
on_decoder_filter_above_write_buffer_high_water_mark_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpUpstreamRequest* upstream_request = RP_UPSTREAM_REQUEST_FMC(self)->m_upstream_request;
    rp_stream_callbacks_on_above_write_buffer_high_watermark(RP_STREAM_CALLBACKS(upstream_request));
}

static void
end_stream_i(RpFilterManagerCallbacks* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static void
disarm_request_timeout_i(RpFilterManagerCallbacks* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static void
reset_idle_timer_i(RpFilterManagerCallbacks* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static void
on_local_reply_i(RpFilterManagerCallbacks* self G_GNUC_UNUSED, evhtp_res code G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %d)", self, code);
}

static void
send_go_away_and_close_i(RpFilterManagerCallbacks* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static bool
is_half_close_enabled_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpUpstreamRequest* upstream_request = RP_UPSTREAM_REQUEST_FMC(self)->m_upstream_request;
    return rp_upstream_request_enable_half_close_(upstream_request);
}

static RpDownstreamStreamFilterCallbacks*
downstream_callbacks_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return NULL;
}

static RpUpstreamStreamFilterCallbacks*
upstream_callbacks_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_UPSTREAM_STREAM_FILTER_CALLBACKS(self);
}

static evhtp_headers_t*
response_headers_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_UPSTREAM_REQUEST_FMC(self)->m_response_headers;
}

static evhtp_headers_t*
response_trailers_i(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_UPSTREAM_REQUEST_FMC(self)->m_response_trailers;
}

static void
filter_manager_callbacks_iface_init(RpFilterManagerCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->encode_headers = encode_headers_i;
    iface->encode_1xx_headers = encode_1xx_headers_i;
    iface->encode_data = encode_data_i;
    iface->encode_trailers = encode_trailers_i;
    iface->set_request_trailers = set_request_trailers_i;
    iface->set_response_headers = set_response_headers_i;
    iface->set_response_trailers = set_response_trailers_i;
    iface->request_headers = request_headers_i;
    iface->request_trailers = request_trailers_i;
    iface->reset_stream = reset_stream_i;
    iface->cluster_info = cluster_info_i;
    iface->on_decoder_filter_below_write_buffer_low_water_mark = on_decoder_filter_below_write_buffer_low_water_mark_i;
    iface->on_decoder_filter_above_write_buffer_high_water_mark = on_decoder_filter_above_write_buffer_high_water_mark_i;
    iface->end_stream = end_stream_i;
    iface->disarm_request_timeout = disarm_request_timeout_i;
    iface->reset_idle_timer = reset_idle_timer_i;
    iface->on_local_reply = on_local_reply_i;
    iface->send_go_away_and_close = send_go_away_and_close_i;
    iface->is_half_close_enabled = is_half_close_enabled_i;
    iface->downstream_callbacks = downstream_callbacks_i;
    iface->upstream_callbacks = upstream_callbacks_i;
    iface->response_headers = response_headers_i;
    iface->response_trailers = response_trailers_i;
}

static RpStreamInfo*
upstream_stream_info_i(RpUpstreamStreamFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_upstream_request_stream_info(RP_UPSTREAM_REQUEST_FMC(self)->m_upstream_request);
}

static RpGenericUpstream*
upstream_i(RpUpstreamStreamFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpUpstreamRequest* upstream_request = RP_UPSTREAM_REQUEST_FMC(self)->m_upstream_request;
    return rp_upstream_request_upstream_(upstream_request);
}

static bool
paused_for_connect_i(RpUpstreamStreamFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpUpstreamRequest* upstream_request = RP_UPSTREAM_REQUEST_FMC(self)->m_upstream_request;
    return rp_upstream_request_paused_for_connect_(upstream_request);
}

static void
set_paused_for_connect_i(RpUpstreamStreamFilterCallbacks* self, bool value)
{
    NOISY_MSG_("(%p, %u)", self, value);
    RpUpstreamRequest* upstream_request = RP_UPSTREAM_REQUEST_FMC(self)->m_upstream_request;
    rp_upstream_request_set_paused_for_connect_(upstream_request, value);
}

static bool
paused_for_web_socket_upgrade_i(RpUpstreamStreamFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpUpstreamRequest* upstream_request = RP_UPSTREAM_REQUEST_FMC(self)->m_upstream_request;
    return rp_upstream_request_paused_for_websocket_upgrade_(upstream_request);
}

static void
set_paused_for_web_socket_upgrade_i(RpUpstreamStreamFilterCallbacks* self, bool value)
{
    NOISY_MSG_("(%p, %u)", self, value);
    RpUpstreamRequest* upstream_request = RP_UPSTREAM_REQUEST_FMC(self)->m_upstream_request;
    rp_upstream_request_set_paused_for_websocket_upgrade_(upstream_request, value);
}

static void
add_upstream_callbacks_i(RpUpstreamStreamFilterCallbacks* self, RpUpstreamCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    RpUpstreamRequest* upstream_request = RP_UPSTREAM_REQUEST_FMC(self)->m_upstream_request;
    GSList** upstream_callbacks_ = rp_upstream_request_upstream_callbacks_(upstream_request);
    *upstream_callbacks_ = g_slist_append(*upstream_callbacks_, callbacks);
}

static void
set_upstream_to_downstream_i(RpUpstreamStreamFilterCallbacks* self, RpUpstreamToDownstream* upstream_to_downstream_interface)
{
    NOISY_MSG_("(%p, %p)", self, upstream_to_downstream_interface);
    RpUpstreamRequest* upstream_request = RP_UPSTREAM_REQUEST_FMC(self)->m_upstream_request;
    RpUpstreamToDownstream** upstream_interface_ = rp_upstream_request_upstream_interface_(upstream_request);
    *upstream_interface_ = upstream_to_downstream_interface;
}

static RpHttpConnPoolInstStreamOptions
upstream_stream_options_i(RpUpstreamStreamFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpUpstreamRequest* upstream_request = RP_UPSTREAM_REQUEST_FMC(self)->m_upstream_request;
    return rp_upstream_to_downstream_upstream_stream_options(RP_UPSTREAM_TO_DOWNSTREAM(upstream_request));
}

static void
upstream_stream_filter_callbacks_iface_init(RpUpstreamStreamFilterCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->upstream_stream_info = upstream_stream_info_i;
    iface->upstream = upstream_i;
    iface->paused_for_connect = paused_for_connect_i;
    iface->set_paused_for_connect = set_paused_for_connect_i;
    iface->paused_for_web_socket_upgrade = paused_for_web_socket_upgrade_i;
    iface->set_paused_for_web_socket_upgrade = set_paused_for_web_socket_upgrade_i;
    iface->add_upstream_callbacks = add_upstream_callbacks_i;
    iface->set_upstream_to_downstream = set_upstream_to_downstream_i;
    iface->upstream_stream_options = upstream_stream_options_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_UPSTREAM_REQUEST:
            g_value_set_object(value, RP_UPSTREAM_REQUEST_FMC(obj)->m_upstream_request);
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
        case PROP_UPSTREAM_REQUEST:
            RP_UPSTREAM_REQUEST_FMC(obj)->m_upstream_request = g_value_get_object(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_upstream_request_fmc_parent_class)->dispose(obj);
}

static void
rp_upstream_request_fmc_class_init(RpUpstreamRequestFmcClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_UPSTREAM_REQUEST] = g_param_spec_object("upstream-request",
                                                    "Upstream request",
                                                    "UpstreamRequest Instance",
                                                    RP_TYPE_UPSTREAM_REQUEST,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_upstream_request_fmc_init(RpUpstreamRequestFmc* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpUpstreamRequestFmc*
rp_upstream_request_fmc_new(RpUpstreamRequest* upstream_request)
{
    LOGD("(%p)", upstream_request);
    g_return_val_if_fail(RP_IS_UPSTREAM_REQUEST(upstream_request), NULL);
    return g_object_new(RP_TYPE_UPSTREAM_REQUEST_FMC,
                        "upstream-request", upstream_request,
                        NULL);
}
