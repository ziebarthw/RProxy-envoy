/*
 * rp-route-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-router.h"
#include "rproxy.h"

G_BEGIN_DECLS

#define RP_TYPE_ROUTE_IMPL rp_route_impl_get_type()
G_DECLARE_FINAL_TYPE(RpRouteImpl, rp_route_impl, RP, ROUTE_IMPL, GObject)

RpRouteImpl* rp_route_impl_new(SHARED_PTR(RpRouteConfig) parent, SHARED_PTR(rule_cfg_t) rule_cfg);
SHARED_PTR(rule_cfg_t) rp_route_impl_get_rule_cfg(RpRouteImpl* self);

G_END_DECLS
