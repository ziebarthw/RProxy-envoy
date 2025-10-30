/*
 * rp-resource.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_resource_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_resource_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-resource.h"

G_DEFINE_INTERFACE(RpResourceLimit, rp_resource_limit, G_TYPE_OBJECT)

static void
rp_resource_limit_default_init(RpResourceLimitInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
