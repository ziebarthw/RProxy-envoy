/*
 * rp-timer.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-time.h"
#include "rp-schedulable-cb.h"

G_BEGIN_DECLS

typedef struct _RpDispatcher RpDispatcher;
typedef struct _RpTimer RpTimer;

/**
 * Callback invoked when a timer event fires.
 */
typedef void (*RpTimerCb)(RpTimer*, gpointer);

/**
 * An abstract timer event. Free the timer to unregister any pending timeouts. Must be freed before
 * the dispatcher is torn down.
 */
#define RP_TYPE_TIMER rp_timer_get_type()
G_DECLARE_INTERFACE(RpTimer, rp_timer, RP, TIMER, GObject)

struct _RpTimerInterface {
    GTypeInterface parent_iface;

    void (*disable_timer)(RpTimer*);
    void (*enable_timer)(RpTimer*, gint64);
    void (*enable_hr_timer)(RpTimer*, gint64);
    bool (*enabled)(RpTimer*);
};

static inline void
rp_timer_disable_timer(RpTimer* self)
{
    if (RP_IS_TIMER(self)) \
        RP_TIMER_GET_IFACE(self)->disable_timer(self);
}
static inline void
rp_timer_enable_timer(RpTimer* self, gint64 ms)
{
    if (RP_IS_TIMER(self)) \
        RP_TIMER_GET_IFACE(self)->enable_timer(self, ms);
}
static inline void
rp_timer_enable_hr_timer(RpTimer* self, gint64 us)
{
    if (RP_IS_TIMER(self)) \
        RP_TIMER_GET_IFACE(self)->enable_hr_timer(self, us);
}
static inline bool
rp_timer_enabled(RpTimer* self)
{
    return RP_IS_TIMER(self) ? RP_TIMER_GET_IFACE(self)->enabled(self) : false;
}


#define RP_TYPE_SCHEDULER rp_scheduler_get_type()
G_DECLARE_INTERFACE(RpScheduler, rp_scheduler, RP, SCHEDULER, GObject)

struct _RpSchedulerInterface {
    GTypeInterface parent_iface;

    RpTimer* (*create_timer)(RpScheduler*, RpTimerCb, gpointer, RpDispatcher*);
};

static inline RpTimer*
rp_scheduler_create_timer(RpScheduler* self, RpTimerCb cb, gpointer arg, RpDispatcher* dispatcher)
{
    return RP_IS_SCHEDULER(self) ?
        RP_SCHEDULER_GET_IFACE(self)->create_timer(self, cb, arg, dispatcher) :
        NULL;
}


/**
 * Interface providing a mechanism to measure time and set timers that run callbacks
 * when the timer fires.
 */
#define RP_TYPE_TIME_SYSTEM rp_time_system_get_type()
G_DECLARE_INTERFACE(RpTimeSystem, rp_time_system, RP, TIME_SYSTEM, RpTimeSource)

struct _RpTimeSystemInterface {
    RpTimeSourceInterface parent_iface;

    RpScheduler* (*create_scheduler)(RpTimeSystem*,
                                        RpScheduler*,
                                        RpCallbackScheduler*);
};

static inline RpScheduler*
rp_time_system_create_scheduler(RpTimeSystem* self, RpScheduler* base_scheduler, RpCallbackScheduler* cb_scheduler)
{
    return RP_IS_TIME_SYSTEM(self) ?
        RP_TIME_SYSTEM_GET_IFACE(self)->create_scheduler(self, base_scheduler, cb_scheduler) :
        NULL;
}


G_END_DECLS
