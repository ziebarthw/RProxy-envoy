/*
 * rp-schedulable-cb-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_schedulable_cb_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_schedulable_cb_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <sys/eventfd.h>

#include "rproxy.h"
#include "event/rp-schedulable-cb-impl.h"

struct _RpSchedulableCallbackImpl {
    RpEventImplBase parent_instance;

    event_t* m_raw_event;
    evbase_t* m_evbase;
    void (*m_cb)(RpSchedulableCallback*, gpointer);
    gpointer m_arg;

    int m_wakeup_fd;
    struct event m_wakeup_event;
};

static void schedulable_callback_iface_init(RpSchedulableCallbackInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpSchedulableCallbackImpl, rp_schedulable_callback_impl, RP_TYPE_EVENT_IMPL_BASE,
    G_IMPLEMENT_INTERFACE(RP_TYPE_SCHEDULABLE_CALLBACK, schedulable_callback_iface_init)
)

static inline bool
internal_enabled(RpSchedulableCallbackImpl* self)
{
    NOISY_MSG_("(%p)", self);
    return evtimer_pending(self->m_raw_event, NULL) != 0;
}

static void
next_iteration(RpSchedulableCallbackImpl* self)
{
    NOISY_MSG_("(%p)", self);
    struct timeval zero_tv = {
        .tv_sec = 0,
        .tv_usec = 0
    };
    event_add(self->m_raw_event, &zero_tv);
}

static void
current_iteration(RpSchedulableCallbackImpl* self)
{
    NOISY_MSG_("(%p)", self);
    if (rp_schedulable_callback_enabled(RP_SCHEDULABLE_CALLBACK(self)))
    {
        NOISY_MSG_("already enabled");
        return;
    }
    event_active(self->m_raw_event, EV_TIMEOUT, 0);
}

static void
wakeup_cb(evutil_socket_t fd, short events, void* arg)
{
    NOISY_MSG_("(%d, %d, %p)", fd, events, arg);
    RpSchedulableCallbackImpl* self = arg;
    eventfd_t value;
    while (eventfd_read(fd, &value) == 0)
    {
        NOISY_MSG_("received value %lu", value);
        (value == 1) ? current_iteration(self) : next_iteration(self);
    }
}

static void
schedule_callback_current_iteration_i(RpSchedulableCallback* self)
{
    NOISY_MSG_("(%p)", self);
    if (rp_schedulable_callback_enabled(self))
    {
        NOISY_MSG_("already enabled");
        return;
    }
    eventfd_write(RP_SCHEDULABLE_CALLBACK_IMPL(self)->m_wakeup_fd, 1);
}

static void
schedule_callback_next_iteration_i(RpSchedulableCallback* self)
{
    NOISY_MSG_("(%p)", self);
    eventfd_write(RP_SCHEDULABLE_CALLBACK_IMPL(self)->m_wakeup_fd, 2);
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
    return internal_enabled(RP_SCHEDULABLE_CALLBACK_IMPL(self));
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

static void
timer_cb(evutil_socket_t fd, short events, void* arg)
{
    NOISY_MSG_("(%d, %d, %p)", fd, events, arg);
    RpSchedulableCallbackImpl* self = RP_SCHEDULABLE_CALLBACK_IMPL(arg);
    self->m_cb(RP_SCHEDULABLE_CALLBACK(self), self->m_arg);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpSchedulableCallbackImpl* self = RP_SCHEDULABLE_CALLBACK_IMPL(obj);
    event_del(&self->m_wakeup_event);
    close(self->m_wakeup_fd);

    G_OBJECT_CLASS(rp_schedulable_callback_impl_parent_class)->dispose(obj);
}

static void
rp_schedulable_callback_impl_class_init(RpSchedulableCallbackImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_schedulable_callback_impl_init(RpSchedulableCallbackImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpSchedulableCallbackImpl*
constructed(RpSchedulableCallbackImpl* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_raw_event = rp_event_impl_base_raw_event_(RP_EVENT_IMPL_BASE(self));
    evtimer_assign(self->m_raw_event, self->m_evbase, timer_cb, self);

    self->m_wakeup_fd = eventfd(0, EFD_NONBLOCK);
    event_assign(&self->m_wakeup_event, self->m_evbase, self->m_wakeup_fd, EV_READ|EV_PERSIST, wakeup_cb, self);
    event_add(&self->m_wakeup_event, NULL);

    return self;
}

RpSchedulableCallbackImpl*
rp_schedulable_callback_impl_new(evbase_t* evbase, RpSchedulableCallbackCb cb, gpointer arg)
{
    LOGD("(%p, %p, %p)", evbase, cb, arg);
    g_return_val_if_fail(evbase != NULL, NULL);
    g_return_val_if_fail(cb != NULL, NULL);
    RpSchedulableCallbackImpl* self = g_object_new(RP_TYPE_SCHEDULABLE_CALLBACK_IMPL, NULL);
    self->m_evbase = evbase;
    self->m_cb = cb;
    self->m_arg = arg;
    return constructed(self);
}
