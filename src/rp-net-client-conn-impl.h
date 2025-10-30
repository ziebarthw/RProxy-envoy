/*
 * rp-net-client-conn-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-dispatcher.h"
#include "rp-net-transport-socket.h"
#include "rp-net-conn-impl.h"

G_BEGIN_DECLS

/**
 * libevent implementation of Network::ClientConnection.
 */
#define RP_TYPE_NETWORK_CLIENT_CONNECTION_IMPL rp_network_client_connection_impl_get_type()
G_DECLARE_FINAL_TYPE(RpNetworkClientConnectionImpl, rp_network_client_connection_impl, RP, NETWORK_CLIENT_CONNECTION_IMPL, RpNetworkConnectionImpl)

RpNetworkClientConnectionImpl* rp_network_client_connection_impl_new(RpDispatcher* dispatcher,
                                                                        struct sockaddr* address,
                                                                        struct sockaddr* source_address,
                                                                        RpNetworkTransportSocket* transport_socket);

G_END_DECLS
