/*
 * rp-route-config-provider-manager.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-rds-config-traits.h"
#include "rp-rds-route-config-provider.h"

G_BEGIN_DECLS

#define RP_TYPE_RDS_ROUTE_CONFIG_PROVIDER_MANAGER rp_rds_route_config_provider_manager_get_type()
G_DECLARE_FINAL_TYPE(RpRdsRouteConfigProviderManager, rp_rds_route_config_provider_manager, RP, RDS_ROUTE_CONFIG_PROVIDER_MANAGER, GObject)

RpRdsRouteConfigProviderManager* rp_rds_route_config_provider_manager_new(void);
void rp_rds_route_config_provider_manager_erase_static_provider(RpRdsRouteConfigProviderManager* self,
                                                                RpRdsRouteConfigProvider* provider);
RpRdsRouteConfigProviderPtr rp_rds_route_config_provider_manager_add_static_provider(RpRdsRouteConfigProviderManager* self,
                                                                                        RpRdsRouteConfigProviderPtr (*create_static_provider)(gpointer),
                                                                                        gpointer arg);

G_END_DECLS
