/*
 * rp-cluster-factory.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_cluster_factory_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_factory_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-cluster-factory.h"

G_DEFINE_INTERFACE(RpClusterFactoryContext, rp_cluster_factory_context, G_TYPE_OBJECT)
static void
rp_cluster_factory_context_default_init(RpClusterFactoryContextInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpClusterFactory, rp_cluster_factory, G_TYPE_OBJECT)
static void
rp_cluster_factory_default_init(RpClusterFactoryInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
