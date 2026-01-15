/*
 * rp-resource-manager.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#include "macrologger.h"

#if (defined(rp_resource_manager_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_resource_manager_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-resource-manager.h"

const gsize RpNumResourcePriorities = 2;

G_DEFINE_INTERFACE(RpResourceManager, rp_resource_manager, G_TYPE_OBJECT)

static void
rp_resource_manager_default_init(RpResourceManagerInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
