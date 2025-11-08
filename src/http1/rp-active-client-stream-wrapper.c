/*
 * rp-active-client-stream-wrapper.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_active_client_stream_wrapper_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_active_client_stream_wrapper_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-header-utility.h"
#include "rp-request-encoder-wrapper.h"
#include "http1/rp-active-client-stream-wrapper.h"

struct _RpActiveClientStreamWrapper {
    GObject parent_instance;

    RpHttp1CpActiveClient* m_parent;

    UNIQUE_PTR(RpResponseDecoderWrapper) m_response_decoder;
    UNIQUE_PTR(RpRequestEncoderWrapper) m_request_encoder;

    bool m_wait_encode_complete : 1;
    bool m_encode_complete : 1;
    bool m_decode_complete : 1;
    bool m_close_connection : 1;
};

static void stream_callbacks_iface_init(RpStreamCallbacksInterface* iface);
static void response_decoder_iface_init(RpResponseDecoderInterface* iface);
static void stream_decoder_iface_init(RpStreamDecoderInterface* iface);
static void request_encoder_iface_init(RpRequestEncoderInterface* iface);
static void stream_encoder_iface_init(RpStreamEncoderInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpActiveClientStreamWrapper, rp_active_client_stream_wrapper, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_CALLBACKS, stream_callbacks_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_DECODER, stream_decoder_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_RESPONSE_DECODER, response_decoder_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_ENCODER, stream_encoder_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_REQUEST_ENCODER, request_encoder_iface_init)
)

static void
on_above_write_buffer_high_watermark_i(RpStreamCallbacks* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static void
on_below_write_buffer_low_watermark_i(RpStreamCallbacks* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static void
on_reset_stream_i(RpStreamCallbacks* self, RpStreamResetReason_e reason G_GNUC_UNUSED, const char* reason_str G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %d, %p(%s))", self, reason, reason_str, reason_str);
    RpActiveClientStreamWrapper* me = RP_ACTIVE_CLIENT_STREAM_WRAPPER(self);
    RpCodecClient* codec_client_ = rp_http_conn_pool_base_active_client_codec_client_(RP_HTTP_CONN_POOL_BASE_ACTIVE_CLIENT(me->m_parent));
    rp_codec_client_close_(codec_client_);
}

static void
stream_callbacks_iface_init(RpStreamCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_above_write_buffer_high_watermark = on_above_write_buffer_high_watermark_i;
    iface->on_below_write_buffer_low_watermark = on_below_write_buffer_low_watermark_i;
    iface->on_reset_stream = on_reset_stream_i;
}

static void
decode_data_i(RpStreamDecoder* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuf_length(data), end_stream);
    RpActiveClientStreamWrapper* me = RP_ACTIVE_CLIENT_STREAM_WRAPPER(self);
    rp_stream_decoder_decode_data(RP_STREAM_DECODER(me->m_response_decoder), data, end_stream);
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
    RpActiveClientStreamWrapper* me = RP_ACTIVE_CLIENT_STREAM_WRAPPER(self);
    rp_response_decoder_decode_1xx_headers(RP_RESPONSE_DECODER(me->m_response_decoder), response_headers);
}

static void
decode_headers_i(RpResponseDecoder* self, evhtp_headers_t* response_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, response_headers, end_stream);
    RpActiveClientStreamWrapper* me = RP_ACTIVE_CLIENT_STREAM_WRAPPER(self);
    RpCodecClient* codec_client_ = rp_http_conn_pool_base_active_client_codec_client_(RP_HTTP_CONN_POOL_BASE_ACTIVE_CLIENT(me->m_parent));
    me->m_close_connection = rp_header_utility_should_close_connection(rp_codec_client_protocol(codec_client_), response_headers);
    rp_response_decoder_decode_headers(RP_RESPONSE_DECODER(me->m_response_decoder), response_headers, end_stream);
}

static void
decode_trailers_i(RpResponseDecoder* self G_GNUC_UNUSED, evhtp_headers_t* trailers G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
}

static void
response_decoder_iface_init(RpResponseDecoderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->decode_1xx_headers = decode_1xx_headers_i;
    iface->decode_headers = decode_headers_i;
    iface->decode_trailers = decode_trailers_i;
}

static RpStream*
get_stream_i(RpStreamEncoder* self)
{
    NOISY_MSG_("(%p)", self);
    RpActiveClientStreamWrapper* me = RP_ACTIVE_CLIENT_STREAM_WRAPPER(self);
    return rp_stream_encoder_get_stream(RP_STREAM_ENCODER(me->m_request_encoder));
}

static void
stream_encoder_iface_init(RpStreamEncoderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->get_stream = get_stream_i;
}

static RpStatusCode_e
encode_headers_i(RpRequestEncoder* self, evhtp_headers_t* request_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, request_headers, end_stream);
    RpActiveClientStreamWrapper* me = RP_ACTIVE_CLIENT_STREAM_WRAPPER(self);
    return rp_request_encoder_encode_headers(RP_REQUEST_ENCODER(me->m_request_encoder), request_headers, end_stream);
}

static void
encode_trailers_i(RpRequestEncoder* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    RpActiveClientStreamWrapper* me = RP_ACTIVE_CLIENT_STREAM_WRAPPER(self);
    rp_request_encoder_encode_trailers(RP_REQUEST_ENCODER(me->m_request_encoder), trailers);
}

static void
request_encoder_iface_init(RpRequestEncoderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->encode_headers = encode_headers_i;
    iface->encode_trailers = encode_trailers_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpActiveClientStreamWrapper* self = RP_ACTIVE_CLIENT_STREAM_WRAPPER(obj);
    RpHttpConnPoolImplBase* base = rp_http1_cp_active_client_parent(self->m_parent);
    rp_conn_pool_impl_base_on_stream_closed(RP_CONN_POOL_IMPL_BASE(base), RP_CONNECTION_POOL_ACTIVE_CLIENT(self->m_parent), true);
    g_clear_object(&self->m_response_decoder);
    g_clear_object(&self->m_request_encoder);

    G_OBJECT_CLASS(rp_active_client_stream_wrapper_parent_class)->dispose(obj);
}

static void
rp_active_client_stream_wrapper_class_init(RpActiveClientStreamWrapperClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_active_client_stream_wrapper_init(RpActiveClientStreamWrapper* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static void
on_pre_decode_complete(GObject* obj G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", obj);
}

static void
on_decode_complete(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    RpActiveClientStreamWrapper* self = RP_ACTIVE_CLIENT_STREAM_WRAPPER(obj);
    self->m_decode_complete = true;

    RpCodecClient* codec_client_ = rp_http_conn_pool_base_active_client_codec_client_(RP_HTTP_CONN_POOL_BASE_ACTIVE_CLIENT(self->m_parent));
    if (!self->m_encode_complete)
    {
        LOGD("response before request complete");
        rp_codec_client_close_(codec_client_);
    }
    else if (self->m_close_connection || rp_codec_client_remote_closed(codec_client_))
    {
        LOGD("saw upstream close connection");
        rp_codec_client_close_(codec_client_);
    }
    else
    {
        RpConnPoolImplBase* base = RP_CONN_POOL_IMPL_BASE(rp_http1_cp_active_client_parent(self->m_parent));
        rp_conn_pool_impl_base_schedule_on_upstream_ready(base);
        rp_http1_cp_active_client_stream_wrapper_reset(self->m_parent);

        rp_conn_pool_impl_base_check_for_idle_and_close_idle_conns_if_draining(base);
    }
}

static void
on_encode_complete(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    RP_ACTIVE_CLIENT_STREAM_WRAPPER(obj)->m_encode_complete = true;
}

static inline RpActiveClientStreamWrapper*
constructed(RpActiveClientStreamWrapper* self, RpResponseDecoder* response_decoder)
{
    NOISY_MSG_("(%p, %p)", self, response_decoder);

    self->m_response_decoder = rp_response_decoder_wrapper_new(response_decoder,
                                                                on_pre_decode_complete,
                                                                on_decode_complete,
                                                                G_OBJECT(self));
    RpCodecClient* codec_client_ = rp_http_conn_pool_base_active_client_codec_client_(RP_HTTP_CONN_POOL_BASE_ACTIVE_CLIENT(self->m_parent));
    RpRequestEncoder* request_encoder = rp_codec_client_new_stream(codec_client_, RP_RESPONSE_DECODER(self));
    self->m_request_encoder = rp_request_encoder_wrapper_new(request_encoder,
                                                                on_encode_complete,
                                                                G_OBJECT(self));
    rp_stream_add_callbacks(
        rp_stream_encoder_get_stream(RP_STREAM_ENCODER(request_encoder)), RP_STREAM_CALLBACKS(self));
    self->m_wait_encode_complete = false;
    self->m_encode_complete = false;
    self->m_decode_complete = false;
    return self;
}

RpActiveClientStreamWrapper*
rp_active_client_stream_wrapper_new(RpResponseDecoder* response_decoder, RpHttp1CpActiveClient* parent)
{
    LOGD("(%p, %p)", response_decoder, parent);
    g_return_val_if_fail(RP_IS_RESPONSE_DECODER(response_decoder), NULL);
    g_return_val_if_fail(RP_IS_HTTP1_CP_ACTIVE_CLIENT(parent), NULL);
    RpActiveClientStreamWrapper* self = g_object_new(RP_TYPE_ACTIVE_CLIENT_STREAM_WRAPPER, NULL);
    self->m_parent = parent;
    return constructed(self, response_decoder);
}

bool
rp_active_client_stream_wrapper_decode_complete_(RpActiveClientStreamWrapper* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_ACTIVE_CLIENT_STREAM_WRAPPER(self), false);
    return self->m_decode_complete;
}
