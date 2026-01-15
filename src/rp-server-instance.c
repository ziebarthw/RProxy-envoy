/*
 * rp-server-instance.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_server_instance_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_server_instance_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-server-instance.h"

G_DEFINE_INTERFACE(RpServerInstance, rp_server_instance, G_TYPE_OBJECT)
static void
rp_server_instance_default_init(RpServerInstanceInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}