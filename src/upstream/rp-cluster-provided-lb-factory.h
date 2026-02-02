/*
 * rp-cluster-provided-lb-factory.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "upstream/rp-load-balancer-factory-base.h"

G_BEGIN_DECLS

#define RP_TYPE_CLUSTER_PROVIDED_LB_CONFIG rp_cluster_provided_lb_config_get_type()
G_DECLARE_FINAL_TYPE(RpClusterProvidedLbConfig, rp_cluster_provided_lb_config, RP, CLUSTER_PROVIDED_LB_CONFIG, GObject)

RpLoadBalancerConfigPtr rp_cluster_provided_lb_config_new(void);


#define RP_TYPE_CLUSTER_PROVIDED_LB_FACTORY rp_cluster_provided_lb_factory_get_type()
G_DECLARE_FINAL_TYPE(RpClusterProvidedLbFactory, rp_cluster_provided_lb_factory, RP, CLUSTER_PROVIDED_LB_FACTORY, RpTypedLoadBalancerFactoryBase)

RpTypedLoadBalancerFactory* rp_cluster_provided_lb_factory_new(void);

G_END_DECLS
