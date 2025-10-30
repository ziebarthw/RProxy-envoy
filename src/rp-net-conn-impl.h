/*
 * rp-net-conn-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-listen-socket.h"
#include "rp-net-transport-socket.h"
#include "rp-net-conn-impl-base.h"

G_BEGIN_DECLS

/**
 * Implementation of Network::Connection, Network::FilterManagerConnection and
 * Envoy::ScopeTrackedObject.
 */
#define RP_TYPE_NETWORK_CONNECTION_IMPL rp_network_connection_impl_get_type()
G_DECLARE_DERIVABLE_TYPE(RpNetworkConnectionImpl, rp_network_connection_impl, RP, NETWORK_CONNECTION_IMPL, RpNetworkConnectionImplBase)

struct _RpNetworkConnectionImplClass {
    RpNetworkConnectionImplBaseClass parent_class;

    void (*on_connected)(RpNetworkConnectionImpl*);
};

static inline void
rp_network_connection_impl_on_connected(RpNetworkConnectionImpl* self)
{
    if (RP_IS_NETWORK_CONNECTION_IMPL(self))
    {
        RP_NETWORK_CONNECTION_IMPL_GET_CLASS(self)->on_connected(self);
    }
}
RpConnectionSocket* rp_network_connection_impl_socket_(RpNetworkConnectionImpl* self);
RpNetworkTransportSocket* rp_network_connection_impl_transport_socket_(RpNetworkConnectionImpl* self);
void rp_network_connection_impl_raise_event(RpNetworkConnectionImpl* self,
                                            RpNetworkConnectionEvent_e event);
void rp_network_connection_impl_on_connected_(RpNetworkConnectionImpl* self);

G_END_DECLS
