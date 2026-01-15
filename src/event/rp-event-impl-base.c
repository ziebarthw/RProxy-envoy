/*
 * rp-event-impl-base.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_event_impl_base_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_event_impl_base_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <event2/event.h>
#include "rproxy.h"
#include "rp-event-impl-base.h"

typedef struct _RpEventImplBasePrivate RpEventImplBasePrivate;
struct _RpEventImplBasePrivate {

    struct event m_raw_event;
};

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpEventImplBase, rp_event_impl_base, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpEventImplBase)
)

#define PRIV(obj) \
    ((RpEventImplBasePrivate*) rp_event_impl_base_get_instance_private(RP_EVENT_IMPL_BASE(obj)))

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpEventImplBasePrivate* me = PRIV(obj);
    event_del(&me->m_raw_event);

    G_OBJECT_CLASS(rp_event_impl_base_parent_class)->dispose(obj);
}

static void
rp_event_impl_base_class_init(RpEventImplBaseClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_event_impl_base_init(RpEventImplBase* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

struct event*
rp_event_impl_base_raw_event_(RpEventImplBase* self)
{
    NOISY_MSG_("(%p)", self);
    return &PRIV(self)->m_raw_event;
}
