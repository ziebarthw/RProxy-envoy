/*
 * rp-io-bev-socket-handle-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"
#include "rp-io-handle.h"

G_BEGIN_DECLS

typedef enum {
    RpHandleType_Accepting,
    RpHandleType_Connecting
} RpHandleType_e;

/**
 * IoHandle derivative for bufferevent sockets.
 */
#define RP_TYPE_IO_BEV_SOCKET_HANDLE_IMPL rp_io_bev_socket_handle_impl_get_type()
G_DECLARE_FINAL_TYPE(RpIoBevSocketHandleImpl, rp_io_bev_socket_handle_impl, RP, IO_BEV_SOCKET_HANDLE_IMPL, GObject)

RpIoBevSocketHandleImpl* rp_io_bev_socket_handle_impl_new(RpHandleType_e type,
                                                            evbev_t* bev,
                                                            evdns_base_t* dns_base);

G_END_DECLS
