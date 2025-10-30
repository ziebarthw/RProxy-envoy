/*
 * rp-time.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_time_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_time_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <stdio.h>
#include "rp-time.h"

G_DEFINE_INTERFACE(RpTimeSource, rp_time_source, G_TYPE_OBJECT)
static void
rp_time_source_default_init(RpTimeSourceInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
