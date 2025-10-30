/*
 * rp-host-description.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_host_decscription_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_host_decscription_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-host-description.h"

G_DEFINE_INTERFACE(RpHostDescription, rp_host_description, G_TYPE_OBJECT)

static void
rp_host_description_default_init(RpHostDescriptionInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
