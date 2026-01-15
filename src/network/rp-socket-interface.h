/*
 * rp-socket-interface.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "../rp-socket-interface.h"

G_BEGIN_DECLS

/*
 * Class to be derived by all SocketInterface implementations.
 *
 * It acts both as a SocketInterface and as a BootstrapExtensionFactory. The latter is used, on the
 * one hand, to configure and initialize the interface and, on the other, for SocketInterface lookup
 * by leveraging the FactoryRegistry. As required for all bootstrap extensions, all derived classes
 * should register via the REGISTER_FACTORY() macro as BootstrapExtensionFactory.
 *
 * SocketInterface instances can be retrieved using the factory name, i.e., string returned by
 * name() function implemented by all classes that derive SocketInterfaceBase, via
 * Network::socketInterface(). When instantiating addresses, address resolvers should
 * set the socket interface field to the name of the socket interface implementation that should
 * be used to create sockets for said addresses.
*/
#define RP_TYPE_NETWORK_ADDRESS_SOCKET_INTERFACE_BASE rp_network_address_socket_interface_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpNetworkAddressSocketInterfaceBase, rp_network_address_socket_interface_base, RP, NETWORK_ADDRESS_INTERFACE_BASE, GObject)

struct _RpNetworkAddressSocketInterfaceBaseClass {
    GObjectClass parent_class;

};

G_END_DECLS
