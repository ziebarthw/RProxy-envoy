/*
 * rp-static-cluster.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-cluster-configuration.h"
#include "upstream/rp-cluster-factory-impl.h"

G_BEGIN_DECLS

/**
 * Implementation of Upstream::Cluster for static clusters (clusters that have a fixed number of
 * hosts with resolved IP addresses).
 */
#define RP_TYPE_STATIC_CLUSTER_IMPL rp_static_cluster_impl_get_type()
G_DECLARE_DERIVABLE_TYPE(RpStaticClusterImpl, rp_static_cluster_impl, RP, STATIC_CLUSTER_IMPL, RpClusterImplBase)

struct _RpStaticClusterImplClass {
    RpClusterImplBaseClass parent_class;
};

RpStaticClusterImpl* rp_static_cluster_impl_new(RpClusterCfg* config,
                                                RpClusterFactoryContext* context,
                                                RpStatusCode_e* creation_status);
void rp_static_cluster_impl_add_downstream(RpStaticClusterImpl* self,
                                            downstream_t* downstream,
                                            RpHost* host);

G_END_DECLS
