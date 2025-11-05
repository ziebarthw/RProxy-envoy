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

G_DEFINE_FINAL_TYPE(ArResponseDecoderWrapper, ar_response_decoder_wrapper, RP_TYPE_RESPONSE_DECODER_WRAPPER)

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
    object_class->dispose = dispose;

    response_decoder_wrapper_class_init(RP_RESPONSE_DECODER_WRAPPER_CLASS(klass));
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
    ArResponseDecoderWrapper* self = g_object_new(AR_TYPE_RESPONSE_DECODER_WRAPPER,
                                                    "active-request", active_request,
                                                    "inner", inner,
                                                    "parent", parent,
                                                    NULL);
    self->m_parent = parent;
    self->m_active_request = active_request;
    return self;
}
