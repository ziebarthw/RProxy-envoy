/*
 * rp-rds.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"
#include "rp-rds-route-config-provider.h"
#include "rp-router.h"

G_BEGIN_DECLS

/**
 * A provider for constant route configurations.
 */
#define RP_TYPE_ROUTE_CONFIG_PROVIDER rp_route_config_provider_get_type()
G_DECLARE_INTERFACE(RpRouteConfigProvider, rp_route_config_provider, RP, ROUTE_CONFIG_PROVIDER, GObject)

struct _RpRouteConfigProviderInterface {
    GTypeInterface parent_iface;
    // RpRdsRouteConfigProviderInterface

    RpRouterConfigConstSharedPtr (*config_cast)(RpRouteConfigProvider*);
};

typedef UNIQUE_PTR(RpRouteConfigProvider) RpRouteConfigProviderPtr;
typedef SHARED_PTR(RpRouteConfigProvider) RpRouteConfigProviderSharedPtr;

static inline RpRouterConfigConstSharedPtr
rp_route_config_provider_config_cast(RpRouteConfigProvider* self)
{
    return RP_IS_ROUTE_CONFIG_PROVIDER(self) ?
        RP_ROUTE_CONFIG_PROVIDER_GET_IFACE(self)->config_cast(self) : NULL;
}

G_END_DECLS
