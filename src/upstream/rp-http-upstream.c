/*
 * rp-http-upstream.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_http_upstream_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_http_upstream_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-router.h"
#include "upstream/rp-http-upstream.h"

typedef struct _RpHttpUpstreamPrivate RpHttpUpstreamPrivate;
struct _RpHttpUpstreamPrivate {

    RpUpstreamToDownstream* m_upstream_request;
    RpRequestEncoder* m_request_encoder;
};

enum
{
    PROP_0, // Reserved.
    PROP_UPSTREAM_REQUEST,
    PROP_REQUEST_ENCODER,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void generic_upstream_iface_init(RpGenericUpstreamInterface* iface);
static void stream_callbacks_iface_init(RpStreamCallbacksInterface* iface);

// https://github.com/envoyproxy/envoy/blob/main/source/extensions/upstreams/http/http/upstream_request.h
// class HttpUpstream : public Router::GenericUpstream, public Envoy::Http::StreamCallbacks

G_DEFINE_TYPE_WITH_CODE(RpHttpUpstream, rp_http_upstream, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpHttpUpstream)
    G_IMPLEMENT_INTERFACE(RP_TYPE_GENERIC_UPSTREAM, generic_upstream_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_CALLBACKS, stream_callbacks_iface_init)
)

#define PRIV(obj) \
    ((RpHttpUpstreamPrivate*) rp_http_upstream_get_instance_private(RP_HTTP_UPSTREAM(obj)))

#define REQUEST_ENCODER(s) \
    PRIV(s)->m_request_encoder
#define STREAM_ENCODER(s) \
    RP_STREAM_ENCODER(REQUEST_ENCODER(s))
#define STREAM_CALLBACKS(s) \
    RP_STREAM_CALLBACKS(REQUEST_ENCODER(s))

static void
encode_data_i(RpGenericUpstream* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuf_length(data), end_stream);
    rp_stream_encoder_encode_data(STREAM_ENCODER(self), data, end_stream);
}

static RpStatusCode_e
encode_headers_i(RpGenericUpstream* self, evhtp_headers_t* request_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, request_headers, end_stream);
    return rp_request_encoder_encode_headers(REQUEST_ENCODER(self), request_headers, end_stream);
}

static void
encode_trailers_i(RpGenericUpstream* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    rp_request_encoder_encode_trailers(REQUEST_ENCODER(self), trailers);
}

static void
read_disable_i(RpGenericUpstream* self, bool disable)
{
    NOISY_MSG_("(%p, %u)", self, disable);
    RpStream* stream = rp_stream_encoder_get_stream(STREAM_ENCODER(self));
    rp_stream_read_disable(stream, disable);
}

static void
reset_stream_i(RpGenericUpstream* self)
{
    NOISY_MSG_("(%p)", self);
    RpStream* stream = rp_stream_encoder_get_stream(STREAM_ENCODER(self));
    rp_stream_remove_callbacks(stream, RP_STREAM_CALLBACKS(self));
    rp_stream_reset_handler_reset_stream(RP_STREAM_RESET_HANDLER(stream), RpStreamResetReason_LocalReset);
}

static void
generic_upstream_iface_init(RpGenericUpstreamInterface* iface)
{
    LOGD("(%p)", iface);
    iface->encode_data = encode_data_i;
    iface->encode_headers = encode_headers_i;
    iface->encode_trailers = encode_trailers_i;
    iface->read_disable = read_disable_i;
    iface->reset_stream = reset_stream_i;
}

static void
on_reset_stream_i(RpStreamCallbacks* self, RpStreamResetReason_e reason, const char* transport_failure_reason)
{
    NOISY_MSG_("(%p, %d, %p(%s))",
        self, reason, transport_failure_reason, transport_failure_reason);
    RpHttpUpstreamPrivate* me = PRIV(self);
    rp_stream_callbacks_on_reset_stream(RP_STREAM_CALLBACKS(me->m_upstream_request), reason, transport_failure_reason);

}

static void
on_above_write_buffer_high_watermark_i(RpStreamCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttpUpstreamPrivate* me = PRIV(self);
    rp_stream_callbacks_on_above_write_buffer_high_watermark(STREAM_CALLBACKS(me->m_upstream_request));
}

static void
on_below_write_buffer_low_watermark_i(RpStreamCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttpUpstreamPrivate* me = PRIV(self);
    rp_stream_callbacks_on_below_write_buffer_low_watermark(STREAM_CALLBACKS(me->m_upstream_request));
}

static void
stream_callbacks_iface_init(RpStreamCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_reset_stream = on_reset_stream_i;
    iface->on_above_write_buffer_high_watermark = on_above_write_buffer_high_watermark_i;
    iface->on_below_write_buffer_low_watermark = on_below_write_buffer_low_watermark_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_UPSTREAM_REQUEST:
            g_value_set_object(value, PRIV(obj)->m_upstream_request);
            break;
        case PROP_REQUEST_ENCODER:
            g_value_set_object(value, PRIV(obj)->m_request_encoder);
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
            PRIV(obj)->m_upstream_request = g_value_get_object(value);
            break;
        case PROP_REQUEST_ENCODER:
            PRIV(obj)->m_request_encoder = g_value_get_object(value);
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

    G_OBJECT_CLASS(rp_http_upstream_parent_class)->constructed(obj);

    rp_stream_add_callbacks(
        rp_stream_encoder_get_stream(STREAM_ENCODER(obj)), RP_STREAM_CALLBACKS(obj));
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_http_upstream_parent_class)->dispose(obj);
}

static void
rp_http_upstream_class_init(RpHttpUpstreamClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    obj_properties[PROP_UPSTREAM_REQUEST] = g_param_spec_object("upstream-request",
                                                    "Upstream Request",
                                                    "UpstremRequest Instance",
                                                    RP_TYPE_UPSTREAM_TO_DOWNSTREAM,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_REQUEST_ENCODER] = g_param_spec_object("request-encoder",
                                                    "Request Encoder",
                                                    "RequestEncoder Instance",
                                                    RP_TYPE_REQUEST_ENCODER,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_http_upstream_init(RpHttpUpstream* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpUpstreamToDownstream*
rp_http_upstream_upstream_request_(RpHttpUpstream* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_UPSTREAM(self), NULL);
    return PRIV(self)->m_upstream_request;
}

RpHttpUpstream*
rp_http_upstream_new(RpUpstreamToDownstream* upstream_request, RpRequestEncoder* encoder)
{
    LOGD("(%p, %p)", upstream_request, encoder);
    return g_object_new(RP_TYPE_HTTP_UPSTREAM,
                        "upstream-request", upstream_request,
                        "request-encoder", encoder,
                        NULL);
}
