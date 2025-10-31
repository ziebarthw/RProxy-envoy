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

G_DEFINE_FINAL_TYPE(RpHttp1RequestEncoderWrapper, rp_http1_request_encoder_wrapper, RP_TYPE_REQUEST_ENCODER_WRAPPER)

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
    object_class->dispose = dispose;

    request_encoder_wrapper_class_init(RP_REQUEST_ENCODER_WRAPPER_CLASS(klass));
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
    RpHttp1RequestEncoderWrapper* self = g_object_new(RP_TYPE_HTTP1_REQUEST_ENCODER_WRAPPER,
                                                        "inner", encoder,
                                                        NULL);
    self->m_parent = parent;
    return self;
}
