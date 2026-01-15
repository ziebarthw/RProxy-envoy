/*
 * rp-singleton-manager.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_singleton_manager_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_singleton_manager_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-singleton-manager.h"

G_DEFINE_INTERFACE(RpSingletonManager, rp_singleton_manager, G_TYPE_OBJECT)

static void
rp_singleton_manager_default_init(RpSingletonManagerInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
