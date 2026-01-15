/*
 * rp-transport-socket-config-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"
#include "rp-factory-context.h"
#include "rp-transport-socket-config.h"

G_BEGIN_DECLS

/**
 * Implementation of TransportSocketFactoryContext.
 */
#define RP_TYPE_TRANSPORT_SOCKET_FACTORY_CONTEXT_IMPL rp_transport_socket_factory_context_impl_get_type()
G_DECLARE_FINAL_TYPE(RpTransportSocketFactoryContextImpl, rp_transport_socket_factory_context_impl, RP, TRANSPORT_SOCKET_FACTORY_CONTEXT_IMPL, GObject)

RpTransportSocketFactoryContextImpl* rp_transport_socket_factory_context_impl_new(RpServerFactoryContext* server_context,
                                                                                    RpClusterManager* cm);

typedef UNIQUE_PTR(RpTransportSocketFactoryContextImpl) RpTransportSocketFactoryContextImplPtr;

G_END_DECLS
