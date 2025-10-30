/*
 * rp-rds.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_rds_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_rds_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-rds.h"

G_DEFINE_INTERFACE(RpRouteConfigProvider, rp_route_config_provider, RP_TYPE_RDS_ROUTE_CONFIG_PROVIDER)
static void
rp_route_config_provider_default_init(RpRouteConfigProviderInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
