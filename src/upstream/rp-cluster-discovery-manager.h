/*
 * rp-cluster-discovery-manager.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-cluster-manager.h"

G_BEGIN_DECLS

/**
 * A base class for cluster lifecycle handler. Mostly to avoid a dependency on
 * ThreadLocalClusterManagerImpl in ClusterDiscoveryManager.
 */
#define RP_TYPE_CLUSTER_LIFECYCLE_CALLBACK_HANDLER rp_cluster_lifecycle_callback_handler_get_type()
G_DECLARE_INTERFACE(RpClusterLifecycleCallbackHandler, rp_cluster_lifecycle_callback_handler, RP, CLUSTER_LIFECYCLE_CALLBACK_HANDLER, GObject)

struct _RpClusterLifecycleCallbackHandlerInterface {
    GTypeInterface parent_iface;

    RpClusterUpdateCallbacksHandlePtr (*add_cluster_update_callbacks)(RpClusterLifecycleCallbackHandler*,
                                                                        RpClusterUpdateCallbacks*);
};

static inline RpClusterUpdateCallbacksHandlePtr
rp_cluster_lifecycle_callback_handler_add_cluster_update_callbacks(RpClusterLifecycleCallbackHandler* self, RpClusterUpdateCallbacks* cb)
{
    return RP_CLUSTER_LIFECYCLE_CALLBACK_HANDLER(self) ?
        RP_CLUSTER_LIFECYCLE_CALLBACK_HANDLER_GET_IFACE(self)->add_cluster_update_callbacks(self, cb) :
        NULL;
}

G_END_DECLS
