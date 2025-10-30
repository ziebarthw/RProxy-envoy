/*
 * rp-rds.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-route-config-provider.h"

G_BEGIN_DECLS

typedef struct _RpRouteConfig RpRouteConfig;

/**
 * A provider for constant route configurations.
 */
#define RP_TYPE_ROUTE_CONFIG_PROVIDER rp_route_config_provider_get_type()
G_DECLARE_INTERFACE(RpRouteConfigProvider, rp_route_config_provider, RP, ROUTE_CONFIG_PROVIDER, RpRdsRouteConfigProvider)

struct _RpRouteConfigProviderInterface {
    RpRdsRouteConfigProviderInterface parent_iface;

    RpRouteConfig* (*config_cast)(RpRouteConfigProvider*);
};

static inline RpRouteConfig*
rp_route_config_provider_config_cast(RpRouteConfigProvider* self)
{
    return RP_IS_ROUTE_CONFIG_PROVIDER(self) ?
        RP_ROUTE_CONFIG_PROVIDER_GET_IFACE(self)->config_cast(self) : NULL;
}

G_END_DECLS
