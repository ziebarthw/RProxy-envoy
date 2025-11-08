/*
 * rp-response-decoder-wrapper.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_response_decoder_wrapper_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_response_decoder_wrapper_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-response-decoder-wrapper.h"

struct _RpResponseDecoderWrapper {
    GObject parent_instance;

    RpResponseDecoder* m_inner;
    void (*m_on_pre_decode_complete)(GObject*);
    void (*m_on_decode_complete)(GObject*);
    GObject* m_arg;
};

static void stream_decoder_iface_init(RpStreamDecoderInterface* iface);
static void response_decoder_iface_init(RpResponseDecoderInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpResponseDecoderWrapper, rp_response_decoder_wrapper, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_DECODER, stream_decoder_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_RESPONSE_DECODER, response_decoder_iface_init)
)

#define PRIV(obj) \
    ((RpResponseDecoderWrapperPrivate*) rp_response_decoder_wrapper_get_instance_private(RP_RESPONSE_DECODER_WRAPPER(obj)))

static inline void
on_pre_decoder_complete(RpResponseDecoderWrapper* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_on_pre_decode_complete(self->m_arg);
}

static inline void
on_decode_complete(RpResponseDecoderWrapper* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_on_decode_complete(self->m_arg);
}

static void
decode_data_i(RpStreamDecoder* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuf_length(data), end_stream);
    RpResponseDecoderWrapper* me = RP_RESPONSE_DECODER_WRAPPER(self);
    if (end_stream)
    {
        on_pre_decoder_complete(me);
    }
    rp_stream_decoder_decode_data(RP_STREAM_DECODER(me->m_inner), data, end_stream);
    if (end_stream)
    {
        on_decode_complete(me);
    }
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
    RpResponseDecoderWrapper* me = RP_RESPONSE_DECODER_WRAPPER(self);
    rp_response_decoder_decode_1xx_headers(me->m_inner, response_headers);
}

static void
decode_headers_i(RpResponseDecoder* self, evhtp_headers_t* response_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, response_headers, end_stream);
    RpResponseDecoderWrapper* me = RP_RESPONSE_DECODER_WRAPPER(self);
    if (end_stream)
    {
        on_pre_decoder_complete(me);
    }
    rp_response_decoder_decode_headers(me->m_inner, response_headers, end_stream);
    if (end_stream)
    {
        on_decode_complete(RP_RESPONSE_DECODER_WRAPPER(self));
    }
}

static void
decode_trailers_i(RpResponseDecoder* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    RpResponseDecoderWrapper* me = RP_RESPONSE_DECODER_WRAPPER(self);
    on_pre_decoder_complete(me);
    rp_response_decoder_decode_trailers(me->m_inner, trailers);
    on_decode_complete(me);
}

static void
response_decoder_iface_init(RpResponseDecoderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->decode_1xx_headers = decode_1xx_headers_i;
    iface->decode_headers = decode_headers_i;
    iface->decode_trailers = decode_trailers_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_response_decoder_wrapper_parent_class)->dispose(obj);
}

static void
rp_response_decoder_wrapper_class_init(RpResponseDecoderWrapperClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_response_decoder_wrapper_init(RpResponseDecoderWrapper* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpResponseDecoderWrapper*
rp_response_decoder_wrapper_new(RpResponseDecoder* inner, void (*on_pre_decode_complete)(GObject*), void (*on_decode_complete)(GObject*), GObject* arg)
{
    LOGD("(%p, %p, %p, %p)", inner, on_pre_decode_complete, on_decode_complete, arg);
    g_return_val_if_fail(RP_IS_RESPONSE_DECODER(inner), NULL);
    g_return_val_if_fail(on_pre_decode_complete != NULL, NULL);
    g_return_val_if_fail(on_decode_complete != NULL, NULL);
    g_return_val_if_fail(arg != NULL, NULL);
    RpResponseDecoderWrapper* self = g_object_new(RP_TYPE_RESPONSE_DECODER_WRAPPER, NULL);
    self->m_inner = inner;
    self->m_on_pre_decode_complete = on_pre_decode_complete;
    self->m_on_decode_complete = on_decode_complete;
    self->m_arg = arg;
    return self;
}
