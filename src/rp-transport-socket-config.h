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
    RpClusterManager* (*cluster_manager)(RpTransportSocketFactoryContext*);
    //TODO...
};

static inline RpServerFactoryContext*
rp_transport_socket_factory_context_server_factory_context(RpTransportSocketFactoryContext* self)
{
    return RP_IS_TRANSPORT_SOCKET_FACTORY_CONTEXT(self) ?
        RP_TRANSPORT_SOCKET_FACTORY_CONTEXT_GET_IFACE(self)->server_factory_context(self) :
        NULL;
}
static inline RpClusterManager*
rp_transport_socket_factory_context_cluster_manager(RpTransportSocketFactoryContext* self)
{
    return RP_IS_TRANSPORT_SOCKET_FACTORY_CONTEXT(self) ?
        RP_TRANSPORT_SOCKET_FACTORY_CONTEXT_GET_IFACE(self)->cluster_manager(self) :
        NULL;
}

typedef UNIQUE_PTR(RpTransportSocketFactoryContext) RpTransportSocketFactoryContextPtr;


/**
 * Implemented by each transport socket used for upstream connections. Registered via class
 * RegisterFactory.
 */
#define RP_TYPE_UPSTREAM_TRANSPORT_SOCKET_CONFIG_FACTORY rp_upstream_transport_socket_config_factory_get_type()
G_DECLARE_INTERFACE(RpUpstreamTransportSocketConfigFactory, rp_upstream_transport_socket_config_factory, RP, UPSTREAM_TRANSPORT_SOCKET_CONFIG_FACTORY, GObject)

struct _RpUpstreamTransportSocketConfigFactoryInterface {
    GTypeInterface parent_iface;

    RpUpstreamTransportSocketFactoryPtr (*create_transport_socket_factory)(RpUpstreamTransportSocketConfigFactory*, gpointer, RpTransportSocketFactoryContext*);
};

static inline RpUpstreamTransportSocketFactoryPtr
rp_upstream_transport_socket_config_factory_create_transport_socket_factory(RpUpstreamTransportSocketConfigFactory* self,
                                                                gpointer config, RpTransportSocketFactoryContext* context)
{
    return RP_IS_UPSTREAM_TRANSPORT_SOCKET_CONFIG_FACTORY(self) ?
        RP_UPSTREAM_TRANSPORT_SOCKET_CONFIG_FACTORY_GET_IFACE(self)->create_transport_socket_factory(self, config, context) :
        NULL;
}


/**
 * Implemented by each transport socket used for downstream connections. Registered via class
 * RegisterFactory.
 */
#define RP_TYPE_DOWNSTREAM_TRANSPORT_SOCKET_CONFIG_FACTORY rp_downstream_transport_socket_config_factory_get_type()
G_DECLARE_INTERFACE(RpDownstreamTransportSocketConfigFactory, rp_downstream_transport_socket_config_factory, RP, DOWNSTREAM_TRANSPORT_SOCKET_CONFIG_FACTORY, GObject)

struct _RpDownstreamTransportSocketConfigFactoryInterface {
    GTypeInterface parent_iface;

    RpDownstreamTransportSocketFactoryPtr (*create_transport_socket_factory)(RpDownstreamTransportSocketConfigFactory*, gpointer, RpTransportSocketFactoryContext*/*,TODO...const std::vector<std::string>&*/);
};

static inline RpDownstreamTransportSocketFactoryPtr
rp_downstream_transport_socket_config_factory_create_transport_socket_factory(RpDownstreamTransportSocketConfigFactory* self,
                                                                                gpointer config, RpTransportSocketFactoryContext* context/*,TODO...const std::vector<std::string>& server_names*/)
{
    return RP_IS_DOWNSTREAM_TRANSPORT_SOCKET_CONFIG_FACTORY(self) ?
        RP_DOWNSTREAM_TRANSPORT_SOCKET_CONFIG_FACTORY_GET_IFACE(self)->create_transport_socket_factory(self, config, context) :
        NULL;
}

G_END_DECLS
