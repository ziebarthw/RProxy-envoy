/*
 * rp-load-balancer-context-base.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-load-balancer.h"

G_BEGIN_DECLS

#define RP_TYPE_LOAD_BALANCER_CONTEXT_BASE rp_load_balancer_context_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpLoadBalancerContextBase, rp_load_balancer_context_base, RP, LOAD_BALANCER_CONTEXT_BASE, GObject)

struct _RpLoadBalancerContextBaseClass {
    GObjectClass parent_class;


};

G_END_DECLS
