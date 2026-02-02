/*
 * trafficstats.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(trafficstats_NOISY) || defined(ALL_NOISY)) && !defined(NO_trafficstats_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "trafficstats.h"

traffic_stats_t g_traffic_stats = {0};
