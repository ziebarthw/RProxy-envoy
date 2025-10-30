/*
 * rp-timer.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_timer_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_timer_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <stdio.h>
#include "rp-timer.h"

G_DEFINE_INTERFACE(RpTimer, rp_timer, G_TYPE_OBJECT)
static void
rp_timer_default_init(RpTimerInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpScheduler, rp_scheduler, G_TYPE_OBJECT)
static void
rp_scheduler_default_init(RpSchedulerInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpTimeSystem, rp_time_system, RP_TYPE_TIME_SOURCE)
static void
rp_time_system_default_init(RpTimeSystemInterface* iface)
{
    LOGD("(%p)", iface);
}
