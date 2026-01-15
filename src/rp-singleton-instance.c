/*
 * rp-singleton-instance.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_singleton_instance_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_singleton_instance_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-singleton-instance.h"

G_DEFINE_INTERFACE(RpSingletonInstance, rp_singleton_instance, G_TYPE_OBJECT)

static void
rp_singleton_instance_default_init(RpSingletonInstanceInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
