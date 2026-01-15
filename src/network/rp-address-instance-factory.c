/*
 * rp-address-instance-factory.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "macrologger.h"

#if (defined(rp_address_instance_factory_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_address_instance_factory_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "network/rp-address-impl.h"

RpNetworkAddressInstanceConstSharedPtr
rp_network_address_instance_factory_create_instance_ptr(const struct sockaddr_in* addr)
{
    LOGD("(%p)", addr);
    return RP_NETWORK_ADDRESS_INSTANCE(rp_network_address_ipv4_instance_new(addr, NULL));
}
RpNetworkAddressInstanceConstSharedPtr
rp_network_address_instance_factory_create_instance_ptr_2(const struct sockaddr_in6* addr, bool v6only)
{
    LOGD("(%p, %u)", addr, v6only);
    return RP_NETWORK_ADDRESS_INSTANCE(rp_network_address_ipv6_instance_new(addr, v6only, NULL));
}
RpNetworkAddressInstanceConstSharedPtr
rp_network_address_instance_factory_create_instance_ptr_3(const struct sockaddr_un* addr, socklen_t ss_len)
{
    LOGD("(%p, %d)", addr, ss_len);
    return NULL; // Not yet implemented.
}
