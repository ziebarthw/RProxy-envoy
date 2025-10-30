/*
 * rp-dispatcher.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <evhtp.h>
#include "rp-listen-socket.h"
#include "rp-time.h"
#include "rp-timer.h"

G_BEGIN_DECLS

/**
 * Callback invoked when a dispatcher post() runs.
 */
typedef void (*RpPostCb)(void*);

/**
 * Minimal interface to the dispatching loop used to create low-level primitives. See Dispatcher
 * below for the full interface.
 */
#define RP_TYPE_DISPATCHER_BASE rp_dispatcher_base_get_type()
G_DECLARE_INTERFACE(RpDispatcherBase, rp_dispatcher_base, RP, DISPATCHER_BASE, GObject)

struct _RpDispatcherBaseInterface {
    GTypeInterface parent_iface;

    void (*post)(RpDispatcherBase*, RpPostCb, gpointer);
    bool (*is_thread_safe)(RpDispatcherBase*);
};

static inline void
rp_dispatcher_base_post(RpDispatcherBase* self, RpPostCb callback, gpointer arg)
{
    if (RP_IS_DISPATCHER_BASE(self)) \
        RP_DISPATCHER_BASE_GET_IFACE(self)->post(self, callback, arg);
}
static inline bool
rp_dispatcher_base_is_thread_safe(RpDispatcherBase* self)
{
    return RP_IS_DISPATCHER_BASE(self) ?
        RP_DISPATCHER_BASE_GET_IFACE(self)->is_thread_safe(self) : false;
}


/**
 * Abstract event dispatching loop.
 */
#define RP_TYPE_DISPATCHER rp_dispatcher_get_type()
G_DECLARE_INTERFACE(RpDispatcher, rp_dispatcher, RP, DISPATCHER, RpDispatcherBase)

struct _RpDispatcherInterface {
    RpDispatcherBaseInterface parent_iface;

    const char* (*name)(RpDispatcher*);
    //TODO...createFileEvent()?
    RpTimer* (*create_timer)(RpDispatcher*, RpTimerCb, gpointer);
    //TODO...createScaledTimer()?
    RpSchedulableCallback* (*create_schedulable_callback)(RpDispatcher*,
                                                            RpSchedulableCallbackCb,
                                                            gpointer);
    //TODO...registerWatchdog()?
    RpTimeSource* (*time_source)(RpDispatcher*);
    void (*clear_deferred_delete_list)(RpDispatcher*);
    void (*clear_deferred_destroy_list)(RpDispatcher*);
    void (*deferred_delete)(RpDispatcher*, GObject*);
    void (*deferred_destroy)(RpDispatcher*, gpointer, GDestroyNotify);
};

static inline const char*
rp_dispatcher_name(RpDispatcher* self)
{
    return RP_IS_DISPATCHER(self) ?
        RP_DISPATCHER_GET_IFACE(self)->name(self) : NULL;
}
static inline RpTimer*
rp_dispatcher_create_timer(RpDispatcher* self, RpTimerCb cb, gpointer arg)
{
    return RP_IS_DISPATCHER(self) ?
        RP_DISPATCHER_GET_IFACE(self)->create_timer(self, cb, arg) : NULL;
}
static inline RpSchedulableCallback*
rp_dispatcher_create_schedulable_callback(RpDispatcher* self, RpSchedulableCallbackCb cb, gpointer arg)
{
    return RP_IS_DISPATCHER(self) ?
        RP_DISPATCHER_GET_IFACE(self)->create_schedulable_callback(self, cb, arg) :
        NULL;
}
static inline RpTimeSource*
rp_dispatcher_time_source(RpDispatcher* self)
{
    return RP_IS_DISPATCHER(self) ?
        RP_DISPATCHER_GET_IFACE(self)->time_source(self) : NULL;
}
static inline void
rp_dispatcher_clear_deferred_delete_list(RpDispatcher* self)
{
    if (RP_IS_DISPATCHER(self)) \
        RP_DISPATCHER_GET_IFACE(self)->clear_deferred_delete_list(self);
}
static inline void
rp_dispatcher_clear_deferred_destroy_list(RpDispatcher* self)
{
    if (RP_IS_DISPATCHER(self)) \
        RP_DISPATCHER_GET_IFACE(self)->clear_deferred_destroy_list(self);
}
static inline void
rp_dispatcher_deferred_delete(RpDispatcher* self, GObject* obj)
{
    if (RP_IS_DISPATCHER(self)) \
        RP_DISPATCHER_GET_IFACE(self)->deferred_delete(self, obj);
}
static inline void
rp_dispatcher_deferred_destroy(RpDispatcher* self, gpointer mem, GDestroyNotify cb)
{
    if (RP_IS_DISPATCHER(self)) \
        RP_DISPATCHER_GET_IFACE(self)->deferred_destroy(self, mem, cb);
}


#if 0
typedef evthr_t RpDispatcher;

void rp_dispatcher_init(RpDispatcher* self);
void rp_dispatcher_exit(RpDispatcher* self);
void rp_dispatcher_deferred_delete(RpDispatcher* self, GObject* obj);
void rp_dispatcher_deferred_destroy(RpDispatcher* self,
                                    gpointer mem,
                                    GDestroyNotify cb);
#endif//0

G_END_DECLS
