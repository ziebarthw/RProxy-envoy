/*
 * rp-static-route-config-provider-impl.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-factory-context.h"
#include "rp-rds-config-traits.h"
#include "rp-rds-route-config-provider.h"
#include "rds/rp-route-config-provider-manager.h"

G_BEGIN_DECLS

#define RP_TYPE_RDS_STATIC_ROUTE_CONFIG_PROVIDER_IMPL rp_rds_static_route_config_provider_impl_get_type()
G_DECLARE_FINAL_TYPE(RpRdsStaticRouteConfigProviderImpl, rp_rds_static_route_config_provider_impl, RP, RDS_STATIC_ROUTE_CONFIG_PROVIDER_IMPL, GObject)

RpRdsStaticRouteConfigProviderImpl* rp_rds_static_route_config_provider_impl_new(const RpRouteConfiguration* route_config_proto,
                                                                                    RpRdsConfigTraits* config_traits,
                                                                                    RpServerFactoryContext* factory_context,
                                                                                    RpRdsRouteConfigProviderManager* route_config_provider_manager);

G_END_DECLS
