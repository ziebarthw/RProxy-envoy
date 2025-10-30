/*
 * rp-local-info.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_local_info_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_local_info_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <stdio.h>
#include "rp-local-info.h"

G_DEFINE_INTERFACE(RpLocalInfo, rp_local_info, G_TYPE_OBJECT)
static void
rp_local_info_default_init(RpLocalInfoInterface* iface)
{
    LOGD("(%p)", iface);
}
