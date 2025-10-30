/*
 * rp-client-socket-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-io-handle.h"
#include "network/rp-connection-socket-impl.h"

G_BEGIN_DECLS

/*
 * ConnectionSocket used with client connections.
 */
#define RP_TYPE_CLIENT_SOCKET_IMPL rp_client_socket_impl_get_type()
G_DECLARE_FINAL_TYPE(RpClientSocketImpl, rp_client_socket_impl, RP, CLIENT_SOCKET_IMPL, RpConnectionSocketImpl)

RpClientSocketImpl* rp_client_socket_impl_new(RpIoHandle* io_handle,
                                                struct sockaddr* remote_address);

G_END_DECLS
