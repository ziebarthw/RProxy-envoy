/*
 * rp-http1-request-encoder-wrapper.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_http1_request_encoder_wrapper_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_http1_request_encoder_wrapper_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "http1/rp-stream-wrapper.h"
#include "rp-http1-request-encoder-wrapper.h"

struct _RpHttp1RequestEncoderWrapper {
    RpRequestEncoderWrapper parent_instance;

    RpHttp1StreamWrapper* m_parent;
};

enum
{
    PROP_0, // Reserved.
    PROP_PARENT,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_FINAL_TYPE(RpHttp1RequestEncoderWrapper, rp_http1_request_encoder_wrapper, RP_TYPE_REQUEST_ENCODER_WRAPPER)

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_PARENT:
            g_value_set_object(value, RP_HTTP1_REQUEST_ENCODER_WRAPPER(obj)->m_parent);
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
            RP_HTTP1_REQUEST_ENCODER_WRAPPER(obj)->m_parent = g_value_get_object(value);
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
    G_OBJECT_CLASS(rp_http1_request_encoder_wrapper_parent_class)->dispose(obj);
}

OVERRIDE void
on_encode_complete(RpRequestEncoderWrapper* self)
{
    NOISY_MSG_("(%p)", self);
    rp_http1_stream_wrapper_encode_complete(RP_HTTP1_REQUEST_ENCODER_WRAPPER(self)->m_parent);
}

static void
request_encoder_wrapper_class_init(RpRequestEncoderWrapperClass* klass)
{
    LOGD("(%p)", klass);
    klass->on_encode_complete = on_encode_complete;
}

static void
rp_http1_request_encoder_wrapper_class_init(RpHttp1RequestEncoderWrapperClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    request_encoder_wrapper_class_init(RP_REQUEST_ENCODER_WRAPPER_CLASS(klass));

    obj_properties[PROP_PARENT] = g_param_spec_object("parent",
                                                    "Parent",
                                                    "Parent Http1StreamWrapper Instance",
                                                    RP_TYPE_HTTP1_STREAM_WRAPPER,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_http1_request_encoder_wrapper_init(RpHttp1RequestEncoderWrapper* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpHttp1RequestEncoderWrapper*
rp_http1_request_encoder_wrapper_new(RpHttp1StreamWrapper* parent, RpRequestEncoder* encoder)
{
    LOGD("(%p, %p)", parent, encoder);
    return g_object_new(RP_TYPE_HTTP1_REQUEST_ENCODER_WRAPPER,
                        "inner", encoder,
                        "parent", parent,
                        NULL);
}
