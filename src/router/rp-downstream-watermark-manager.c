/*
 * rp-downstream-watermark-manager.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_downstream_watermark_manager_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_downstream_watermark_manager_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "router/rp-downstream-watermark-manager.h"

#define PARENT(s) RP_DOWNSTREAM_WATERMARK_MANAGER(s)->m_parent

struct _RpDownstreamWatermarkManager {
    GObject parent_instance;

    RpUpstreamRequest* m_parent;
};

static void downstream_watermark_callbacks_iface_init(RpDownstreamWatermarkCallbacksInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpDownstreamWatermarkManager, rp_downstream_watermark_manager, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_DOWNSTREAM_WATERMARK_CALLBACKS, downstream_watermark_callbacks_iface_init)
)

static void
on_above_write_buffer_high_watermark_i(RpDownstreamWatermarkCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    g_assert(rp_upstream_request_upstream_(PARENT(self)));
    rp_upstream_request_read_disable_or_defer(PARENT(self), true);
}

static void
on_below_write_buffer_low_watermark_i(RpDownstreamWatermarkCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    g_assert(rp_upstream_request_upstream_(PARENT(self)));
    rp_upstream_request_read_disable_or_defer(PARENT(self), false);
}

static void
downstream_watermark_callbacks_iface_init(RpDownstreamWatermarkCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_above_write_buffer_high_watermark = on_above_write_buffer_high_watermark_i;
    iface->on_below_write_buffer_low_watermark = on_below_write_buffer_low_watermark_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_downstream_watermark_manager_parent_class)->dispose(obj);
}

static void
rp_downstream_watermark_manager_class_init(RpDownstreamWatermarkManagerClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_downstream_watermark_manager_init(RpDownstreamWatermarkManager* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpDownstreamWatermarkManager*
rp_downstream_watermark_manager_new(RpUpstreamRequest* parent)
{
    LOGD("(%p)", parent);
    g_return_val_if_fail(RP_IS_UPSTREAM_REQUEST(parent), NULL);
    RpDownstreamWatermarkManager* self = g_object_new(RP_TYPE_DOWNSTREAM_WATERMARK_MANAGER, NULL);
    self->m_parent = parent;
    return self;
}
