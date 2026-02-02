/*
 * rp-static-cluster.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "upstream/rp-cluster-factory-impl.h"
#include "upstream/rp-upstream-impl.h"

G_BEGIN_DECLS

typedef struct _RpStaticClusterFactory RpStaticClusterFactory;

/**
 * Implementation of Upstream::Cluster for static clusters (clusters that have a fixed number of
 * hosts with resolved IP addresses).
 */
#define RP_TYPE_STATIC_CLUSTER_IMPL rp_static_cluster_impl_get_type()
G_DECLARE_FINAL_TYPE(RpStaticClusterImpl, rp_static_cluster_impl, RP, STATIC_CLUSTER_IMPL, RpClusterImplBase)

RpStaticClusterImpl* rp_static_cluster_impl_new(const RpClusterCfg* cluster,
                                                RpClusterFactoryContext* context,
                                                RpStatusCode_e* creation_status);
PairClusterSharedPtrThreadAwareLoadBalancerPtr rp_static_cluster_impl_create(const RpClusterCfg* cluster,
                                                                                RpClusterFactoryContext* context);


/**
 * Factory for StaticClusterImpl cluster.
 */
#define RP_TYPE_STATIC_CLUSTER_FACTORY rp_static_cluster_factory_get_type()
G_DECLARE_FINAL_TYPE(RpStaticClusterFactory, rp_static_cluster_factory, RP, STATIC_CLUSTER_FACTORY, RpClusterFactoryImplBase)

RpStaticClusterFactory* rp_static_cluster_factory_new(void);


G_END_DECLS
