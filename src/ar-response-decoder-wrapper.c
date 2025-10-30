/*
 * ar-response-decoder-wrapper.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(ar_response_decoder_wrapper_NOISY) || defined(ALL_NOISY)) && !defined(NO_ar_response_decoder_wrapper_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-codec-client-active-request.h"
#include "ar-response-decoder-wrapper.h"

struct _ArResponseDecoderWrapper {
    RpResponseDecoderWrapper parent_instance;

    RpCodecClient* m_parent;
    RpCodecClientActiveRequest* m_active_request;
};

enum
{
    PROP_0, // Reserved.
    PROP_PARENT,
    PROP_ACTIVE_REQUEST,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_FINAL_TYPE(ArResponseDecoderWrapper, ar_response_decoder_wrapper, RP_TYPE_RESPONSE_DECODER_WRAPPER)

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_PARENT:
            g_value_set_object(value, AR_RESPONSE_DECODER_WRAPPER(obj)->m_parent);
            break;
        case PROP_ACTIVE_REQUEST:
            g_value_set_object(value, AR_RESPONSE_DECODER_WRAPPER(obj)->m_active_request);
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
            AR_RESPONSE_DECODER_WRAPPER(obj)->m_parent = g_value_get_object(value);
            break;
        case PROP_ACTIVE_REQUEST:
            AR_RESPONSE_DECODER_WRAPPER(obj)->m_active_request = g_value_get_object(value);
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
    G_OBJECT_CLASS(ar_response_decoder_wrapper_parent_class)->dispose(obj);
}

OVERRIDE void
on_pre_decode_complete(RpResponseDecoderWrapper* self)
{
    NOISY_MSG_("(%p)", self);
    ArResponseDecoderWrapper* me = AR_RESPONSE_DECODER_WRAPPER(self);
    rp_codec_client_response_pre_decode_complete(me->m_parent, me->m_active_request);
}

OVERRIDE void
on_decode_complete(RpResponseDecoderWrapper* self G_GNUC_UNUSED)
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
ar_response_decoder_wrapper_class_init(ArResponseDecoderWrapperClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    response_decoder_wrapper_class_init(RP_RESPONSE_DECODER_WRAPPER_CLASS(klass));

    obj_properties[PROP_PARENT] = g_param_spec_object("parent",
                                                    "Parent",
                                                    "Parent CodecClient Instance",
                                                    RP_TYPE_CODEC_CLIENT,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_ACTIVE_REQUEST] = g_param_spec_object("active-request",
                                                    "Active request",
                                                    "Active Request Instance",
                                                    RP_TYPE_CODEC_CLIENT_ACTIVE_REQUEST,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
ar_response_decoder_wrapper_init(ArResponseDecoderWrapper* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

ArResponseDecoderWrapper*
ar_response_decoder_wrapper_new(RpResponseDecoder* inner, RpCodecClient* parent, RpCodecClientActiveRequest* active_request)
{
    LOGD("(%p, %p, %p)", inner, parent, active_request);
    g_return_val_if_fail(RP_IS_RESPONSE_DECODER(inner), NULL);
    g_return_val_if_fail(RP_IS_CODEC_CLIENT(parent), NULL);
    g_return_val_if_fail(RP_IS_CODEC_CLIENT_ACTIVE_REQUEST(active_request), NULL);
    return g_object_new(AR_TYPE_RESPONSE_DECODER_WRAPPER,
                        "active-request", active_request,
                        "inner", inner,
                        "parent", parent,
                        NULL);
}
