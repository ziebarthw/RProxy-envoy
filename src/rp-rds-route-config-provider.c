/*
 * rp-rds-route-config-provider.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_rds_route_config_provider_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_rds_route_config_provider_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-rds-route-config-provider.h"

G_DEFINE_INTERFACE(RpRdsRouteConfigProvider, rp_rds_route_config_provider, G_TYPE_OBJECT)
static void
rp_rds_route_config_provider_default_init(RpRdsRouteConfigProviderInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}