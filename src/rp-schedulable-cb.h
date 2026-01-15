/*
 * rp-schedulable-cb.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"

G_BEGIN_DECLS

/**
 * Callback wrapper that allows direct scheduling of callbacks in the event loop.
 */
#define RP_TYPE_SCHEDULABLE_CALLBACK rp_schedulable_callback_get_type()
G_DECLARE_INTERFACE(RpSchedulableCallback, rp_schedulable_callback, RP, SCHEDULABLE_CALLBACK, GObject)

typedef void (*RpSchedulableCallbackCb)(RpSchedulableCallback*, gpointer);

struct _RpSchedulableCallbackInterface {
    GTypeInterface parent_iface;

    void (*schedule_callback_current_iteration)(RpSchedulableCallback*);
    void (*schedule_callback_next_iteration)(RpSchedulableCallback*);
    void (*cancel)(RpSchedulableCallback*);
    bool (*enabled)(RpSchedulableCallback*);
};

typedef UNIQUE_PTR(RpSchedulableCallback) RpSchedulableCallbackPtr;

static inline void
rp_schedulable_callback_schedule_callback_current_iteration(RpSchedulableCallback* self)
{
    if (RP_IS_SCHEDULABLE_CALLBACK(self)) \
        RP_SCHEDULABLE_CALLBACK_GET_IFACE(self)->schedule_callback_current_iteration(self);
}
static inline void
rp_schedulable_callback_schedule_callback_next_iteration(RpSchedulableCallback* self)
{
    if (RP_IS_SCHEDULABLE_CALLBACK(self)) \
        RP_SCHEDULABLE_CALLBACK_GET_IFACE(self)->schedule_callback_next_iteration(self);
}
static inline void
rp_schedulable_callback_cancel(RpSchedulableCallback* self)
{
    if (RP_IS_SCHEDULABLE_CALLBACK(self)) \
        RP_SCHEDULABLE_CALLBACK_GET_IFACE(self)->cancel(self);
}
static inline bool
rp_schedulable_callback_enabled(RpSchedulableCallback* self)
{
    return RP_IS_SCHEDULABLE_CALLBACK(self) ?
        RP_SCHEDULABLE_CALLBACK_GET_IFACE(self)->enabled(self) : false;
}


/**
 * SchedulableCallback factory.
 */
#define RP_TYPE_CALLBACK_SCHEDULER rp_callback_scheduler_get_type()
G_DECLARE_INTERFACE(RpCallbackScheduler, rp_callback_scheduler, RP, CALLBACK_SCHEDULER, GObject)

struct _RpCallbackSchedulerInterface {
    GTypeInterface parent_iface;

    RpSchedulableCallbackPtr (*create_schedulable_callback)(RpCallbackScheduler*,
                                                            RpSchedulableCallbackCb,
                                                            gpointer);
};

static inline RpSchedulableCallbackPtr
rp_callback_scheduler_create_schedulable_callback(RpCallbackScheduler* self, RpSchedulableCallbackCb cb, gpointer arg)
{
    return RP_IS_CALLBACK_SCHEDULER(self) ?
        RP_CALLBACK_SCHEDULER_GET_IFACE(self)->create_schedulable_callback(self, cb, arg) :
        NULL;
}

G_END_DECLS
