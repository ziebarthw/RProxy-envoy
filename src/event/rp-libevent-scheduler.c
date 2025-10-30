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

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include "event/rp-schedulable-cb-impl.h"
#include "event/rp-timer-impl.h"
#include "event/rp-libevent-scheduler.h"

struct _RpLibeventScheduler {
    GObject parent_instance;

    evthr_t* m_thr;
};

enum
{
    PROP_0, // Reserved.
    PROP_THR,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

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
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_THR:
            g_value_set_pointer(value, RP_LIBEVENT_SCHEDULER(obj)->m_thr);
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
        case PROP_THR:
            RP_LIBEVENT_SCHEDULER(obj)->m_thr = g_value_get_pointer(value);
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
    G_OBJECT_CLASS(rp_libevent_scheduler_parent_class)->dispose(obj);
}

static void
rp_libevent_scheduler_class_init(RpLibeventSchedulerClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_THR] = g_param_spec_pointer("thr",
                                                    "Thr",
                                                    "Thread (evthr_t) Instance",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_libevent_scheduler_init(RpLibeventScheduler* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpLibeventScheduler*
rp_libevent_scheduler_new(evthr_t* thr)
{
    LOGD("(%p)", thr);
    g_return_val_if_fail(thr != NULL, NULL);
    return g_object_new(RP_TYPE_LIBEVENT_SCHEDULER, "thr", thr, NULL);
}

evbase_t*
rp_libevent_scheduler_base(RpLibeventScheduler* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_LIBEVENT_SCHEDULER(self), NULL);
    return evthr_get_base(RP_LIBEVENT_SCHEDULER(self)->m_thr);
}
