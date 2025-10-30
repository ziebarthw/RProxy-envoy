/*
 * rp-socket-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <evhtp.h>
#include "rp-socket.h"
#include "network/rp-conn-info-setter-impl.h"

G_BEGIN_DECLS

#define RP_TYPE_SOCKET_IMPL rp_socket_impl_get_type()
G_DECLARE_DERIVABLE_TYPE(RpSocketImpl, rp_socket_impl, RP, SOCKET_IMPL, GObject)

struct _RpSocketImplClass {
    GObjectClass parent_class;
};

G_END_DECLS
