/*
 * rp-cluster.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "upstream/rp-cluster-factory-impl.h"
#include "upstream/rp-upstream-impl.h"
#include "rp-cluster-store.h"
#include "rp-header-utility.h"
#include "rp-http-conn-pool.h"

G_BEGIN_DECLS

/**
 * LoadBalancer implementation.
 */
#define RP_TYPE_DFP_CLUSTER_LOAD_BALANCER rp_dfp_cluster_load_balancer_get_type()
G_DECLARE_FINAL_TYPE(RpDfpClusterLoadBalancer, rp_dfp_cluster_load_balancer, RP, DFP_CLUSTER_LOAD_BALANCER, GObject)

RpDfpClusterLoadBalancer* rp_dfp_cluster_load_balancer_new(RpDfpCluster* cluster);


/**
 * LoadBalancerFactory implementation.
 */
#define RP_TYPE_DFP_CLUSTER_LOAD_BALANCER_FACTORY rp_dfp_cluster_load_balancer_factory_get_type()
G_DECLARE_FINAL_TYPE(RpDfpClusterLoadBalancerFactory, rp_dfp_cluster_load_balancer_factory, RP, DFP_CLUSTER_LOAD_BALANCER_FACTORY, GObject)

RpDfpClusterLoadBalancerFactory* rp_dfp_cluster_load_balancer_factory_new(RpDfpCluster* cluster);


/**
 * ThreadAwareLoadBalancer implementation.
 */
#define RP_TYPE_DFP_CLUSTER_THREAD_AWARE_LOAD_BALANCER rp_dfp_cluster_thread_aware_load_balancer_get_type()
G_DECLARE_FINAL_TYPE(RpDfpClusterThreadAwareLoadBalancer, rp_dfp_cluster_thread_aware_load_balancer, RP, DFP_CLUSTER_THREAD_AWARE_LOAD_BALANCER, GObject)

RpDfpClusterThreadAwareLoadBalancer* rp_dfp_cluster_thread_aware_load_balancer_new(RpDfpCluster* cluster);


/**
 * Implementation of a dynamic forward proxy cluster.
 */
#define RP_TYPE_DFP_CLUSTER_IMPL rp_dfp_cluster_impl_get_type()
G_DECLARE_FINAL_TYPE(RpDfpClusterImpl, rp_dfp_cluster_impl, RP, DFP_CLUSTER_IMPL, RpClusterImplBase)

RpDfpClusterImpl* rp_dfp_cluster_impl_new(const RpClusterCfg* config,
                                          RpClusterFactoryContext* context,
                                          RpStatusCode_e* creation_status);
RpHostSelectionResponse rp_dfp_cluster_impl_choose_host(RpDfpClusterImpl* self,
                                                        const char* host,
                                                        RpLoadBalancerContext* context);
RpHostConstSharedPtr rp_dfp_cluster_impl_find_host_by_name(RpDfpClusterImpl* self, const char* host);


/**
 * ClusterFactory implementation.
 */
#define RP_TYPE_DFP_CLUSTER_FACTORY rp_dfp_cluster_factory_get_type()
G_DECLARE_FINAL_TYPE(RpDfpClusterFactory, rp_dfp_cluster_factory, RP, DFP_CLUSTER_FACTORY, RpClusterFactoryImplBase)

RpDfpClusterFactory* rp_dfp_cluster_factory_new(void);


static inline char*
normalize_host_for_dfp(const char* host, guint16 default_port)
{
    if (rp_header_utility_host_has_port(host))
    {
        return g_strdup(host);
    }
    return g_strdup_printf("%s:%u", host, default_port);
}

G_END_DECLS
