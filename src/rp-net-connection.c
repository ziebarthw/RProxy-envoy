/*
 * rp-connection.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#include "rp-net-connection.h"

G_DEFINE_INTERFACE(RpNetworkConnectionCallbacks, rp_network_connection_callbacks, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpNetworkConnection, rp_network_connection, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpNetworkServerConnection, rp_network_server_connection, RP_TYPE_NETWORK_CONNECTION)
G_DEFINE_INTERFACE(RpNetworkClientConnection, rp_network_client_connection, RP_TYPE_NETWORK_CONNECTION)

static void
rp_network_connection_callbacks_default_init(RpNetworkConnectionCallbacksInterface* iface) {}
static void
rp_network_connection_default_init(RpNetworkConnectionInterface* iface) {}
static void
rp_network_server_connection_default_init(RpNetworkServerConnectionInterface* iface) {}
static void
rp_network_client_connection_default_init(RpNetworkClientConnectionInterface* iface) {}
