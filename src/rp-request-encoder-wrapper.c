/*
 * rp-request-encoder-wrapper.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_request_encoder_wrapper_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_request_encoder_wrapper_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-request-encoder-wrapper.h"

struct _RpRequestEncoderWrapper {
    GObject parent_instance;

    RpRequestEncoder* m_inner_encoder;
    void (*m_on_encode_complete)(GObject*);
    GObject* m_arg;
};

static void stream_encoder_iface_init(RpStreamEncoderInterface* iface);
static void request_encoder_iface_init(RpRequestEncoderInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpRequestEncoderWrapper, rp_request_encoder_wrapper, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_ENCODER, stream_encoder_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_REQUEST_ENCODER, request_encoder_iface_init)
)

static inline void
on_encode_complete(RpRequestEncoderWrapper* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_on_encode_complete(self->m_arg);
}

static void
encode_data_i(RpStreamEncoder* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuf_length(data), end_stream);
    RpRequestEncoderWrapper* me = RP_REQUEST_ENCODER_WRAPPER(self);
    rp_stream_encoder_encode_data(RP_STREAM_ENCODER(me->m_inner_encoder), data, end_stream);
    if (end_stream)
    {
        on_encode_complete(RP_REQUEST_ENCODER_WRAPPER(self));
    }
}

static RpStream*
get_stream_i(RpStreamEncoder* self)
{
    NOISY_MSG_("(%p)", self);
    RpRequestEncoderWrapper* me = RP_REQUEST_ENCODER_WRAPPER(self);
    return rp_stream_encoder_get_stream(RP_STREAM_ENCODER(me->m_inner_encoder));
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
    RpRequestEncoderWrapper* me = RP_REQUEST_ENCODER_WRAPPER(self);
    rp_request_encoder_encode_headers(me->m_inner_encoder, request_headers, end_stream);
    if (end_stream)
    {
        on_encode_complete(me);
    }
    return RpStatusCode_Ok;
}

static void
encode_trailers_i(RpRequestEncoder* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("%p, %p", self, trailers);
    RpRequestEncoderWrapper* me = RP_REQUEST_ENCODER_WRAPPER(self);
    rp_request_encoder_encode_trailers(me->m_inner_encoder, trailers);
    on_encode_complete(me);
}

static void
enable_tcp_tunneling_i(RpRequestEncoder* self)
{
    NOISY_MSG_("(%p)", self);
    RpRequestEncoderWrapper* me = RP_REQUEST_ENCODER_WRAPPER(self);
    rp_request_encoder_enable_tcp_tunneling(me->m_inner_encoder);
}

static void
request_encoder_iface_init(RpRequestEncoderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->encode_headers = encode_headers_i;
    iface->encode_trailers = encode_trailers_i;
    iface->enable_tcp_tunneling = enable_tcp_tunneling_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpRequestEncoderWrapper* self = RP_REQUEST_ENCODER_WRAPPER(obj);
    g_clear_object(&self->m_inner_encoder);

    G_OBJECT_CLASS(rp_request_encoder_wrapper_parent_class)->dispose(obj);
}

static void
rp_request_encoder_wrapper_class_init(RpRequestEncoderWrapperClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_request_encoder_wrapper_init(RpRequestEncoderWrapper* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpRequestEncoderWrapper*
rp_request_encoder_wrapper_new(RpRequestEncoder* inner, void (*on_encode_complete)(GObject*), GObject* arg)
{
    LOGD("(%p, %p, %p)", inner, on_encode_complete, arg);
    g_return_val_if_fail(on_encode_complete != NULL, NULL);
    g_return_val_if_fail(G_IS_OBJECT(arg), NULL);
    RpRequestEncoderWrapper* self = g_object_new(RP_TYPE_REQUEST_ENCODER_WRAPPER, NULL);
    self->m_inner_encoder = inner ? g_object_ref(inner) : inner;
    self->m_on_encode_complete = on_encode_complete;
    self->m_arg = arg;
    return self;
}

void
rp_request_encoder_wrapper_set_inner_encoder_(RpRequestEncoderWrapper* self, RpRequestEncoder* inner)
{
    LOGD("(%p, %p)", self, inner);
    g_return_if_fail(RP_IS_REQUEST_ENCODER_WRAPPER(self));
    g_return_if_fail(RP_IS_REQUEST_ENCODER(inner));
    self->m_inner_encoder = g_object_ref(inner);
}
