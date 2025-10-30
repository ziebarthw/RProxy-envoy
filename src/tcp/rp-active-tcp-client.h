/*
 * rp-active-tcp-client.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "tcp/rp-conn-pool.h"

G_BEGIN_DECLS

typedef struct _RpTcpConnectionData RpTcpConnectionData;

void rp_active_tcp_client_clear_callbacks(RpActiveTcpClient* self);
void rp_active_tcp_client_read_enable_if_new(RpActiveTcpClient* self);
void rp_active_tcp_client_on_upstream_data(RpActiveTcpClient* self,
                                            evbuf_t* data,
                                            bool end_stream);
void rp_active_tcp_client_set_tcp_connection_data_(RpActiveTcpClient* self,
                                                    RpTcpConnectionData* data);
RpNetworkClientConnection* rp_active_tcp_client_connection_(RpActiveTcpClient* self);
void rp_active_tcp_client_set_callbacks_(RpActiveTcpClient* self,
                                            RpTcpConnPoolUpstreamCallbacks* callbacks);

G_END_DECLS
