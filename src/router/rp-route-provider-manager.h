/*
 * rp-route-provider-manager.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-rds.h"
#include "rp-route-config-provider-manager.h"
#include "rp-singleton-instance.h"
#include "rds/rp-route-config-provider-manager.h"
#include "rds/rp-static-route-config-provider-impl.h"

G_BEGIN_DECLS

#define RP_TYPE_ROUTE_CONFIG_PROVIDER_MANAGER_IMPL rp_route_config_provider_manager_impl_get_type()
G_DECLARE_FINAL_TYPE(RpRouteConfigProviderManagerImpl, rp_route_config_provider_manager_impl, RP, ROUTE_CONFIG_PROVIDER_MANAGER_IMPL, GObject)

typedef UNIQUE_PTR(RpRouteConfigProviderManagerImpl) RpRouteConfigProviderManagerImplPtr;

RpRouteConfigProviderManager* rp_route_config_provider_manager_impl_new(void);

#define RP_TYPE_ROUTE_CONFIG_PROVIDER_MANAGER_FACTORY rp_route_config_provider_manager_factory_get_type()
G_DECLARE_FINAL_TYPE(RpRouteConfigProviderManagerFactory, rp_route_config_provider_manager_factory, RP, ROuTE_CONFIG_PROVIDER_MANAGER_FACTORY, GObject)

RpRouteConfigProviderManagerFactory* rp_route_config_provider_manager_factory_new(RpSingletonManager* singleton_manager);
RpRouteConfigProviderManagerSharedPtr rp_route_config_provider_manager_factory_get(RpRouteConfigProviderManagerFactory* self);

G_END_DECLS
