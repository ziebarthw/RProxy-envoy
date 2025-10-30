/*
 * rp-raw-buffer-socket.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "network/rp-io-bev-socket-handle-impl.h"
#include "rp-net-transport-socket.h"

G_BEGIN_DECLS

#define RP_TYPE_RAW_BUFFER_SOCKET rp_raw_buffer_socket_get_type()
G_DECLARE_FINAL_TYPE(RpRawBufferSocket, rp_raw_buffer_socket, RP, RAW_BUFFER_SOCKET, GObject)

RpRawBufferSocket* rp_raw_buffer_socket_new(RpHandleType_e type,
                                            evbev_t* bev,
                                            evhtp_ssl_t* ssl,
                                            evdns_base_t* dns_base);

G_END_DECLS
