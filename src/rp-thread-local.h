/*
 * rp-thread-local.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-dispatcher.h"
#include "rp-thread-local-object.h"

G_BEGIN_DECLS

/**
 * An individual allocated TLS slot. When the slot is destroyed the stored thread local will
 * be freed on each thread.
 */
#define RP_TYPE_SLOT rp_slot_get_type()
G_DECLARE_INTERFACE(RpSlot, rp_slot, RP, SLOT, GObject)

typedef RpThreadLocalObjectSharedPtr (*RpInitializeCb)(RpDispatcher*, gpointer);
typedef void (*RpUpdateCb)(RpThreadLocalObjectSharedPtr, gpointer);

struct _RpSlotInterface {
    GTypeInterface parent_iface;

    bool (*current_thread_registered)(RpSlot*);
    RpThreadLocalObjectSharedPtr (*get)(RpSlot*);
    /**
     * Set thread local data on all threads previously registered via registerThread().
     * @param initializeCb supplies the functor that will be called *on each thread*. The functor
     *                     returns the thread local object which is then stored. The storage is via
     *                     a shared_ptr. Thus, this is a flexible mechanism that can be used to share
     *                     the same data across all threads or to share different data on each thread.
     *
     * NOTE: The initialize callback is not supposed to capture the Slot, or its owner, as the owner
     * may be destructed in main thread before the update_cb gets called in a worker thread.
     */
    void (*set)(RpSlot*, RpInitializeCb, gpointer);
    void (*run_on_all_threads)(RpSlot*, RpUpdateCb, gpointer);
    void (*run_on_all_threads_completed)(RpSlot*,
                                        RpUpdateCb,
                                        const void(*complete_cb)(gpointer),
                                        gpointer);
    bool (*is_shutdown)(RpSlot*);
};

typedef UNIQUE_PTR(RpSlot) RpSlotPtr;

static inline bool
rp_slot_current_thread_registered(RpSlot* self)
{
    return RP_IS_SLOT(self) ?
        RP_SLOT_GET_IFACE(self)->current_thread_registered(self) : false;
}
static inline RpThreadLocalObjectSharedPtr
rp_slot_get(RpSlot* self)
{
    return RP_IS_SLOT(self) ? RP_SLOT_GET_IFACE(self)->get(self) : NULL;
}
static inline void
rp_slot_set(RpSlot* self, RpInitializeCb cb, gpointer arg)
{
    if (RP_IS_SLOT(self)) \
        RP_SLOT_GET_IFACE(self)->set(self, cb, arg);
}
static inline void
rp_slot_run_on_all_threads(RpSlot* self, RpUpdateCb cb, gpointer arg)
{
    if (RP_IS_SLOT(self)) \
        RP_SLOT_GET_IFACE(self)->run_on_all_threads(self, cb, arg);
}
static inline void
rp_slot_run_on_all_threads_completed(RpSlot* self, RpUpdateCb cb, const void(*complete_cb)(gpointer), gpointer arg)
{
    if (RP_IS_SLOT(self)) \
        RP_SLOT_GET_IFACE(self)->run_on_all_threads_completed(self, cb, complete_cb, arg);
}
static inline bool
rp_slot_is_shutdown(RpSlot* self)
{
    return RP_IS_SLOT(self) ? RP_SLOT_GET_IFACE(self)->is_shutdown(self) : true;
}


/**
 * Interface used to allocate thread local slots.
 */
#define RP_TYPE_SLOT_ALLOCATOR rp_slot_allocator_get_type()
G_DECLARE_INTERFACE(RpSlotAllocator, rp_slot_allocator, RP, SLOT_ALLOCATOR, GObject)

struct _RpSlotAllocatorInterface {
    GTypeInterface parent_iface;

    RpSlotPtr (*allocate_slot)(RpSlotAllocator*);
};

static inline RpSlotPtr
rp_slot_allocator_allocate_slot(RpSlotAllocator* self)
{
    return RP_IS_SLOT_ALLOCATOR(self) ?
        RP_SLOT_ALLOCATOR_GET_IFACE(self)->allocate_slot(self) : NULL;
}


/**
 * Interface for getting and setting thread local data as well as registering a thread
 */
#define RP_TYPE_THREAD_LOCAL_INSTANCE rp_thread_local_instance_get_type()
G_DECLARE_INTERFACE(RpThreadLocalInstance, rp_thread_local_instance, RP, THREAD_LOCAL_INSTANCE, RpSlotAllocator)

struct _RpThreadLocalInstanceInterface {
    RpSlotAllocatorInterface parent_iface;

    void (*register_thread)(RpThreadLocalInstance*, RpDispatcher*, bool);
    void (*shutdown_global_threading)(RpThreadLocalInstance*);
    void (*shutdown_thread)(RpThreadLocalInstance*);
    RpDispatcher* (*dispatcher)(RpThreadLocalInstance*);
    bool (*is_shutdown)(RpThreadLocalInstance*);
};

static inline void
rp_thread_local_instance_register_thread(RpThreadLocalInstance* self, RpDispatcher* dispatcher, bool main_thread)
{
    if (RP_IS_THREAD_LOCAL_INSTANCE(self)) \
        RP_THREAD_LOCAL_INSTANCE_GET_IFACE(self)->register_thread(self, dispatcher, main_thread);
}
static inline void
rp_thread_local_instance_shutdown_global_threading(RpThreadLocalInstance* self)
{
    if (RP_IS_THREAD_LOCAL_INSTANCE(self)) \
        RP_THREAD_LOCAL_INSTANCE_GET_IFACE(self)->shutdown_global_threading(self);
}
static inline void
rp_thread_local_instance_shutdown_thread(RpThreadLocalInstance* self)
{
    if (RP_IS_THREAD_LOCAL_INSTANCE(self)) \
        RP_THREAD_LOCAL_INSTANCE_GET_IFACE(self)->shutdown_thread(self);
}
static inline RpDispatcher*
rp_thread_local_instance_dispatcher(RpThreadLocalInstance* self)
{
    return RP_IS_THREAD_LOCAL_INSTANCE(self) ?
        RP_THREAD_LOCAL_INSTANCE_GET_IFACE(self)->dispatcher(self) : NULL;
}
static inline bool
rp_thread_local_instance_is_shutdown(RpThreadLocalInstance* self)
{
    return RP_IS_THREAD_LOCAL_INSTANCE(self) ?
        RP_THREAD_LOCAL_INSTANCE_GET_IFACE(self)->is_shutdown(self) : true;
}

G_END_DECLS
