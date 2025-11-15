/*
 * rp-active-tcp-conn.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-net-connection.h"
#include "rp-stream-info.h"

G_BEGIN_DECLS

#define RP_TYPE_ACTIVE_TCP_CONN rp_active_tcp_conn_get_type()
G_DECLARE_FINAL_TYPE(RpActiveTcpConn, rp_active_tcp_conn, RP, ACTIVE_TCP_CONN, GObject)

RpActiveTcpConn* rp_active_tcp_conn_new(GList** active_connections,
                                        UNIQUE_PTR(RpNetworkConnection) new_connection,
                                        UNIQUE_PTR(RpStreamInfo) stream_info,
                                        SHARED_PTR(thread_ctx_t) thread_ctx);

G_END_DECLS
