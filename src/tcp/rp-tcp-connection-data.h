/*
 * rp-tcp-connection-data.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "tcp/rp-active-tcp-client.h"

G_BEGIN_DECLS

/*
 * This acts as the bridge between the ActiveTcpClient and an individual TCP connection.
 */
#define RP_TYPE_TCP_CONNECTION_DATA rp_tcp_connection_data_get_type()
G_DECLARE_FINAL_TYPE(RpTcpConnectionData, rp_tcp_connection_data, RP, TCP_CONNECTION_DATA, GObject)

RpTcpConnectionData* rp_tcp_connection_data_new(RpActiveTcpClient* parent,
                                                RpNetworkClientConnection* connection);
void rp_tcp_connection_data_release(RpTcpConnectionData* self);

G_END_DECLS
