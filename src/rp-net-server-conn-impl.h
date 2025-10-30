/*
 * rp-net-server-conn-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-dispatcher.h"
#include "rp-listen-socket.h"
#include "rp-net-conn-impl.h"
#include "rp-net-transport-socket.h"
#include "rp-stream-info.h"

G_BEGIN_DECLS

#define RP_TYPE_NETWORK_SERVER_CONNECTION_IMPL rp_network_server_connection_impl_get_type()
G_DECLARE_FINAL_TYPE(RpNetworkServerConnectionImpl, rp_network_server_connection_impl, RP, NETWORK_SERVER_CONNECTION_IMPL, RpNetworkConnectionImpl)

RpNetworkServerConnectionImpl* rp_network_server_connection_impl_new(RpDispatcher* dispatcher,
                                                                        RpConnectionSocket* socket,
                                                                        RpNetworkTransportSocket* transport_socket,
                                                                        RpStreamInfo* stream_info);

G_END_DECLS
