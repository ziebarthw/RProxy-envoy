/*
 * rp-load-balancer-factory-base.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-load-balancer.h"

G_BEGIN_DECLS

/**
 * Base class for cluster provided load balancers and load balancers specified by load balancing
 * policy config. This class should be extended directly if the load balancing policy specifies a
 * thread-aware load balancer.
 *
 * TODO: provide a ThreadLocalLoadBalancer construct to abstract away thread-awareness from load
 * balancing extensions that don't require it.
 */
#define RP_TYPE_TYPED_LOAD_BALANCER_FACTORY_BASE rp_typed_load_balancer_factory_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpTypedLoadBalancerFactoryBase, rp_typed_load_balancer_factory_base, RP, TYPED_LOAD_BALANCER_FACTORY_BASE, GObject)

struct _RpTypedLoadBalancerFactoryBaseClass {
    GObjectClass parent_class;
    
};

G_END_DECLS
