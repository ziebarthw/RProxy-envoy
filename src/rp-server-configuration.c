/*
 * rp-server-configuration.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_server_configuration_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_server_configuration_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-server-configuration.h"

G_DEFINE_INTERFACE(RpServerConfigurationMain, rp_server_configuration_main, G_TYPE_OBJECT)
static void
rp_server_configuration_main_default_init(RpServerConfigurationMainInterface* iface)
{
    LOGD("(%p)", iface);
}
