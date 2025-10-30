/*
 * rp-route-config-provider-manager.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_route_config_provider_manager_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_route_config_provider_manager_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-route-config-provider-manager.h"

G_DEFINE_INTERFACE(RpRouteConfigProviderManager, rp_route_config_provider_manager, G_TYPE_OBJECT)
static void
rp_route_config_provider_manager_default_init(RpRouteConfigProviderManagerInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
