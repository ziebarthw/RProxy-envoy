/*
 * rp-socket-interface.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-socket.h"

G_BEGIN_DECLS

#define RP_TYPE_NETWORK_ADDRESS_SOCKET_INTERFACE rp_network_address_socket_interface_get_type()
G_DECLARE_INTERFACE(RpNetworkAddressSocketInterface, rp_network_address_socket_interface, RP, NETWORK_ADDRESS_SOCKET_INTERFACE, GObject)

struct _RpNetworkAddressSocketInterfaceInterface {
    GTypeInterface parent_iface;

    RpIoHandlePtr (*socket)(RpNetworkAddressSocketInterface*, RpSocketType_e, RpAddressType_e, RpIpVersion_e, bool);
    RpIoHandlePtr (*socket_2)(RpNetworkAddressSocketInterface*, RpSocketType_e, RpNetworkAddressInstanceConstSharedPtr);
    bool (*ip_family_supported)(RpNetworkAddressSocketInterface*, int);
};

static inline RpIoHandlePtr
rp_network_address_socket_interface_socket(RpNetworkAddressSocketInterface* self, RpSocketType_e socket_type, RpAddressType_e addr_type, RpIpVersion_e version, bool ipv6only)
{
    return RP_IS_NETWORK_ADDRESS_SOCKET_INTERFACE(self) ?
        RP_NETWORK_ADDRESS_SOCKET_INTERFACE_GET_IFACE(self)->socket(self, socket_type, addr_type, version, ipv6only) :
        NULL;
}
static inline RpIoHandlePtr
rp_network_address_socket_interface_socket_2(RpNetworkAddressSocketInterface* self, RpSocketType_e socket_type, RpNetworkAddressInstanceConstSharedPtr addr)
{
    return RP_IS_NETWORK_ADDRESS_SOCKET_INTERFACE(self) ?
        RP_NETWORK_ADDRESS_SOCKET_INTERFACE_GET_IFACE(self)->socket_2(self, socket_type, addr) :
        NULL;
}
static inline bool
rp_network_address_socket_interface_ip_family_supported(RpNetworkAddressSocketInterface* self, int domain)
{
    return RP_IS_NETWORK_ADDRESS_SOCKET_INTERFACE(self) ?
        RP_NETWORK_ADDRESS_SOCKET_INTERFACE_GET_IFACE(self)->ip_family_supported(self, domain) :
        false;
}

G_END_DECLS
