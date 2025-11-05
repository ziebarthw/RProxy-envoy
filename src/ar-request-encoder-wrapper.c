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

G_DEFINE_FINAL_TYPE(ArRequestEncoderWrapper, ar_request_encoder_wrapper, RP_TYPE_REQUEST_ENCODER_WRAPPER)

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
    object_class->dispose = dispose;

    RpRequestEncoderWrapperClass* wrapper_class = RP_REQUEST_ENCODER_WRAPPER_CLASS(klass);
    wrapper_class->on_encode_complete = on_encode_complete;
}

static void
ar_request_encoder_wrapper_init(ArRequestEncoderWrapper* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

ArRequestEncoderWrapper*
ar_request_encoder_wrapper_new(RpRequestEncoder* inner, RpCodecClient* parent, gpointer arg)
{
    LOGD("(%p, %p, %p)", inner, parent, arg);
    g_return_val_if_fail(RP_IS_REQUEST_ENCODER(inner), NULL);
    g_return_val_if_fail(RP_IS_CODEC_CLIENT(parent), NULL);
    g_return_val_if_fail(RP_IS_CODEC_CLIENT_ACTIVE_REQUEST(arg), NULL);
    ArRequestEncoderWrapper* self = g_object_new(AR_TYPE_REQUEST_ENCODER_WRAPPER,
                                                    "inner", inner,
                                                    NULL);
    self->m_parent = parent;
    self->m_arg = arg;
    return self;
}
