/*
 * ar-request-encoder-wrapper.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(ar_request_encoder_wrapper_NOISY) || defined(ALL_NOISY)) && !defined(NO_ar_request_encoder_wrapper_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-codec-client-active-request.h"
#include "ar-request-encoder-wrapper.h"

struct _ArRequestEncoderWrapper {
    RpRequestEncoderWrapper parent_instance;

    RpCodecClient* m_parent;
    void* m_arg;
};

enum
{
    PROP_0, // Reserved.
    PROP_PARENT,
    PROP_ARG,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_FINAL_TYPE(ArRequestEncoderWrapper, ar_request_encoder_wrapper, RP_TYPE_REQUEST_ENCODER_WRAPPER)

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_PARENT:
            g_value_set_object(value, AR_REQUEST_ENCODER_WRAPPER(obj)->m_parent);
            break;
        case PROP_ARG:
            g_value_set_pointer(value, AR_REQUEST_ENCODER_WRAPPER(obj)->m_arg);
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
            AR_REQUEST_ENCODER_WRAPPER(obj)->m_parent = g_value_get_object(value);
            break;
        case PROP_ARG:
            AR_REQUEST_ENCODER_WRAPPER(obj)->m_arg = g_value_get_pointer(value);
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
    G_OBJECT_CLASS(ar_request_encoder_wrapper_parent_class)->dispose(obj);
}

OVERRIDE void
on_encode_complete(RpRequestEncoderWrapper* self)
{
    NOISY_MSG_("(%p)", self);
    ArRequestEncoderWrapper* me = AR_REQUEST_ENCODER_WRAPPER(self);
    rp_codec_client_request_encode_complete(me->m_parent, me->m_arg);
}

static void
ar_request_encoder_wrapper_class_init(ArRequestEncoderWrapperClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    RpRequestEncoderWrapperClass* wrapper_class = RP_REQUEST_ENCODER_WRAPPER_CLASS(klass);
    wrapper_class->on_encode_complete = on_encode_complete;

    obj_properties[PROP_PARENT] = g_param_spec_object("parent",
                                                    "Parent",
                                                    "Parent CodecClient Instance",
                                                    RP_TYPE_CODEC_CLIENT,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_ARG] = g_param_spec_pointer("arg",
                                                    "Arg",
                                                    "Argument",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
ar_request_encoder_wrapper_init(ArRequestEncoderWrapper* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

ArRequestEncoderWrapper*
ar_request_encoder_wrapper_new(RpRequestEncoder* inner, RpCodecClient* parent, void* arg)
{
    LOGD("(%p, %p, %p)", inner, parent, arg);
    g_return_val_if_fail(RP_IS_REQUEST_ENCODER(inner), NULL);
    g_return_val_if_fail(RP_IS_CODEC_CLIENT(parent), NULL);
    g_return_val_if_fail(RP_IS_CODEC_CLIENT_ACTIVE_REQUEST(arg), NULL);
    return g_object_new(AR_TYPE_REQUEST_ENCODER_WRAPPER,
                        "arg", arg,
                        "inner", inner,
                        "parent", parent,
                        NULL);
}
