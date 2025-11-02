/*
 * rp-transport-socket-factory-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-net-transport-socket.h"
#include "rproxy.h"

G_BEGIN_DECLS

#define RP_TYPE_TRANSPORT_SOCKET_FACTORY_IMPL rp_transport_socket_factory_impl_get_type()
G_DECLARE_FINAL_TYPE(RpTransportSocketFactoryImpl, rp_transport_socket_factory_impl, RP, TRANSPORT_SOCKET_FACTORY_IMPL, GObject)

RpTransportSocketFactoryImpl* rp_transport_socket_factory_impl_new(SHARED_PTR(upstream_t) downstream);

G_END_DECLS
