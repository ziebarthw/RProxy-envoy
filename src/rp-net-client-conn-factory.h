/*
 * rp-net-client-conn-factory.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-dispatcher.h"
#include "rp-net-connection.h"
#include "rp-listen-socket.h"
#include "rp-net-transport-socket.h"

G_BEGIN_DECLS

/*
 * The factory to create a client connection. This factory hides the details of various remote
 * address type and transport socket.
 */
#define RP_TYPE_CLIENT_CONNECTION_FACTORY rp_client_connection_factory_get_type()
G_DECLARE_INTERFACE(RpClientConnectionFactory, rp_client_connection_factory, RP, CLIENT_CONNECTION_FACTORY, GObject)

struct _RpClientConnectionFactoryInterface {
    GTypeInterface parent_iface;

    RpNetworkClientConnection* (*create_client_connection)(RpClientConnectionFactory*,
                                                            RpDispatcher*,
                                                            struct sockaddr*,
                                                            struct sockaddr*,
                                                            RpNetworkTransportSocket*);
};

static inline RpNetworkClientConnection*
rp_client_connection_factory_create_client_connection(RpClientConnectionFactory* self, RpDispatcher* dispatcher,
                                                        struct sockaddr* address, struct sockaddr* source_address,
                                                        RpNetworkTransportSocket* transport_socket)
{
    return RP_IS_CLIENT_CONNECTION_FACTORY(self) ?
        RP_CLIENT_CONNECTION_FACTORY_GET_IFACE(self)->create_client_connection(self, dispatcher, address, source_address, transport_socket) :
        NULL;
}

G_END_DECLS
