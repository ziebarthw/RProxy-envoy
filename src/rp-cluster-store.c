/*
 * rp-cluster-store.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_store_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_store_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-cluster-store.h"

G_DEFINE_INTERFACE(RpDfpLb, rp_dfp_lb, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpDfpCluster, rp_dfp_cluster, G_TYPE_OBJECT)

static void
rp_dfp_lb_default_init(RpDfpLbInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

static void
rp_dfp_cluster_default_init(RpDfpClusterInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
