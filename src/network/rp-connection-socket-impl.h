/*
 * rp-connection-socket-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-listen-socket.h"
#include "network/rp-socket-impl.h"

G_BEGIN_DECLS

#define RP_TYPE_CONNECTION_SOCKET_IMPL rp_connection_socket_impl_get_type()
G_DECLARE_DERIVABLE_TYPE(RpConnectionSocketImpl, rp_connection_socket_impl, RP, CONNECTION_SOCKET_IMPL, RpSocketImpl)

struct _RpConnectionSocketImplClass {
    RpSocketImplClass parent_class;

};

G_END_DECLS
