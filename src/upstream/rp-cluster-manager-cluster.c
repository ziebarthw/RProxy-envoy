/*
 * rp-cluster-manager-cluster.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_manager_cluster_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_manager_cluster_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-cluster-manager-impl.h"

G_DEFINE_INTERFACE(RpClusterManagerCluster, rp_cluster_manager_cluster, G_TYPE_OBJECT)
static void
rp_cluster_manager_cluster_default_init(RpClusterManagerClusterInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
