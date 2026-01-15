/*
 * rp-conn-pool.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-host-description.h"

G_BEGIN_DECLS

typedef struct _RpConnectionPoolInstance RpConnectionPoolInstance;
typedef void (*RpIdleCb)(RpConnectionPoolInstance*);

/**
 * Controls the behavior of a canceled stream.
 */
typedef enum {
    RpCancelPolicy_Default,
    RpCancelPolicy_CloseExcess,
} RpCancelPolicy_e;

/**
 * Controls the behavior when draining a connection pool.
 */
typedef enum {
    RpDrainBehavior_Undefined,
    RpDrainBehavior_DrainAndDelete,
    RpDrainBehavior_DrainExistingConnections,
} RpDrainBehavior_e;

typedef enum {
    RpPoolFailureReason_Overflow,
    RpPoolFailureReason_LocalConnectionFailure,
    RpPoolFailureReason_RemoteConnectionFailure,
    RpPoolFailureReason_Timeout,
} RpPoolFailureReason_e;


/**
 * Handle that allows a pending connection or stream to be canceled before it is completed.
 */
#define RP_TYPE_CANCELLABLE rp_cancellable_get_type()
G_DECLARE_INTERFACE(RpCancellable, rp_cancellable, RP, CANCELLABLE, GObject)

struct _RpCancellableInterface {
    GTypeInterface parent_iface;

    void (*cancel)(RpCancellable*, RpCancelPolicy_e);
};

static inline void
rp_cancellable_cancel(RpCancellable* self, RpCancelPolicy_e cancel_policy)
{
    if (RP_IS_CANCELLABLE(self))
    {
        RP_CANCELLABLE_GET_IFACE(self)->cancel(self, cancel_policy);
    }
}


/**
 * An instance of a generic connection pool.
 */
#define RP_TYPE_CONNECTION_POOL_INSTANCE rp_connection_pool_instance_get_type()
G_DECLARE_INTERFACE(RpConnectionPoolInstance, rp_connection_pool_instance, RP, CONNECTION_POOL_INSTANCE, GObject)

struct _RpConnectionPoolInstanceInterface {
    GTypeInterface parent_iface;

    void (*add_idle_callback)(RpConnectionPoolInstance*, RpIdleCb);
    bool (*is_idle)(RpConnectionPoolInstance*);
    void (*drain_connections)(RpConnectionPoolInstance*, RpDrainBehavior_e);
    RpHostDescription* (*host)(RpConnectionPoolInstance*);
    bool (*maybe_preconnect)(RpConnectionPoolInstance*, float);
};

static inline void
rp_connection_pool_instance_add_idle_callback(RpConnectionPoolInstance* self, RpIdleCb cb)
{
    if (RP_IS_CONNECTION_POOL_INSTANCE(self))
    {
        RP_CONNECTION_POOL_INSTANCE_GET_IFACE(self)->add_idle_callback(self, cb);
    }
}
static inline bool
rp_connection_pool_instance_is_idle(RpConnectionPoolInstance* self)
{
    return RP_IS_CONNECTION_POOL_INSTANCE(self) ?
        RP_CONNECTION_POOL_INSTANCE_GET_IFACE(self)->is_idle(self) : false;
}
static inline void
rp_connection_pool_instance_drain_connections(RpConnectionPoolInstance* self, RpDrainBehavior_e drain_behavior)
{
    if (RP_IS_CONNECTION_POOL_INSTANCE(self))
    {
        RP_CONNECTION_POOL_INSTANCE_GET_IFACE(self)->drain_connections(self, drain_behavior);
    }
}
static inline RpHostDescription*
rp_connection_pool_instance_host(RpConnectionPoolInstance* self)
{
    return RP_IS_CONNECTION_POOL_INSTANCE(self) ?
        RP_CONNECTION_POOL_INSTANCE_GET_IFACE(self)->host(self) : NULL;
}
static inline bool
rp_connection_pool_instance_maybe_preconnect(RpConnectionPoolInstance* self, float preconnect_ratio)
{
    return RP_IS_CONNECTION_POOL_INSTANCE(self) ?
        RP_CONNECTION_POOL_INSTANCE_GET_IFACE(self)->maybe_preconnect(self, preconnect_ratio) :
        false;
}

G_END_DECLS
