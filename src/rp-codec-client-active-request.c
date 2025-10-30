/*
 * rp-codec-client-active-request.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_codec_client_active_request_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_codec_client_active_request_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "ar-response-decoder-wrapper.h"
#include "ar-request-encoder-wrapper.h"
#include "rp-codec-client-active-request.h"

struct _RpCodecClientActiveRequest {
    GObject parent_instance;

    RpCodecClient* m_parent;
    RpResponseDecoder* m_inner;

    ArResponseDecoderWrapper* m_response_decoder;
    ArRequestEncoderWrapper* m_request_encoder;

    bool m_wait_encode_complete : 1;
    bool m_encode_complete : 1;
    bool m_decode_complete : 1;
};

static void stream_callbacks_iface_init(RpStreamCallbacksInterface* iface);
static void stream_decoder_iface_init(RpStreamDecoderInterface* iface);
static void response_decoder_iface_init(RpResponseDecoderInterface* iface);
static void stream_encoder_iface_init(RpStreamEncoderInterface* iface);
static void request_encoder_iface_init(RpRequestEncoderInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpCodecClientActiveRequest, rp_codec_client_active_request, G_TYPE_OBJECT,
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
stream_callbacks_iface_init(RpStreamCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_above_write_buffer_high_watermark = on_above_write_buffer_high_watermark_i;
    iface->on_below_write_buffer_low_watermark = on_below_write_buffer_low_watermark_i;
}

static void
decode_data_i(RpStreamDecoder* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    RpCodecClientActiveRequest* me = RP_CODEC_CLIENT_ACTIVE_REQUEST(self);
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
    RpCodecClientActiveRequest* me = RP_CODEC_CLIENT_ACTIVE_REQUEST(self);
    rp_response_decoder_decode_1xx_headers(RP_RESPONSE_DECODER(me->m_response_decoder), response_headers);
}

static void
decode_headers_i(RpResponseDecoder* self, evhtp_headers_t* response_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, response_headers, end_stream);
    RpCodecClientActiveRequest* me = RP_CODEC_CLIENT_ACTIVE_REQUEST(self);
    rp_response_decoder_decode_headers(RP_RESPONSE_DECODER(me->m_response_decoder), response_headers, end_stream);
}

static void
decode_trailers_i(RpResponseDecoder* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    RpCodecClientActiveRequest* me = RP_CODEC_CLIENT_ACTIVE_REQUEST(self);
    rp_response_decoder_decode_trailers(RP_RESPONSE_DECODER(me->m_response_decoder), trailers);
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
encode_data_i(RpStreamEncoder* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    RpCodecClientActiveRequest* me = RP_CODEC_CLIENT_ACTIVE_REQUEST(self);
    rp_stream_encoder_encode_data(RP_STREAM_ENCODER(me->m_request_encoder), data, end_stream);
}

static RpStream*
get_stream_i(RpStreamEncoder* self)
{
    NOISY_MSG_("(%p)", self);
    RpCodecClientActiveRequest* me = RP_CODEC_CLIENT_ACTIVE_REQUEST(self);
    return rp_stream_encoder_get_stream(RP_STREAM_ENCODER(me->m_request_encoder));
}

static void
stream_encoder_iface_init(RpStreamEncoderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->encode_data = encode_data_i;
    iface->get_stream = get_stream_i;
}

static RpStatusCode_e
encode_headers_i(RpRequestEncoder* self, evhtp_headers_t* request_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, request_headers, end_stream);
    RpCodecClientActiveRequest* me = RP_CODEC_CLIENT_ACTIVE_REQUEST(self);
    return rp_request_encoder_encode_headers(RP_REQUEST_ENCODER(me->m_request_encoder), request_headers, end_stream);
}

static void
encode_trailers_i(RpRequestEncoder* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    RpCodecClientActiveRequest* me = RP_CODEC_CLIENT_ACTIVE_REQUEST(self);
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

    RpCodecClientActiveRequest* self = RP_CODEC_CLIENT_ACTIVE_REQUEST(obj);
    g_clear_object(&self->m_response_decoder);
    g_clear_object(&self->m_request_encoder);

    G_OBJECT_CLASS(rp_codec_client_active_request_parent_class)->dispose(obj);
}

static void
rp_codec_client_active_request_class_init(RpCodecClientActiveRequestClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_codec_client_active_request_init(RpCodecClientActiveRequest* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpCodecClientActiveRequest*
constructed(RpCodecClientActiveRequest* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_response_decoder = ar_response_decoder_wrapper_new(self->m_inner, self->m_parent, self);
    self->m_wait_encode_complete = false;
    self->m_encode_complete = false;
    self->m_decode_complete = false;
    return self;
}

RpCodecClientActiveRequest*
rp_codec_client_active_request_new(RpCodecClient* parent, RpResponseDecoder* inner)
{
    LOGD("(%p, %p)", parent, inner);
    g_return_val_if_fail(RP_IS_CODEC_CLIENT(parent), NULL);
    g_return_val_if_fail(RP_IS_RESPONSE_DECODER(inner), NULL);
    RpCodecClientActiveRequest* self = g_object_new(RP_TYPE_CODEC_CLIENT_ACTIVE_REQUEST, NULL);
    self->m_parent = parent;
    self->m_inner = inner;
    return constructed(self);
}

void
rp_codec_client_active_request_set_encoder(RpCodecClientActiveRequest* self,
                                                RpRequestEncoder* encoder)
{
    LOGD("(%p, %p)", self, encoder);
    g_return_if_fail(RP_IS_CODEC_CLIENT_ACTIVE_REQUEST(self));
    g_return_if_fail(RP_IS_REQUEST_ENCODER(encoder));
    self->m_request_encoder = ar_request_encoder_wrapper_new(encoder, self->m_parent, self);
}

void
rp_codec_client_active_request_set_decode_complete(RpCodecClientActiveRequest* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_CODEC_CLIENT_ACTIVE_REQUEST(self));
    self->m_decode_complete = true;
}

bool
rp_codec_client_active_request_get_decode_complete(RpCodecClientActiveRequest* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CODEC_CLIENT_ACTIVE_REQUEST(self), false);
    return self->m_decode_complete;
}

void
rp_codec_client_active_request_set_encode_complete(RpCodecClientActiveRequest* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_CODEC_CLIENT_ACTIVE_REQUEST(self));
    self->m_encode_complete = true;
}

bool
rp_codec_client_active_request_get_encode_complete(RpCodecClientActiveRequest* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CODEC_CLIENT_ACTIVE_REQUEST(self), false);
    return self->m_encode_complete;
}

bool
rp_codec_client_active_request_get_wait_encode_complete(RpCodecClientActiveRequest* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CODEC_CLIENT_ACTIVE_REQUEST(self), false);
    return self->m_wait_encode_complete;
}

void
rp_codec_client_active_request_remove_encoder_callbacks(RpCodecClientActiveRequest* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_CODEC_CLIENT_ACTIVE_REQUEST(self));
    RpStream* stream = rp_stream_encoder_get_stream(RP_STREAM_ENCODER(self->m_request_encoder));
    rp_stream_remove_callbacks(stream, RP_STREAM_CALLBACKS(self));
}
