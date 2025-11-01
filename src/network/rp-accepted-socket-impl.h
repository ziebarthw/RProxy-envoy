/*
 * rp-accepted-socket-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"
#include "network/rp-connection-socket-impl.h"
#include "rp-io-handle.h"

G_BEGIN_DECLS

/*
 * ConnectionSocket used with server connections.
 */
#define RP_TYPE_ACCEPTED_SOCKET_IMPL rp_accepted_socket_impl_get_type()
G_DECLARE_FINAL_TYPE(RpAcceptedSocketImpl, rp_accepted_socket_impl, RP, ACCEPTED_SOCKET_IMPL, RpConnectionSocketImpl)

RpAcceptedSocketImpl* rp_accepted_socket_impl_new(RpIoHandle* io_handle,
                                                    UNIQUE_PTR(evhtp_connection_t) conn);

G_END_DECLS
