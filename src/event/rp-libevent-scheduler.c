/*
 * rp-libevent-scheduler.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_libevent_scheduler_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_libevent_scheduler_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "event/rp-schedulable-cb-impl.h"
#include "event/rp-timer-impl.h"
#include "event/rp-libevent-scheduler.h"

struct _RpLibeventScheduler {
    GObject parent_instance;

    SHARED_PTR(evthr_t) m_thr;
};

static void scheduler_iface_init(RpSchedulerInterface* iface);
static void callback_scheduler_iface_init(RpCallbackSchedulerInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpLibeventScheduler, rp_libevent_scheduler, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_SCHEDULER, scheduler_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_CALLBACK_SCHEDULER, callback_scheduler_iface_init)
)

static RpTimer*
create_timer_i(RpScheduler* self, RpTimerCb cb, void* arg, RpDispatcher* dispatcher)
{
    NOISY_MSG_("(%p, %p, %p, %p)", self, cb, arg, dispatcher);
    RpLibeventScheduler* me = RP_LIBEVENT_SCHEDULER(self);
    return RP_TIMER(rp_timer_impl_new(evthr_get_base(me->m_thr), cb, arg, dispatcher));
}

static void
scheduler_iface_init(RpSchedulerInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_timer = create_timer_i;
}

static RpSchedulableCallback*
create_schedulable_callback_i(RpCallbackScheduler* self, RpSchedulableCallbackCb cb, gpointer arg)
{
    NOISY_MSG_("(%p, %p, %p)", self, cb, arg);
    RpLibeventScheduler* me = RP_LIBEVENT_SCHEDULER(self);
    return RP_SCHEDULABLE_CALLBACK(rp_schedulable_callback_impl_new(evthr_get_base(me->m_thr), cb, arg));
}

static void
callback_scheduler_iface_init(RpCallbackSchedulerInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_schedulable_callback = create_schedulable_callback_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_libevent_scheduler_parent_class)->dispose(obj);
}

static void
rp_libevent_scheduler_class_init(RpLibeventSchedulerClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_libevent_scheduler_init(RpLibeventScheduler* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpLibeventScheduler*
rp_libevent_scheduler_new(SHARED_PTR(evthr_t) thr)
{
    LOGD("(%p)", thr);
    g_return_val_if_fail(thr != NULL, NULL);
    RpLibeventScheduler* self = g_object_new(RP_TYPE_LIBEVENT_SCHEDULER, NULL);
    self->m_thr = thr;
    return self;
}

evbase_t*
rp_libevent_scheduler_base(RpLibeventScheduler* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_LIBEVENT_SCHEDULER(self), NULL);
    return evthr_get_base(RP_LIBEVENT_SCHEDULER(self)->m_thr);
}

void
rp_libevent_scheduler_run(RpLibeventScheduler* self, RpRunType_e mode)
{
    LOGD("(%p, %d)", self, mode);
    g_return_if_fail(RP_IS_LIBEVENT_SCHEDULER(self));
    int flag = 0;
    switch (mode)
    {
        case RpRunType_NON_BLOCK:
            flag = EVLOOP_NONBLOCK;
            break;
        case RpRunType_BLOCK:
            break;
        case RpRunType_RUN_UNTIL_EXIT:
            flag = EVLOOP_NO_EXIT_ON_EMPTY;
            break;
        default:
            break;
    }
    event_base_loop(evthr_get_base(self->m_thr), flag);
}

void
rp_libevent_scheduler_loop_exit(RpLibeventScheduler* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_LIBEVENT_SCHEDULER(self));
    event_base_loopexit(evthr_get_base(self->m_thr), NULL);
}
