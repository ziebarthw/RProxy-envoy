/*
 * rp-net-transport-socket.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_net_transport_socket_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_net_transport_socket_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-net-transport-socket.h"

G_DEFINE_INTERFACE(RpNetworkTransportSocketCallbacks, rp_network_transport_socket_callbacks, G_TYPE_OBJECT)
static void
rp_network_transport_socket_callbacks_default_init(RpNetworkTransportSocketCallbacksInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}


G_DEFINE_INTERFACE(RpNetworkTransportSocket, rp_network_transport_socket, G_TYPE_OBJECT)
static void
rp_network_transport_socket_default_init(RpNetworkTransportSocketInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpTransportSocketFactoryBase, rp_transport_socket_factory_base, G_TYPE_OBJECT)
static void
rp_transport_socket_factory_base_default_init(RpTransportSocketFactoryBaseInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpUpstreamTransportSocketFactory, rp_upstream_transport_socket_factory, RP_TYPE_TRANSPORT_SOCKET_FACTORY_BASE)
static void
rp_upstream_transport_socket_factory_default_init(RpUpstreamTransportSocketFactoryInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpDownstreamTransportSocketFactory, rp_downstream_transport_socket_factory, RP_TYPE_TRANSPORT_SOCKET_FACTORY_BASE)
static void
rp_downstream_transport_socket_factory_default_init(RpDownstreamTransportSocketFactoryInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
