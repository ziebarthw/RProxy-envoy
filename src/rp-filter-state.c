/*
 * rp-filter-state.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_filter_state_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_filter_state_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-filter-state.h"

G_DEFINE_INTERFACE(RpFilterState, rp_filter_state, G_TYPE_OBJECT)

static void
rp_filter_state_default_init(RpFilterStateInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
