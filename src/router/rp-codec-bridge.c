/*
 * rp-codec-bridge.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_codec_bridge_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_codec_bridge_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "router/rp-upstream-codec-filter.h"
#include "rp-http-utility.h"
#include "router/rp-codec-bridge.h"

struct _RpCodecBridge {
    GObject parent_instance;

    RpUpstreamCodecFilter* m_filter;

    bool m_seen_1xx_headers;
};

enum
{
    PROP_0, // Reserved.
    PROP_FILTER,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void stream_decoder_iface_init(RpStreamDecoderInterface* iface);
static void response_decoder_iface_init(RpResponseDecoderInterface* iface);
static void stream_callbacks_iface_init(RpStreamCallbacksInterface* iface);
static void upstream_to_downstream_iface_init(RpUpstreamToDownstreamInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpCodecBridge, rp_codec_bridge, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_DECODER, stream_decoder_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_RESPONSE_DECODER, response_decoder_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_CALLBACKS, stream_callbacks_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_UPSTREAM_TO_DOWNSTREAM, upstream_to_downstream_iface_init)
)

#define CALLBACKS(s) \
    rp_upstream_codec_filter_callbacks_(RP_CODEC_BRIDGE(s)->m_filter)
#define STREAM_FILTER_CALLBACKS(s) RP_STREAM_FILTER_CALLBACKS(CALLBACKS(s))

static void
maybe_end_decode(RpCodecBridge* self, bool end_stream)
{
    NOISY_MSG_("(%p, %u)", self, end_stream);
    if (end_stream)
    {
        rp_upstream_timing_on_last_upstream_rx_byte_received(
            rp_upstream_codec_filter_upstream_timing(self->m_filter));
    }
}

static void
decode_data_i(RpStreamDecoder* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    rp_stream_decoder_filter_callbacks_encode_data(CALLBACKS(self), data, end_stream);
}

static void
stream_decoder_iface_init(RpStreamDecoderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->decode_data = decode_data_i;
}

static void
decode_1xx_headers_i(RpResponseDecoder* self, evhtp_headers_t* response_headers)
{
    NOISY_MSG_("(%p, %p)", self, response_headers);
    RpCodecBridge* me = RP_CODEC_BRIDGE(self);
    if (!me->m_seen_1xx_headers)
    {
        me->m_seen_1xx_headers = true;
        rp_stream_decoder_filter_callbacks_encode_1xx_headers(CALLBACKS(me), response_headers);
    }
}

static inline bool
is_2xx(evhtp_res code)
{
    return code >= 200 && code <= 300;
}

static void
decode_headers_i(RpResponseDecoder* self, evhtp_headers_t* response_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, response_headers, end_stream);
    RpCodecBridge* me = RP_CODEC_BRIDGE(self);
    rp_upstream_timing_on_first_upstream_rx_byte_received(
        rp_upstream_codec_filter_upstream_timing(me->m_filter));

    RpStreamDecoderFilterCallbacks* callbacks_ = CALLBACKS(me);
    if (rp_upstream_stream_filter_callbacks_paused_for_connect(
        rp_stream_filter_callbacks_upstream_callbacks(RP_STREAM_FILTER_CALLBACKS(callbacks_))) &&
        is_2xx(http_utility_get_response_status(response_headers)))
    {
        rp_upstream_stream_filter_callbacks_set_paused_for_connect(
            rp_stream_filter_callbacks_upstream_callbacks(RP_STREAM_FILTER_CALLBACKS(callbacks_)), false);
        rp_stream_decoder_filter_callbacks_continue_decoding(callbacks_);
    }

    //TODO...

    maybe_end_decode(me, end_stream);
    rp_stream_decoder_filter_callbacks_encode_headers(CALLBACKS(me), response_headers, end_stream, "via_upstream");
}

static void
decode_trailers_i(RpResponseDecoder* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    maybe_end_decode(RP_CODEC_BRIDGE(self), true);
    rp_stream_decoder_filter_callbacks_encode_trailers(CALLBACKS(self), trailers);
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
on_reset_stream_i(RpStreamCallbacks* self, RpStreamResetReason_e reason, const char* transport_failure_reason)
{
    NOISY_MSG_("(%p, %d, %p(%s))", self, reason, transport_failure_reason, transport_failure_reason);
    RpCodecBridge* me = RP_CODEC_BRIDGE(self);
    if (rp_upstream_codec_filter_calling_encode_headers_(me->m_filter))
    {
//TODO...
        return;
    }

    //TODO...
    rp_stream_filter_callbacks_reset_stream(STREAM_FILTER_CALLBACKS(self), reason, transport_failure_reason);
}

static void
stream_callbacks_iface_init(RpStreamCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_reset_stream = on_reset_stream_i;
}

static RpRoute*
route_i(RpUpstreamToDownstream* self)
{
    NOISY_MSG_("(%p)", self);
NOISY_MSG_("filter %p", RP_CODEC_BRIDGE(self)->m_filter);
NOISY_MSG_("callbacks %p", STREAM_FILTER_CALLBACKS(self));
    return rp_stream_filter_callbacks_route(STREAM_FILTER_CALLBACKS(self));
}

static RpNetworkConnection*
connection_i(RpUpstreamToDownstream* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_stream_filter_callbacks_connection(STREAM_FILTER_CALLBACKS(self));
}

static RpHttpConnPoolInstStreamOptions
upstream_stream_options_i(RpUpstreamToDownstream* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_upstream_stream_filter_callbacks_upstream_stream_options(
                rp_stream_filter_callbacks_upstream_callbacks(STREAM_FILTER_CALLBACKS(self)));
}

static void
upstream_to_downstream_iface_init(RpUpstreamToDownstreamInterface* iface)
{
    LOGD("(%p)", iface);
    iface->route = route_i;
    iface->connection = connection_i;
    iface->upstream_stream_options = upstream_stream_options_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_FILTER:
            g_value_set_object(value, RP_CODEC_BRIDGE(obj)->m_filter);
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
        case PROP_FILTER:
            RP_CODEC_BRIDGE(obj)->m_filter = g_value_get_object(value);
NOISY_MSG_("filter %p", RP_CODEC_BRIDGE(obj)->m_filter);
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
    G_OBJECT_CLASS(rp_codec_bridge_parent_class)->dispose(obj);
}

static void
rp_codec_bridge_class_init(RpCodecBridgeClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_FILTER] = g_param_spec_object("filter",
                                                    "Filter",
                                                    "UpstreamCodecFilter Instance",
                                                    RP_TYPE_UPSTREAM_CODEC_FILTER,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_codec_bridge_init(RpCodecBridge* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_seen_1xx_headers = false;
}

RpCodecBridge*
rp_codec_bridge_new(RpUpstreamCodeFilter* filter)
{
    LOGD("(%p)", filter);
    g_return_val_if_fail(RP_IS_UPSTREAM_CODEC_FILTER(filter), NULL);
    return g_object_new(RP_TYPE_CODEC_BRIDGE, "filter", filter, NULL);
}
