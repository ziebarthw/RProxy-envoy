/*
 * rp-server-configuration-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"
#include "rp-cluster-manager.h"
#include "rp-server-instance.h"
#include "rp-server-configuration.h"

G_BEGIN_DECLS

/**
 * Implementation of Server::Configuration::Main that reads a configuration from
 * a JSON file.
 */
#define RP_TYPE_SERVER_CONFIGURATION_MAIN_IMPL rp_server_configuration_main_impl_get_type()
G_DECLARE_FINAL_TYPE(RpServerConfigurationMainImpl, rp_server_configuration_main_impl, RP, SERVER_CONFIGURATION_MAIN_IMPL, GObject)

RpServerConfigurationMainImpl* rp_server_configuration_main_impl_new(void);
bool rp_server_configuration_main_impl_initialize(RpServerConfigurationMainImpl* self,
                                                    rproxy_t* bootstrap,
                                                    RpServerInstance* server,
                                                    RpClusterManagerFactory* cluster_manager_factory,
                                                    GError** err);

G_END_DECLS
