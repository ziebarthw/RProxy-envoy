/*
 * rp-route-config-provider.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"
#include "rp-codec.h"
#include "rp-route-configuration.h"

G_BEGIN_DECLS

typedef struct _RpRouteConfig RpRouteConfig;

typedef struct _RpRdsConfigInfo RpRdsConfigInfo;
struct _RpRdsConfigInfo {
    RpRouteConfiguration m_config;
    const char* m_name;
};
static inline RpRdsConfigInfo
rp_rds_config_info_ctor(RpRouteConfiguration config, const char* name)
{
    RpRdsConfigInfo self = {
        .m_config = config,
        .m_name = name
    };
    return self;
}

/**
 * A provider for constant route configurations.
 */
#define RP_TYPE_RDS_ROUTE_CONFIG_PROVIDER rp_rds_route_config_provider_get_type()
G_DECLARE_INTERFACE(RpRdsRouteConfigProvider, rp_rds_route_config_provider, RP, RDS_ROUTE_CONFIG_PROVIDER, GObject)

struct _RpRdsRouteConfigProviderInterface {
    GTypeInterface parent_iface;

    RpRouteConfig* (*config)(RpRdsRouteConfigProvider*);
    RpRdsConfigInfo* (*config_info)(RpRdsRouteConfigProvider*);
    gint64 (*last_updated)(RpRdsRouteConfigProvider*);
    RpStatusCode_e (*on_config_update)(RpRdsRouteConfigProvider*);
};

static inline RpRouteConfig*
rp_rds_route_config_provider_config(RpRdsRouteConfigProvider* self)
{
    return RP_IS_RDS_ROUTE_CONFIG_PROVIDER(self) ?
        RP_RDS_ROUTE_CONFIG_PROVIDER_GET_IFACE(self)->config(self) : NULL;
}
static inline RpRdsConfigInfo*
rp_rds_route_config_provider_config_info(RpRdsRouteConfigProvider* self)
{
    return RP_IS_RDS_ROUTE_CONFIG_PROVIDER(self) ?
        RP_RDS_ROUTE_CONFIG_PROVIDER_GET_IFACE(self)->config_info(self) : NULL;
}
static inline gint64
rp_rds_route_config_provider_last_updated(RpRdsRouteConfigProvider* self)
{
    return RP_IS_RDS_ROUTE_CONFIG_PROVIDER(self) ?
        RP_RDS_ROUTE_CONFIG_PROVIDER_GET_IFACE(self)->last_updated(self) : -1;
}
static inline RpStatusCode_e
rp_rds_route_config_provider_on_config_update(RpRdsRouteConfigProvider* self)
{
    return RP_IS_RDS_ROUTE_CONFIG_PROVIDER(self) ?
        RP_RDS_ROUTE_CONFIG_PROVIDER_GET_IFACE(self)->on_config_update(self) :
        RpStatusCode_Ok;
}

G_END_DECLS
