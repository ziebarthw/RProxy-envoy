/*
 * rp-transport-socket-config.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-cluster-manager.h"
#include "rp-net-transport-socket.h"

G_BEGIN_DECLS

typedef struct _RpServerFactoryContext RpServerFactoryContext;

/**
 * Context passed to transport socket factory to access server resources.
 */
#define RP_TYPE_TRANSPORT_SOCKET_FACTORY_CONTEXT rp_transport_socket_factory_context_get_type()
G_DECLARE_INTERFACE(RpTransportSocketFactoryContext, rp_transport_socket_factory_context, RP, TRANSPORT_SOCKET_FACTORY_CONTEXT, GObject)

struct _RpTransportSocketFactoryContextInterface {
    GTypeInterface parent_iface;

    RpServerFactoryContext* (*server_factory_context)(RpTransportSocketFactoryContext*);
};

static inline RpServerFactoryContext*
rp_transport_socket_factory_context_server_factory_context(RpTransportSocketFactoryContext* self)
{
    return RP_IS_TRANSPORT_SOCKET_FACTORY_CONTEXT(self) ?
        RP_TRANSPORT_SOCKET_FACTORY_CONTEXT_GET_IFACE(self)->server_factory_context(self) :
        NULL;
}

G_END_DECLS
