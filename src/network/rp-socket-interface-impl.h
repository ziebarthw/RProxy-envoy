/*
 * rp-socket-interface-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-singleton-manager.h"
#include "network/rp-socket-interface.h"

G_BEGIN_DECLS

#define RP_TYPE_NETWORK_ADDRESS_SOCKET_INTERFACE_IMPL rp_network_address_socket_interface_impl_get_type()
G_DECLARE_FINAL_TYPE(RpNetworkAddressSocketInterfaceImpl, rp_network_address_socket_interface_impl, RP, NETWORK_ADDRESS_SOCKET_INTERFACE_IMPL, RpNetworkAddressSocketInterfaceBase)

RpNetworkAddressSocketInterfaceImpl* rp_network_address_socket_interface_impl_new(void);


#define RP_TYPE_NETWORK_ADDRESS_SOCKET_INTERFACE_IMPL_FACTORY rp_network_address_socket_interface_impl_factory_get_type()
G_DECLARE_FINAL_TYPE(RpNetworkAddressSocketInterfaceImplFactory, rp_network_address_socket_interface_impl_factory, RP, NETWORK_ADDRESS_SOCKET_INTERFACE_IMPL_FACTORY, GObject)

RpNetworkAddressSocketInterfaceImplFactory* rp_network_address_socket_interface_impl_factory_new(RpSingletonManager* singleton_manager);
RpNetworkAddressSocketInterface* rp_network_address_socket_interface_impl_factory_get(RpNetworkAddressSocketInterfaceImplFactory* self);

G_END_DECLS
