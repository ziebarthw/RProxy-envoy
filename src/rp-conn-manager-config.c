/*
 * rp-conn-manager-config.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#include "rp-conn-manager-config.h"

G_DEFINE_INTERFACE(RpConnectionManagerConfig, rp_connection_manager_config, G_TYPE_OBJECT)

static void
rp_connection_manager_config_default_init(RpConnectionManagerConfigInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}