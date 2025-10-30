/*
 * rp-instance.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_instance_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_instance_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-instance.h"

G_DEFINE_INTERFACE(RpInstance, rp_instance, G_TYPE_OBJECT)
static void
rp_instance_default_init(RpInstanceInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}