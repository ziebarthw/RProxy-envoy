/*
 * rp-tcp-pending-stream.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_tcp_pending_stream_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_tcp_pending_stream_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "tcp/rp-tcp-pending-stream.h"

struct _RpTcpPendingStream {
    RpPendingStream parent_instance;

    RpTcpAttachContext m_context;
};

enum
{
    PROP_0, // Reserved.
    PROP_CONTEXT,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_FINAL_TYPE(RpTcpPendingStream, rp_tcp_pending_stream, RP_TYPE_PENDING_STREAM)

OVERRIDE void
set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_CONTEXT:
            RP_TCP_PENDING_STREAM(obj)->m_context = *((RpTcpAttachContextPtr)g_value_get_pointer(value));
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
    G_OBJECT_CLASS(rp_tcp_pending_stream_parent_class)->dispose(obj);
}

OVERRIDE RpConnectionPoolAttachContextPtr
context(RpPendingStream* self)
{
    NOISY_MSG_("(%p)", self);
    return (RpConnectionPoolAttachContextPtr)&RP_TCP_PENDING_STREAM(self)->m_context;
}

static void
pending_stream_class_init(RpPendingStreamClass* klass)
{
    LOGD("(%p)", klass);
    klass->context = context;
}

static void
rp_tcp_pending_stream_class_init(RpTcpPendingStreamClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    pending_stream_class_init(RP_PENDING_STREAM_CLASS(klass));

    obj_properties[PROP_CONTEXT] = g_param_spec_pointer("context",
                                                    "Context",
                                                    "ClusterFactoryContext Instance",
                                                    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_tcp_pending_stream_init(RpTcpPendingStream* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpTcpPendingStream*
rp_tcp_pending_stream_new(RpConnPoolImplBase* parent, bool can_send_early_data, RpTcpAttachContext* context)
{
    LOGD("(%p, %u, %p)", parent, can_send_early_data, context);
    return g_object_new(RP_TYPE_TCP_PENDING_STREAM,
                        "parent", parent,
                        "can-send-early-data", can_send_early_data,
                        "context", context,
                        NULL);
}
