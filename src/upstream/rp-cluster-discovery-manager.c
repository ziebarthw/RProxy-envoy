/*
 * rp-cluster-discovery-manager.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_discovery_manager_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_discovery_manager_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-cluster-discovery-manager.h"

G_DEFINE_INTERFACE(RpClusterLifecycleCallbackHandler, rp_cluster_lifecycle_callback_handler, G_TYPE_OBJECT)
static void
rp_cluster_lifecycle_callback_handler_default_init(RpClusterLifecycleCallbackHandlerInterface* iface)
{
    LOGD("(%p)", iface);
}
