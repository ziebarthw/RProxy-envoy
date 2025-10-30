/*
 * rp-libevent-scheduler.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-dispatcher.h"
#include "rp-schedulable-cb.h"
#include "rp-timer.h"

#include <evhtp.h>

G_BEGIN_DECLS

// Implements Scheduler based on libevent.
//
// Here is a rough summary of operations that libevent performs in each event loop iteration, in
// order. Note that the invocation order for "same-iteration" operations that execute as a group
// can be surprising and invocation order of expired timers is non-deterministic.
// Whenever possible, it is preferable to avoid making event invocation ordering assumptions.
//
// 1. Calculate the poll timeout by comparing the current time to the deadline of the closest
// timer (the one at head of the priority queue).
// 2. Run registered "prepare" callbacks.
// 3. Poll for fd events using the closest timer as timeout, add active fds to the work list.
// 4. Run registered "check" callbacks.
// 5. Check timer deadlines against current time and move expired timers from the timer priority
// queue to the work list. Expired timers are moved to the work list is a non-deterministic order.
// 6. Execute items in the work list until the list is empty. Note that additional work
// items could be added to the work list during execution of this step, more details below.
// 7. Goto 1 if the loop termination condition has not been reached
//
// The following "same-iteration" work items are added directly to the work list when they are
// scheduled so they execute in the current iteration of the event loop. Note that there are no
// ordering guarantees when mixing the mechanisms below. Specifically, it is unsafe to assume that
// calling post followed by deferredDelete will result in the post callback being invoked before the
// deferredDelete; deferredDelete will run first if there is a pending deferredDeletion at the time
// the post callback is scheduled because deferredDelete invocation is grouped.
// - Event::Dispatcher::post(cb). Post callbacks are invoked as a group.
// - Event::Dispatcher::deferredDelete(object) and Event::DeferredTaskUtil::deferredRun(...).
// The same mechanism implements both of these operations, so they are invoked as a group.
// - Event::SchedulableCallback::scheduleCallbackCurrentIteration(). Each of these callbacks is
// scheduled and invoked independently.
//
// Event::FileEvent::activate and Event::SchedulableCallback::scheduleCallbackNextIteration are
// implemented as libevent timers with a deadline of 0. Both of these actions are moved to the work
// list while checking for expired timers during step 5.
//
// Events execute in the following order, derived from the order in which items were added to the
// work list:
// 0. Events added via event_active prior to the start of the event loop (in tests)
// 1. Fd events
// 2. Timers, FileEvent::activate and SchedulableCallback::scheduleCallbackNextIteration
// 3. "Same-iteration" work items described above, including Event::Dispatcher::post callbacks
#define RP_TYPE_LIBEVENT_SCHEDULER rp_libevent_scheduler_get_type()
G_DECLARE_FINAL_TYPE(RpLibeventScheduler, rp_libevent_scheduler, RP, LIBEVENT_SCHEDULER, GObject)

RpLibeventScheduler* rp_libevent_scheduler_new(evthr_t* thr);
evbase_t* rp_libevent_scheduler_base(RpLibeventScheduler* self);

G_END_DECLS
