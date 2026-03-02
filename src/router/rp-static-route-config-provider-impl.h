/*
 * rp-static-route-config-provider-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-factory-context.h"
#include "rp-rds-config-traits.h"
#include "rp-route-config-provider-manager.h"
#include "rds/rp-static-route-config-provider-impl.h"

G_BEGIN_DECLS

/**
 * Implementation of RouteConfigProvider that holds a static route configuration.
 */
#define RP_TYPE_STATIC_ROUTE_CONFIG_PROVIDER_IMPL rp_static_route_config_provider_impl_get_type()
G_DECLARE_FINAL_TYPE(RpStaticRouteConfigProviderImpl, rp_static_route_config_provider_impl, RP, STATIC_ROUTE_CONFIG_PROVIDER_IMPL, GObject)

RpStaticRouteConfigProviderImpl* rp_static_route_config_provider_impl_new(const RpRouteConfiguration* route_config_proto,
                                                                            RpRdsConfigTraits* config_traits,
                                                                            RpServerFactoryContext* factory_context,
                                                                            RpRdsRouteConfigProviderManager* route_config_provider_manager);

G_END_DECLS
