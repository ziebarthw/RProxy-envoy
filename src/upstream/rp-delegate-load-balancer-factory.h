/*
 * rp-delegate-load-balancer-factory.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-load-balancer.h"
#include "rp-upstream.h"

G_BEGIN_DECLS

typedef RpLoadBalancerPtr (*RpLbCreateFn)(RpClusterSharedPtr, RpLoadBalancerParams*);

#define RP_TYPE_DELEGATE_LOAD_BALANCER_FACTORY rp_delegate_load_balancer_factory_get_type()
G_DECLARE_FINAL_TYPE(RpDelegateLoadBalancerFactory, rp_delegate_load_balancer_factory, RP, DELEGATE_LOAD_BALANCER_FACTORY, GObject)

RpLoadBalancerFactory* rp_delegate_load_balancer_factory_new(RpClusterSharedPtr parent,
                                                                RpLbCreateFn create_fn,
                                                                bool recreate_on_host_change);

G_END_DECLS
