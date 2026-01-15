/*
 * rp-server-configuration.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-cluster-manager.h"

G_BEGIN_DECLS

/**
 * The main server configuration.
 */
#define RP_TYPE_SERVER_CONFIGURATION_MAIN rp_server_configuration_main_get_type()
G_DECLARE_INTERFACE(RpServerConfigurationMain, rp_server_configuration_main, RP, SERVER_CONFIGURATION_MAIN, GObject)

struct _RpServerConfigurationMainInterface {
    GTypeInterface parent_iface;

    RpClusterManager* (*cluster_manager)(RpServerConfigurationMain*);
};

static inline RpClusterManager*
rp_server_configuration_cluster_manager(RpServerConfigurationMain* self)
{
    return RP_IS_SERVER_CONFIGURATION_MAIN(self) ?
        RP_SERVER_CONFIGURATION_MAIN_GET_IFACE(self)->cluster_manager(self) :
        NULL;
}

G_END_DECLS
