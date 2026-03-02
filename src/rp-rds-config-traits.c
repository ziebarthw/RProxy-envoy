/*
 * rp-rds-config-traits.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_rds_config_traits_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_rds_config_traits_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-rds-config-traits.h"

G_DEFINE_INTERFACE(RpRdsConfigTraits, rp_rds_config_traits, G_TYPE_OBJECT)
static void
rp_rds_config_traits_default_init(RpRdsConfigTraitsInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
