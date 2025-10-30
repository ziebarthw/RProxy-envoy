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

typedef struct _RpResponseDecoderWrapperPrivate RpResponseDecoderWrapperPrivate;
struct _RpResponseDecoderWrapperPrivate {

    RpResponseDecoder* m_inner;

};

enum
{
    PROP_0, // Reserved.
    PROP_INNER,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void stream_decoder_iface_init(RpStreamDecoderInterface* iface);
static void response_decoder_iface_init(RpResponseDecoderInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpResponseDecoderWrapper, rp_response_decoder_wrapper, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpResponseDecoderWrapper)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_DECODER, stream_decoder_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_RESPONSE_DECODER, response_decoder_iface_init)
)

#define PRIV(obj) \
    ((RpResponseDecoderWrapperPrivate*) rp_response_decoder_wrapper_get_instance_private(RP_RESPONSE_DECODER_WRAPPER(obj)))

static inline void
on_pre_decoder_complete(RpResponseDecoderWrapper* self)
{
    NOISY_MSG_("(%p)", self);
    if (RP_RESPONSE_DECODER_WRAPPER_GET_CLASS(self)->on_pre_decode_complete) \
        RP_RESPONSE_DECODER_WRAPPER_GET_CLASS(self)->on_pre_decode_complete(self);
}

static inline void
on_decode_complete(RpResponseDecoderWrapper* self)
{
    NOISY_MSG_("(%p)", self);
    if (RP_RESPONSE_DECODER_WRAPPER_GET_CLASS(self)->on_decode_complete) \
        RP_RESPONSE_DECODER_WRAPPER_GET_CLASS(self)->on_decode_complete(self);
}

static void
decode_data_i(RpStreamDecoder* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    if (end_stream)
    {
        on_pre_decoder_complete(RP_RESPONSE_DECODER_WRAPPER(self));
    }
    rp_stream_decoder_decode_data(RP_STREAM_DECODER(PRIV(self)->m_inner), data, end_stream);
    if (end_stream)
    {
        on_decode_complete(RP_RESPONSE_DECODER_WRAPPER(self));
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
    rp_response_decoder_decode_1xx_headers(PRIV(self)->m_inner, response_headers);
}

static void
decode_headers_i(RpResponseDecoder* self, evhtp_headers_t* response_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, response_headers, end_stream);
    if (end_stream)
    {
        on_pre_decoder_complete(RP_RESPONSE_DECODER_WRAPPER(self));
    }
    rp_response_decoder_decode_headers(PRIV(self)->m_inner, response_headers, end_stream);
    if (end_stream)
    {
        on_decode_complete(RP_RESPONSE_DECODER_WRAPPER(self));
    }
}

static void
decode_trailers_i(RpResponseDecoder* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    on_pre_decoder_complete(RP_RESPONSE_DECODER_WRAPPER(self));
    rp_response_decoder_decode_trailers(PRIV(self)->m_inner, trailers);
    on_decode_complete(RP_RESPONSE_DECODER_WRAPPER(self));
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
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_INNER:
            g_value_set_object(value, PRIV(obj)->m_inner);
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
        case PROP_INNER:
            PRIV(obj)->m_inner = g_value_get_object(value);
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

#if 0
RpResponseDecoderWrapperPrivate* me = PRIV(obj);
NOISY_MSG_("calling g_clear_object(%p(%p))", &me->m_inner, me->m_inner);
g_clear_object(&me->m_inner);
NOISY_MSG_("back");
#endif//0

    G_OBJECT_CLASS(rp_response_decoder_wrapper_parent_class)->dispose(obj);
}

static void
rp_response_decoder_wrapper_class_init(RpResponseDecoderWrapperClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_INNER] = g_param_spec_object("inner",
                                                    "Inner",
                                                    "Wrapped ResponseDecoder Instance",
                                                    RP_TYPE_RESPONSE_DECODER,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_response_decoder_wrapper_init(RpResponseDecoderWrapper* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}
