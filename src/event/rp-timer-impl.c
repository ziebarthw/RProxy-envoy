/*
 * rp-timer-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_timer_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_timer_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include "utils/gduration.h"
#include "event/rp-timer-impl.h"

struct _RpTimerImpl {
    RpEventImplBase parent_instance;

    event_t* m_raw_event;
    evbase_t* m_evbase;
    RpTimerCb m_cb;
    gpointer m_arg;
    RpDispatcher* m_dispatcher;
};

enum
{
    PROP_0, // Reserved.
    PROP_EVBASE,
    PROP_CB,
    PROP_ARG,
    PROP_DISPATCHER,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void timer_iface_init(RpTimerInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpTimerImpl, rp_timer_impl, RP_TYPE_EVENT_IMPL_BASE,
    G_IMPLEMENT_INTERFACE(RP_TYPE_TIMER, timer_iface_init)
)

static inline void
internal_enable_timer(RpTimerImpl* self, struct timeval* tv)
{
    NOISY_MSG_("(%p, %p)", self, tv);
    event_add(self->m_raw_event, tv);
}

static void
disable_timer_i(RpTimer* self)
{
    NOISY_MSG_("(%p)", self);
    event_del(RP_TIMER_IMPL(self)->m_raw_event);
}

static void
duration_to_timeval(GTimeSpan span, struct timeval* tv)
{
    NOISY_MSG_("(%zd, %p)", span, tv);
    if (span < 0)
    {
        LOGE("negative duration passed to durationToTimeval");
        tv->tv_sec = 0;
        tv->tv_usec = 500000;
        return;
    }
    gint64 clip_to = G_MAXINT32; // 136.102208 years.
    guint64 secs = span / G_USEC_PER_SEC;
    if (secs > clip_to)
    {
        tv->tv_sec = clip_to;
        tv->tv_usec = 0;
        return;
    }

    guint64 usecs = span - secs;
    tv->tv_sec = secs;
    tv->tv_usec = usecs;
}

static void
enable_timer_i(RpTimer* self, gint64 msecs)
{
    NOISY_MSG_("(%p, %zd)", self, msecs);
    struct timeval tv;
    duration_to_timeval(gd_milliseconds(msecs), &tv);
    internal_enable_timer(RP_TIMER_IMPL(self), &tv);
}

static void
enable_hr_timer_i(RpTimer* self, gint64 usecs)
{
    NOISY_MSG_("(%p, %zd)", self, usecs);
    struct timeval tv;
    duration_to_timeval(usecs, &tv);
    internal_enable_timer(RP_TIMER_IMPL(self), &tv);
}

static bool
enabled_i(RpTimer* self)
{
    NOISY_MSG_("(%p)", self);
    return evtimer_pending(RP_TIMER_IMPL(self)->m_raw_event, NULL) != 0;
}

static void
timer_iface_init(RpTimerInterface* iface)
{
    LOGD("(%p)", iface);
    iface->disable_timer = disable_timer_i;
    iface->enable_timer = enable_timer_i;
    iface->enable_hr_timer = enable_hr_timer_i;
    iface->enabled = enabled_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_EVBASE:
            g_value_set_pointer(value, RP_TIMER_IMPL(obj)->m_evbase);
            break;
        case PROP_CB:
            g_value_set_pointer(value, RP_TIMER_IMPL(obj)->m_cb);
            break;
        case PROP_ARG:
            g_value_set_pointer(value, RP_TIMER_IMPL(obj)->m_arg);
            break;
        case PROP_DISPATCHER:
            g_value_set_object(value, RP_TIMER_IMPL(obj)->m_dispatcher);
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
        case PROP_EVBASE:
            RP_TIMER_IMPL(obj)->m_evbase = g_value_get_pointer(value);
            break;
        case PROP_CB:
            RP_TIMER_IMPL(obj)->m_cb = g_value_get_pointer(value);
            break;
        case PROP_ARG:
            RP_TIMER_IMPL(obj)->m_arg = g_value_get_pointer(value);
            break;
        case PROP_DISPATCHER:
            RP_TIMER_IMPL(obj)->m_dispatcher = g_value_get_object(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
timer_cb(evutil_socket_t fd, short events, void* arg)
{
    NOISY_MSG_("(%d, %d, %p)", fd, events, arg);
    RpTimerImpl* self = RP_TIMER_IMPL(arg);
    self->m_cb(RP_TIMER(self), self->m_arg);
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_timer_impl_parent_class)->constructed(obj);

    RpTimerImpl* self = RP_TIMER_IMPL(obj);
    self->m_raw_event = rp_event_impl_base_raw_event_(RP_EVENT_IMPL_BASE(self));
    evtimer_assign(self->m_raw_event, self->m_evbase, timer_cb, obj);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpTimerImpl* self = RP_TIMER_IMPL(obj);
//    if (timer_enabled(self)) timer_cancel(self);

    G_OBJECT_CLASS(rp_timer_impl_parent_class)->dispose(obj);
}

static void
rp_timer_impl_class_init(RpTimerImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    obj_properties[PROP_EVBASE] = g_param_spec_pointer("evbase",
                                                    "evbase",
                                                    "Base event loop (evbase_t) Instance",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_CB] = g_param_spec_pointer("cb",
                                                    "callback",
                                                    "Callback func",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_ARG] = g_param_spec_pointer("arg",
                                                    "arg",
                                                    "Argument",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_DISPATCHER] = g_param_spec_object("dispatcher",
                                                    "Dispatcher",
                                                    "Dispatcher Instance",
                                                    RP_TYPE_DISPATCHER,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_timer_impl_init(RpTimerImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpTimerImpl*
rp_timer_impl_new(evbase_t* evbase, RpTimerCb cb, gpointer arg, RpDispatcher* dispatcher)
{
    LOGD("(%p, %p, %p, %p)", evbase, cb, arg, dispatcher);
    return g_object_new(RP_TYPE_TIMER_IMPL,
                        "evbase", evbase,
                        "cb", cb,
                        "arg", arg,
                        NULL);
}
