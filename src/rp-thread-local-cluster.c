/*
 * rp-thread-local-cluster.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_thread_local_cluster_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_thread_local_cluster_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-thread-local-cluster.h"

G_DEFINE_INTERFACE(RpThreadLocalCluster, rp_thread_local_cluster, G_TYPE_OBJECT)
static void
rp_thread_local_cluster_default_init(RpThreadLocalClusterInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
