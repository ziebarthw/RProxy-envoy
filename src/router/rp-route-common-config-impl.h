/*
 * rp-route-common-config-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-factory-context.h"
#include "rp-route-configuration.h"
#include "rp-router.h"

G_BEGIN_DECLS

/**
 * Shared part of the route configuration implementation.
 */
#define RP_TYPE_ROUTE_COMMON_CONFIG_IMPL rp_route_common_config_impl_get_type()
G_DECLARE_FINAL_TYPE(RpRouteCommonConfigImpl, rp_route_common_config_impl, RP, ROUTE_COMMON_CONFIG_IMPL, GObject)

RpRouteCommonConfigImpl* rp_route_common_config_impl_create(RpRouteConfiguration* config,
                                                            RpServerFactoryContext* factory_context);
const char* rp_route_common_config_impl_name(RpRouteCommonConfigImpl* self);
guint32 rp_route_common_config_impl_max_direct_response_body_size_bytes(RpRouteCommonConfigImpl* self);

G_END_DECLS
