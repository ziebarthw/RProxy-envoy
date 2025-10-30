/*
 * rp-schedulable-cb-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_schedulable_cb_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_schedulable_cb_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include "event/rp-schedulable-cb-impl.h"

struct _RpSchedulableCallbackImpl {
    RpEventImplBase parent_instance;

    event_t* m_raw_event;
    evbase_t* m_evbase;
    void (*m_cb)(RpSchedulableCallback*, gpointer);
    gpointer m_arg;
};

enum
{
    PROP_0, // Reserved.
    PROP_EVBASE,
    PROP_CB,
    PROP_ARG,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void schedulable_callback_iface_init(RpSchedulableCallbackInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpSchedulableCallbackImpl, rp_schedulable_callback_impl, RP_TYPE_EVENT_IMPL_BASE,
    G_IMPLEMENT_INTERFACE(RP_TYPE_SCHEDULABLE_CALLBACK, schedulable_callback_iface_init)
)

static void
schedule_callback_current_iteration_i(RpSchedulableCallback* self)
{
    NOISY_MSG_("(%p)", self);
    if (rp_schedulable_callback_enabled(self))
    {
        NOISY_MSG_("already enabled");
        return;
    }
    event_active(RP_SCHEDULABLE_CALLBACK_IMPL(self)->m_raw_event, EV_TIMEOUT, 0);
}

static void
schedule_callback_next_iteration_i(RpSchedulableCallback* self)
{
    NOISY_MSG_("(%p)", self);
    struct timeval zero_tv = {
        .tv_sec = 0,
        .tv_usec = 0
    };
    event_add(RP_SCHEDULABLE_CALLBACK_IMPL(self)->m_raw_event, &zero_tv);
}

static void
cancel_i(RpSchedulableCallback* self)
{
    NOISY_MSG_("(%p)", self);
    event_del(RP_SCHEDULABLE_CALLBACK_IMPL(self)->m_raw_event);
}

static bool
enabled_i(RpSchedulableCallback* self)
{
    NOISY_MSG_("(%p)", self);
    return evtimer_pending(RP_SCHEDULABLE_CALLBACK_IMPL(self)->m_raw_event, NULL) != 0;
}

static void
schedulable_callback_iface_init(RpSchedulableCallbackInterface* iface)
{
    LOGD("(%p)", iface);
    iface->schedule_callback_current_iteration = schedule_callback_current_iteration_i;
    iface->schedule_callback_next_iteration = schedule_callback_next_iteration_i;
    iface->cancel = cancel_i;
    iface->enabled = enabled_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_EVBASE:
            g_value_set_pointer(value, RP_SCHEDULABLE_CALLBACK_IMPL(obj)->m_evbase);
            break;
        case PROP_CB:
            g_value_set_pointer(value, RP_SCHEDULABLE_CALLBACK_IMPL(obj)->m_cb);
            break;
        case PROP_ARG:
            g_value_set_pointer(value, RP_SCHEDULABLE_CALLBACK_IMPL(obj)->m_arg);
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
            RP_SCHEDULABLE_CALLBACK_IMPL(obj)->m_evbase = g_value_get_pointer(value);
            break;
        case PROP_CB:
            RP_SCHEDULABLE_CALLBACK_IMPL(obj)->m_cb = g_value_get_pointer(value);
            break;
        case PROP_ARG:
            RP_SCHEDULABLE_CALLBACK_IMPL(obj)->m_arg = g_value_get_pointer(value);
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
    RpSchedulableCallbackImpl* self = RP_SCHEDULABLE_CALLBACK_IMPL(arg);
    self->m_cb(RP_SCHEDULABLE_CALLBACK(self), self->m_arg);
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_schedulable_callback_impl_parent_class)->constructed(obj);

    RpSchedulableCallbackImpl* self = RP_SCHEDULABLE_CALLBACK_IMPL(obj);
    self->m_raw_event = rp_event_impl_base_raw_event_(RP_EVENT_IMPL_BASE(self));
    evtimer_assign(self->m_raw_event, self->m_evbase, timer_cb, obj);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpSchedulableCallbackImpl* self = RP_SCHEDULABLE_CALLBACK_IMPL(obj);

    G_OBJECT_CLASS(rp_schedulable_callback_impl_parent_class)->dispose(obj);
}

static void
rp_schedulable_callback_impl_class_init(RpSchedulableCallbackImplClass* klass)
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

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_schedulable_callback_impl_init(RpSchedulableCallbackImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpSchedulableCallbackImpl*
rp_schedulable_callback_impl_new(evbase_t* evbase, RpSchedulableCallbackCb cb, gpointer arg)
{
    LOGD("(%p, %p, %p)", evbase, cb, arg);
    g_return_val_if_fail(evbase != NULL, NULL);
    g_return_val_if_fail(cb != NULL, NULL);
    return g_object_new(RP_TYPE_SCHEDULABLE_CALLBACK_IMPL,
                        "evbase", evbase,
                        "cb", cb,
                        "arg", arg,
                        NULL);
}
