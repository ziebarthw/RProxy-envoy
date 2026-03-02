/*
 * rp-rds-route-config-provider.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-codec.h"
#include "rp-rds-config.h"
#include "rp-route-configuration.h"
#include "rp-time.h"

G_BEGIN_DECLS

/**
 * A provider for constant route configurations.
 */
#define RP_TYPE_RDS_ROUTE_CONFIG_PROVIDER rp_rds_route_config_provider_get_type()
G_DECLARE_INTERFACE(RpRdsRouteConfigProvider, rp_rds_route_config_provider, RP, RDS_ROUTE_CONFIG_PROVIDER, GObject)

typedef struct _RpRdsConfigInfo RpRdsConfigInfo;
struct _RpRdsConfigInfo {
    const RpRouteConfiguration* m_config;
    const char* m_version;
};

struct _RpRdsRouteConfigProviderInterface {
    GTypeInterface parent_interface;

    RpRdsConfigConstSharedPtr (*config)(const RpRdsRouteConfigProvider*);
    const RpRdsConfigInfo* (*config_info)(const RpRdsRouteConfigProvider*);
    RpSystemTime (*last_updated)(const RpRdsRouteConfigProvider*);
    RpStatusCode_e (*on_config_update)(const RpRdsRouteConfigProvider*);
};

typedef UNIQUE_PTR(RpRdsRouteConfigProvider) RpRdsRouteConfigProviderPtr;
typedef SHARED_PTR(RpRdsRouteConfigProvider) RpRdsRouteConfigProviderSharedPtr;

static inline gboolean
rp_rds_route_config_provider_is_a(const RpRdsRouteConfigProvider* self)
{
    g_return_val_if_fail(self != NULL, false);
    return RP_IS_RDS_ROUTE_CONFIG_PROVIDER((RpRdsRouteConfigProvider*)self);
}
static inline RpRdsRouteConfigProviderInterface*
rp_rds_route_config_provider_iface(const RpRdsRouteConfigProvider* self)
{
    return RP_RDS_ROUTE_CONFIG_PROVIDER_GET_IFACE((RpRdsRouteConfigProvider*)self);
}
static inline RpRdsConfigConstSharedPtr
rp_rds_route_config_provider_config(const RpRdsRouteConfigProvider* self)
{
    g_return_val_if_fail(rp_rds_route_config_provider_is_a(self), NULL);
    RpRdsRouteConfigProviderInterface* iface = rp_rds_route_config_provider_iface(self);
    return iface->config(self);
}
static inline const RpRdsConfigInfo*
rp_rds_route_config_provider_config_info(const RpRdsRouteConfigProvider* self)
{
    g_return_val_if_fail(rp_rds_route_config_provider_is_a(self), NULL);
    RpRdsRouteConfigProviderInterface* iface = rp_rds_route_config_provider_iface(self);
    return iface->config_info(self);
}
static inline RpSystemTime
rp_rds_route_config_provider_last_updated(const RpRdsRouteConfigProvider* self)
{
    g_return_val_if_fail(rp_rds_route_config_provider_is_a(self), 0);
    RpRdsRouteConfigProviderInterface* iface = rp_rds_route_config_provider_iface(self);
    return iface->last_updated(self);
}
static inline RpStatusCode_e
rp_rds_route_config_provider_on_config_update(const RpRdsRouteConfigProvider* self)
{
    g_return_val_if_fail(rp_rds_route_config_provider_is_a(self), RpStatusCode_Ok);
    RpRdsRouteConfigProviderInterface* iface = rp_rds_route_config_provider_iface(self);
    return iface->on_config_update(self);
}

G_END_DECLS
