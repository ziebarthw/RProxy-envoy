/*
 * rp-real-time-system.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_real_time_system_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_real_time_system_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <stdio.h>
#include "rproxy.h"
#include "event/rp-real-time-system.h"

struct _RpRealTimeSystem {
    GObject parent_instance;

    RpScheduler* m_base_scheduler;
};

static void time_source_iface_init(RpTimeSourceInterface* iface);
static void time_system_iface_init(RpTimeSystemInterface* iface);
static void scheduler_iface_init(RpSchedulerInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpRealTimeSystem, rp_real_time_system, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_TIME_SOURCE, time_source_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_TIME_SYSTEM, time_system_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_SCHEDULER, scheduler_iface_init)
)

static RpTimer*
create_timer_i(RpScheduler* self, RpTimerCb cb, gpointer arg, RpDispatcher* dispatcher)
{
    NOISY_MSG_("(%p, %p, %p, %p)", self, cb, arg, dispatcher);
    return rp_scheduler_create_timer(RP_REAL_TIME_SYSTEM(self)->m_base_scheduler, cb, arg, dispatcher);
}

static void
scheduler_iface_init(RpSchedulerInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_timer = create_timer_i;
}

static RpMonotonicTime
monotonic_time_i(RpTimeSource* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return g_get_monotonic_time();
}

static RpSystemTime
system_time_i(RpTimeSource* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return g_get_real_time();
}

static void
time_source_iface_init(RpTimeSourceInterface* iface)
{
    LOGD("(%p)", iface);
    iface->monotonic_time = monotonic_time_i;
    iface->system_time = system_time_i;
}

static RpScheduler*
create_scheduler_i(RpTimeSystem* self, RpScheduler* scheduler, RpCallbackScheduler* cb_scheduler G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p, %p)", self, scheduler, cb_scheduler);
    RP_REAL_TIME_SYSTEM(self)->m_base_scheduler = scheduler;
    return g_object_ref(RP_SCHEDULER(self));
}

static void
time_system_iface_init(RpTimeSystemInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_scheduler = create_scheduler_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_real_time_system_parent_class)->dispose(obj);
}

static void
rp_real_time_system_class_init(RpRealTimeSystemClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_real_time_system_init(RpRealTimeSystem* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpRealTimeSystem*
rp_real_time_system_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_REAL_TIME_SYSTEM, NULL);
}
