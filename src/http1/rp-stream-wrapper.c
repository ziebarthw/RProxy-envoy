/*
 * rp-stream-wrapper.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_stream_wrapper_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_stream_wrapper_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-header-utility.h"
#include "rp-response-decoder-wrapper.h"
#include "http1/rp-http1-conn-pool.h"
#include "http1/rp-http1-request-encoder-wrapper.h"
#include "http1/rp-stream-wrapper.h"

struct _RpHttp1StreamWrapper {
    RpResponseDecoderWrapper parent_instance;

    RpHttp1CpActiveClient* m_parent;
    RpResponseDecoder* m_response_decoder;
    RpHttp1RequestEncoderWrapper* m_encoder_wrapper;

    bool m_stream_incomplete : 1;
    bool m_encode_complete : 1;
    bool m_decode_complete : 1;
    bool m_close_connection : 1;
};

enum
{
    PROP_0, // Reserved.
    PROP_PARENT,
    PROP_RESPONSE_DECODER,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void stream_callbacks_iface_init(RpStreamCallbacksInterface* iface);
static void response_decoder_iface_init(RpResponseDecoderInterface* iface);
static void stream_encoder_iface_init(RpStreamEncoderInterface* iface);
static void request_encoder_iface_init(RpRequestEncoderInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpHttp1StreamWrapper, rp_http1_stream_wrapper, RP_TYPE_RESPONSE_DECODER_WRAPPER,
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_CALLBACKS, stream_callbacks_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_RESPONSE_DECODER, response_decoder_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_ENCODER, stream_encoder_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_REQUEST_ENCODER, request_encoder_iface_init)
)

static inline RpResponseDecoderInterface*
parent_iface(RpResponseDecoder* self)
{
    return (RpResponseDecoderInterface*)g_type_interface_peek_parent(RP_RESPONSE_DECODER_GET_IFACE(self));
}

static inline RpCodecClient*
get_codec_client(RpHttp1StreamWrapper* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_http_conn_pool_base_active_client_codec_client_(RP_HTTP_CONN_POOL_BASE_ACTIVE_CLIENT(self->m_parent));
}

static void
on_reset_stream_i(RpStreamCallbacks* self, RpStreamResetReason_e reason, const char* reason_str)
{
    NOISY_MSG_("(%p, %d, %p(%s))", self, reason, reason_str, reason_str);
    rp_codec_client_close_(get_codec_client(RP_HTTP1_STREAM_WRAPPER(self)));
}

static void
stream_callbacks_iface_init(RpStreamCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_reset_stream = on_reset_stream_i;
}

static void
encode_data_i(RpStreamEncoder* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)",
        self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    RpHttp1StreamWrapper* me = RP_HTTP1_STREAM_WRAPPER(self);
    rp_stream_encoder_encode_data(RP_STREAM_ENCODER(me->m_encoder_wrapper), data, end_stream);
}

static RpStream*
get_stream_i(RpStreamEncoder* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttp1StreamWrapper* me = RP_HTTP1_STREAM_WRAPPER(self);
    return rp_stream_encoder_get_stream(RP_STREAM_ENCODER(me->m_encoder_wrapper));
}

static void
stream_encoder_iface_init(RpStreamEncoderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->encode_data = encode_data_i;
    iface->get_stream = get_stream_i;
}

static void
decode_1xx_headers_i(RpResponseDecoder* self, evhtp_headers_t* response_headers)
{
    NOISY_MSG_("(%p, %p)", self, response_headers);
    parent_iface(self)->decode_1xx_headers(self, response_headers);
}

static void
decode_headers_i(RpResponseDecoder* self, evhtp_headers_t* response_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, response_headers, end_stream);
    RpHttp1StreamWrapper* me = RP_HTTP1_STREAM_WRAPPER(self);
    evhtp_proto protocol = rp_codec_client_protocol(get_codec_client(me));
    me->m_close_connection = rp_header_utility_should_close_connection(protocol, response_headers);
    if (me->m_close_connection)
    {
        NOISY_MSG_("should close connection");
        //TODO...parent_.parent().host()->cluster().trafficStats()...
    }
    parent_iface(self)->decode_headers(self, response_headers, end_stream);
}

static void
decode_trailers_i(RpResponseDecoder* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    parent_iface(self)->decode_trailers(self, trailers);
}

static void
response_decoder_iface_init(RpResponseDecoderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->decode_1xx_headers = decode_1xx_headers_i;
    iface->decode_headers = decode_headers_i;
    iface->decode_trailers = decode_trailers_i;
}

static RpStatusCode_e
encode_headers_i(RpRequestEncoder* self, evhtp_headers_t* request_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, request_headers, end_stream);
    RpHttp1StreamWrapper* me = RP_HTTP1_STREAM_WRAPPER(self);
    return rp_request_encoder_encode_headers(RP_REQUEST_ENCODER(me->m_encoder_wrapper), request_headers, end_stream);
}

static void
encode_trailers_i(RpRequestEncoder* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    RpHttp1StreamWrapper* me = RP_HTTP1_STREAM_WRAPPER(self);
    rp_request_encoder_encode_trailers(RP_REQUEST_ENCODER(me->m_encoder_wrapper), trailers);
}

static void
request_encoder_iface_init(RpRequestEncoderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->encode_headers = encode_headers_i;
    iface->encode_trailers = encode_trailers_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_PARENT:
            g_value_set_object(value, RP_HTTP1_STREAM_WRAPPER(obj)->m_parent);
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
            RP_HTTP1_STREAM_WRAPPER(obj)->m_parent = g_value_get_object(value);
            break;
        case PROP_RESPONSE_DECODER:
            RP_HTTP1_STREAM_WRAPPER(obj)->m_response_decoder = g_value_get_object(value);
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

    G_OBJECT_CLASS(rp_http1_stream_wrapper_parent_class)->constructed(obj);

    RpHttp1StreamWrapper* self = RP_HTTP1_STREAM_WRAPPER(obj);

    RpCodecClient* codec_client = get_codec_client(self);
    RpRequestEncoder* request_encoder = rp_codec_client_new_stream(codec_client, RP_RESPONSE_DECODER(self));
    self->m_encoder_wrapper = rp_http1_request_encoder_wrapper_new(self, request_encoder);

    rp_stream_add_callbacks(
        rp_stream_encoder_get_stream(RP_STREAM_ENCODER(request_encoder)), RP_STREAM_CALLBACKS(self));
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpHttp1StreamWrapper* me = RP_HTTP1_STREAM_WRAPPER(obj);
    rp_conn_pool_impl_base_on_stream_closed(RP_CONN_POOL_IMPL_BASE(
        rp_http1_cp_active_client_parent(me->m_parent)), RP_CONNECTION_POOL_ACTIVE_CLIENT(me->m_parent), true);

    G_OBJECT_CLASS(rp_http1_stream_wrapper_parent_class)->dispose(obj);
}

OVERRIDE void
on_decode_complete(RpResponseDecoderWrapper* self)
{
    NOISY_MSG_("(%p)", self);

    RpHttp1StreamWrapper* me = RP_HTTP1_STREAM_WRAPPER(self);
    g_assert(!me->m_decode_complete);

    me->m_decode_complete = me->m_encode_complete;
    NOISY_MSG_("response complete");

    if (!me->m_encode_complete)
    {
        LOGD("response before request complete");
        rp_codec_client_close_(get_codec_client(me));
    }
    else if (me->m_close_connection || rp_codec_client_remote_closed(get_codec_client(me)))
    {
        LOGD("saw upstream close connection");
        rp_codec_client_close_(get_codec_client(me));
    }
    else
    {
        RpConnPoolImplBase* pool = RP_CONN_POOL_IMPL_BASE(rp_http1_cp_active_client_parent(me->m_parent));
        rp_conn_pool_impl_base_schedule_on_upstream_ready(pool);
        rp_http1_cp_active_client_stream_wrapper_reset(me->m_parent);

        rp_conn_pool_impl_base_check_for_idle_and_close_idle_conns_if_draining(pool);
    }
}

OVERRIDE void
on_pre_decode_complete(RpResponseDecoderWrapper* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static void
response_decoder_wrapper_class_init(RpResponseDecoderWrapperClass* klass)
{
    LOGD("(%p)", klass);
    klass->on_decode_complete = on_decode_complete;
    klass->on_pre_decode_complete = on_pre_decode_complete;
}

static void
rp_http1_stream_wrapper_class_init(RpHttp1StreamWrapperClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    response_decoder_wrapper_class_init(RP_RESPONSE_DECODER_WRAPPER_CLASS(klass));

    obj_properties[PROP_PARENT] = g_param_spec_object("parent",
                                                    "Parent",
                                                    "Parent ActiveClient Instance",
                                                    RP_TYPE_HTTP1_CP_ACTIVE_CLIENT,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_RESPONSE_DECODER] = g_param_spec_object("response-decoder",
                                                    "Response decoder",
                                                    "ResponseDecoder Instance",
                                                    RP_TYPE_RESPONSE_DECODER,
                                                    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_http1_stream_wrapper_init(RpHttp1StreamWrapper* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpHttp1StreamWrapper*
rp_http1_stream_wrapper_new(RpResponseDecoder* response_decoder, RpHttp1CpActiveClientPtr parent)
{
    LOGD("(%p, %p)", response_decoder, parent);
    g_return_val_if_fail(RP_IS_RESPONSE_DECODER(response_decoder), NULL);
    g_return_val_if_fail(RP_IS_HTTP1_CP_ACTIVE_CLIENT(parent), NULL);
    return g_object_new(RP_TYPE_HTTP1_STREAM_WRAPPER,
                        "response-decoder", response_decoder,
                        "inner", response_decoder,
                        "parent", parent,
                        NULL);
}

void
rp_http1_stream_wrapper_encode_complete(RpHttp1StreamWrapper* self)
{
    LOGD("(%p)", self);
    self->m_encode_complete = true;
}

bool
rp_http1_stream_wrapper_decode_complete_(RpHttp1StreamWrapper* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP1_STREAM_WRAPPER(self), true);
    return self->m_decode_complete;
}
