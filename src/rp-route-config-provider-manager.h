/*
 * rp-route-config-provider-manager.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-rds.h"
#include "rp-cluster-manager.h"
#include "rp-factory-context.h"

G_BEGIN_DECLS

/**
 * The RouteConfigProviderManager exposes the ability to get a RouteConfigProvider. This interface
 * is exposed to the Server's FactoryContext in order to allow HttpConnectionManagers to get
 * RouteConfigProviders.
 */
#define RP_TYPE_ROUTE_CONFIG_PROVIDER_MANAGER rp_route_config_provider_manager_get_type()
G_DECLARE_INTERFACE(RpRouteConfigProviderManager, rp_route_config_provider_manager, RP, ROUTE_CONFIG_PROVIDER_MANAGER, GObject)

struct _RpRouteConfigProviderManagerInterface {
    GTypeInterface parent_iface;

    RpRouteConfigProvider* (*create_static_route_config_provider)(RpRouteConfigProviderManager*, void*, RpServerFactoryContext*);
};

static inline RpRouteConfigProvider*
rp_route_config_provider_manager_create_static_route_config_provider(RpRouteConfigProviderManager* self, void* config, RpServerFactoryContext* factory_context)
{
    return RP_IS_ROUTE_CONFIG_PROVIDER_MANAGER(self) ?
        RP_ROUTE_CONFIG_PROVIDER_MANAGER_GET_IFACE(self)->create_static_route_config_provider(self, config, factory_context) :
        NULL;
}

G_END_DECLS
