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
#include "rproxy.h"
#include "rp-listen-socket.h"
#include "rp-signal.h"
#include "rp-time.h"
#include "rp-timer.h"

G_BEGIN_DECLS

typedef struct _RpNetworkTransportSocket* RpNetworkTransportSocketPtr;
typedef struct _RpNetworkServerConnection* RpNetworkServerConnectionPtr;
typedef struct _RpNetworkClientConnection* RpNetworkClientConnectionPtr;
typedef struct _RpStreamInfo RpStreamInfo;

/**
 * Callback invoked when a dispatcher post() runs.
 */
typedef void (*RpPostCb)(gpointer);

typedef enum {
    RpRunType_BLOCK,
    RpRunType_NON_BLOCK,
    RpRunType_RUN_UNTIL_EXIT
} RpRunType_e;

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
    RpNetworkServerConnectionPtr (*create_server_connection)(RpDispatcher*,
                                                                RpSocketPtr,
                                                                RpNetworkTransportSocketPtr,
                                                                RpStreamInfo*);
    RpNetworkClientConnectionPtr (*create_client_connection)(RpDispatcher*,
                                                                RpNetworkAddressInstanceConstSharedPtr,
                                                                RpNetworkAddressInstanceConstSharedPtr,
                                                                RpNetworkTransportSocketPtr);
    void (*exit)(RpDispatcher*);
    RpSignalEventPtr (*listen_for_signal)(RpDispatcher*,
                                            signal_t,
                                            RpSignalCb,
                                            gpointer);
//TODO...virtual void deleteInDispatcherThread(DispatcherThradDeletableConstPtr deletable) PURE;
    void (*run)(RpDispatcher*, RpRunType_e);
    //TODO...
    void (*shutdown)(RpDispatcher*);

    // Custom...
    evthr_t* (*thr)(RpDispatcher*);
    evbase_t* (*base)(RpDispatcher*);
    evdns_base_t* (*dns_base)(RpDispatcher*);
};

typedef UNIQUE_PTR(RpDispatcher) RpDispatcherPtr;

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
static inline RpNetworkServerConnectionPtr
rp_dispatcher_create_server_connection(RpDispatcher* self, RpSocketPtr socket, RpNetworkTransportSocketPtr transport_socket, RpStreamInfo* stream_info)
{
    return RP_IS_DISPATCHER(self) ?
        RP_DISPATCHER_GET_IFACE(self)->create_server_connection(self, socket, transport_socket, stream_info) :
        NULL;
}
static inline RpNetworkClientConnectionPtr
rp_dispatcher_create_client_connection(RpDispatcher* self, RpNetworkAddressInstanceConstSharedPtr address, RpNetworkAddressInstanceConstSharedPtr source_address, RpNetworkTransportSocketPtr transport_socket)
{
    return RP_IS_DISPATCHER(self) ?
        RP_DISPATCHER_GET_IFACE(self)->create_client_connection(self, address, source_address, transport_socket) :
        NULL;
}
static inline void
rp_dispatcher_exit(RpDispatcher* self)
{
    if (RP_IS_DISPATCHER(self)) \
        RP_DISPATCHER_GET_IFACE(self)->exit(self);
}
static inline RpSignalEventPtr
rp_dispatcher_listen_for_signal(RpDispatcher* self, signal_t signal_num, RpSignalCb cb, gpointer arg)
{
    return RP_IS_DISPATCHER(self) ?
        RP_DISPATCHER_GET_IFACE(self)->listen_for_signal(self, signal_num, cb, arg) :
        NULL;
}
static inline void
rp_dispatcher_run(RpDispatcher* self, RpRunType_e type)
{
    if (RP_IS_DISPATCHER(self)) \
        RP_DISPATCHER_GET_IFACE(self)->run(self, type);
}
static inline void
rp_dispatcher_shutdown(RpDispatcher* self)
{
    if (RP_IS_DISPATCHER(self)) \
        RP_DISPATCHER_GET_IFACE(self)->shutdown(self);
}
static inline evthr_t*
rp_dispatcher_thr(RpDispatcher* self)
{
    return RP_IS_DISPATCHER(self) ?
        RP_DISPATCHER_GET_IFACE(self)->thr(self) : NULL;
}
static inline evbase_t*
rp_dispatcher_base(RpDispatcher* self)
{
    return RP_IS_DISPATCHER(self) ?
        RP_DISPATCHER_GET_IFACE(self)->base(self) : NULL;
}
static inline evdns_base_t*
rp_dispatcher_dns_base(RpDispatcher* self)
{
    return RP_IS_DISPATCHER(self) ?
        RP_DISPATCHER_GET_IFACE(self)->dns_base(self) : NULL;
}


G_END_DECLS
