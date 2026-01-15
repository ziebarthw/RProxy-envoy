/*
 * rp-signal-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_signal_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_signal_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "event/rp-signal-impl.h"

struct _RpSignalEventImpl {
    RpEventImplBase parent_instance;

    RpSignalCb m_cb;
    gpointer m_arg;
};

G_DEFINE_FINAL_TYPE_WITH_CODE(RpSignalEventImpl, rp_signal_event_impl, RP_TYPE_EVENT_IMPL_BASE,
    G_IMPLEMENT_INTERFACE(RP_TYPE_SIGNAL_EVENT, NULL)
)

static void
signal_cb(evutil_socket_t fd G_GNUC_UNUSED, short event G_GNUC_UNUSED, gpointer arg)
{
    printf("\n");
    NOISY_MSG_("(%d, %d, %p)", fd, event, arg);
    RpSignalEventImpl* self = arg;
    self->m_cb(self->m_arg);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_signal_event_impl_parent_class)->dispose(obj);
}

static void
rp_signal_event_impl_class_init(RpSignalEventImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_signal_event_impl_init(RpSignalEventImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpSignalEvent*
constructed(RpSignalEventImpl* self, RpDispatcherImpl* dispatcher, signal_t signal_num)
{
    NOISY_MSG_("(%p, %p, %d)", self, dispatcher, signal_num);

    struct event* raw_event = rp_event_impl_base_raw_event_(RP_EVENT_IMPL_BASE(self));
    evbase_t* evbase = rp_dispatcher_impl_base(dispatcher);
    evsignal_assign(raw_event, evbase, signal_num, signal_cb, self);
    evsignal_add(raw_event, NULL);
    return RP_SIGNAL_EVENT(self);
}

RpSignalEvent*
rp_signal_event_impl_new(RpDispatcherImpl* dispatcher, signal_t signal_num, RpSignalCb cb, gpointer arg)
{
    LOGD("(%p, %d, %p, %p)", dispatcher, signal_num, cb, arg);

    g_return_val_if_fail(RP_IS_DISPATCHER_IMPL(dispatcher), NULL);
    g_return_val_if_fail(cb != NULL, NULL);

    RpSignalEventImpl* self = g_object_new(RP_TYPE_SIGNAL_EVENT_IMPL, NULL);
    self->m_cb = cb;
    self->m_arg = arg;
    return constructed(self, dispatcher, signal_num);
}
