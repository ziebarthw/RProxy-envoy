/*
 * rp-rds-config.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_rds_config_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_rds_config_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-rds-config.h"

G_DEFINE_INTERFACE(RpRdsConfig, rp_rds_config, G_TYPE_OBJECT)
static void
rp_rds_config_default_init(RpRdsConfigInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
