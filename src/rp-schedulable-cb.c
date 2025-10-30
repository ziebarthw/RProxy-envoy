/*
 * rp-schedulable-cb.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_schedulable_cb_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_schedulable_cb_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <stdio.h>
#include "rp-schedulable-cb.h"

G_DEFINE_INTERFACE(RpSchedulableCallback, rp_schedulable_callback, G_TYPE_OBJECT)
static void
rp_schedulable_callback_default_init(RpSchedulableCallbackInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpCallbackScheduler, rp_callback_scheduler, G_TYPE_OBJECT)
static void
rp_callback_scheduler_default_init(RpCallbackSchedulerInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
