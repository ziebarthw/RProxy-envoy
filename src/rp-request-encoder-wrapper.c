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

typedef struct _RpRequestEncoderWrapperPrivate RpRequestEncoderWrapperPrivate;
struct _RpRequestEncoderWrapperPrivate {

    RpRequestEncoder* m_inner;

};

enum
{
    PROP_0, // Reserved.
    PROP_INNER,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void stream_encoder_iface_init(RpStreamEncoderInterface* iface);
static void request_encoder_iface_init(RpRequestEncoderInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpRequestEncoderWrapper, rp_request_encoder_wrapper, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpRequestEncoderWrapper)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_ENCODER, stream_encoder_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_REQUEST_ENCODER, request_encoder_iface_init)
)

#define PRIV(obj) \
    ((RpRequestEncoderWrapperPrivate*) rp_request_encoder_wrapper_get_instance_private(RP_REQUEST_ENCODER_WRAPPER(obj)))

static inline void
on_encode_complete(RpRequestEncoderWrapper* self)
{
    NOISY_MSG_("(%p)", self);
    RP_REQUEST_ENCODER_WRAPPER_GET_CLASS(self)->on_encode_complete(self);
}

static void
encode_data_i(RpStreamEncoder* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    rp_stream_encoder_encode_data(RP_STREAM_ENCODER(PRIV(self)->m_inner), data, end_stream);
    if (end_stream)
    {
        on_encode_complete(RP_REQUEST_ENCODER_WRAPPER(self));
    }
}

static RpStream*
get_stream_i(RpStreamEncoder* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_stream_encoder_get_stream(RP_STREAM_ENCODER(PRIV(self)->m_inner));
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
    rp_request_encoder_encode_headers(PRIV(self)->m_inner, request_headers, end_stream);
    if (end_stream)
    {
        on_encode_complete(RP_REQUEST_ENCODER_WRAPPER(self));
    }
    return RpStatusCode_Ok;
}

static void
encode_trailers_i(RpRequestEncoder* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("%p, %p", self, trailers);
    rp_request_encoder_encode_trailers(PRIV(self)->m_inner, trailers);
    on_encode_complete(RP_REQUEST_ENCODER_WRAPPER(self));
}

static void
enable_tcp_tunneling_i(RpRequestEncoder* self)
{
    NOISY_MSG_("(%p)", self);
    rp_request_encoder_enable_tcp_tunneling(PRIV(self)->m_inner);
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
    G_OBJECT_CLASS(rp_request_encoder_wrapper_parent_class)->dispose(obj);
}

static void
rp_request_encoder_wrapper_class_init(RpRequestEncoderWrapperClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_INNER] = g_param_spec_object("inner",
                                                    "Inner",
                                                    "Wrapped RequestEncoder Instance",
                                                    RP_TYPE_REQUEST_ENCODER,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_request_encoder_wrapper_init(RpRequestEncoderWrapper* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}
