/*
 * rp-socket-interface.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_socket_interface_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_socket_interface_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "network/rp-socket-interface.h"

G_DEFINE_ABSTRACT_TYPE(RpNetworkAddressSocketInterfaceBase, rp_network_address_socket_interface_base, G_TYPE_OBJECT)

static void
rp_network_address_socket_interface_base_class_init(RpNetworkAddressSocketInterfaceBaseClass* klass G_GNUC_UNUSED)
{
    LOGD("(%p)", klass);
}

static void
rp_network_address_socket_interface_base_init(RpNetworkAddressSocketInterfaceBase* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}
