/*
 * rp-cluster-manager.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_cluster_manager_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_manager_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-cluster-manager.h"

G_DEFINE_INTERFACE(RpClusterUpdateCallbacks, rp_cluster_update_callbacks, G_TYPE_OBJECT)
static void
rp_cluster_update_callbacks_default_init(RpClusterUpdateCallbacksInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpClusterManagerFactory, rp_cluster_manager_factory, G_TYPE_OBJECT)
static void
rp_cluster_manager_factory_default_init(RpClusterManagerFactoryInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpClusterManager, rp_cluster_manager, G_TYPE_OBJECT)
static void
rp_cluster_manager_default_init(RpClusterManagerInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpClusterUpdateCallbacksHandle, rp_cluster_update_callbacks_handle, G_TYPE_OBJECT)
static void
rp_cluster_update_callbacks_handle_default_init(RpClusterUpdateCallbacksHandleInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
