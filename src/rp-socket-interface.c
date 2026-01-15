/*
 * rp-socket-interface.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_network_address_socket_interface_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_network_address_socket_interface_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-socket-interface.h"

G_DEFINE_INTERFACE(RpNetworkAddressSocketInterface, rp_network_address_socket_interface, G_TYPE_OBJECT)
static void
rp_network_address_socket_interface_default_init(RpNetworkAddressSocketInterfaceInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
