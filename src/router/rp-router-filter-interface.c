/*
 * rp-router-filter-interface.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_router_filter_interface_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_router_filter_interface_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-router-filter-interface.h"

G_DEFINE_INTERFACE(RpRouterFilterInterface, rp_router_filter_interface, G_TYPE_OBJECT)

static void
rp_router_filter_interface_default_init(RpRouterFilterInterfaceInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
