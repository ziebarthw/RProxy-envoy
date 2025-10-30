/*
 * rp-http-pending-stream.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_http_pending_stream_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_http_pending_stream_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-http-conn-pool-base.h"
#include "rp-http-pending-stream.h"

struct _RpHttpPendingStream {
    RpPendingStream parent_instance;

    RpResponseDecoder* m_decoder;
    RpHttpConnPoolCallbacks* m_callbacks;
    RpHttpAttachContext m_context;
};

G_DEFINE_FINAL_TYPE(RpHttpPendingStream, rp_http_pending_stream, RP_TYPE_PENDING_STREAM)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_http_pending_stream_parent_class)->dispose(obj);
}

OVERRIDE RpConnectionPoolAttachContextPtr
context(RpPendingStream* self)
{
    NOISY_MSG_("(%p)", self);
    return (RpConnectionPoolAttachContextPtr)&RP_HTTP_PENDING_STREAM(self)->m_context;
}

static void
pending_stream_class_init(RpPendingStreamClass* klass)
{
    LOGD("(%p)", klass);
    klass->context = context;
}

static void
rp_http_pending_stream_class_init(RpHttpPendingStreamClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    pending_stream_class_init(RP_PENDING_STREAM_CLASS(klass));
}

static void
rp_http_pending_stream_init(RpHttpPendingStream* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpHttpPendingStream*
constructed(RpHttpPendingStream* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_context = rp_http_attach_context_ctor(self->m_decoder, self->m_callbacks);
    return self;
}

RpHttpPendingStream*
rp_http_pending_stream_new(RpConnPoolImplBase* parent, RpResponseDecoder* decoder,
                            RpHttpConnPoolCallbacks* callbacks, bool can_send_early_data)
{
    LOGD("(%p, %p, %p, %u)", parent, decoder, callbacks, can_send_early_data);
    g_return_val_if_fail(RP_IS_CONN_POOL_IMPL_BASE(parent), NULL);
    g_return_val_if_fail(RP_IS_RESPONSE_DECODER(decoder), NULL);
    g_return_val_if_fail(RP_IS_HTTP_CONN_POOL_CALLBACKS(callbacks), NULL);
    RpHttpPendingStream* self = g_object_new(RP_TYPE_HTTP_PENDING_STREAM,
                                                "parent", parent,
                                                "can-send-early-data", can_send_early_data,
                                                NULL);
    self->m_decoder = decoder;
    self->m_callbacks = callbacks;
    return constructed(self);
}
