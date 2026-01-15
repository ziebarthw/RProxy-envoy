/*
 * rp-upstream.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_upstream_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_upstream_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-upstream.h"

G_DEFINE_INTERFACE(RpHost, rp_host, G_TYPE_OBJECT)
static void
rp_host_default_init(RpHostInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpCluster, rp_cluster, G_TYPE_OBJECT)
static void
rp_cluster_default_init(RpClusterInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpClusterInfo, rp_cluster_info, RP_TYPE_FILTER_CHAIN_FACTORY)
static void
rp_cluster_info_default_init(RpClusterInfoInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpUpstreamLocalAddressSelector, rp_upstream_local_address_selector, G_TYPE_OBJECT)
static void
rp_upstream_local_address_selector_default_init(RpUpstreamLocalAddressSelectorInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpPrioritySet, rp_priority_set, G_TYPE_OBJECT)
static void
rp_priority_set_default_init(RpPrioritySetInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpHostSet, rp_host_set, G_TYPE_OBJECT)
static void
rp_host_set_default_init(RpHostSetInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
