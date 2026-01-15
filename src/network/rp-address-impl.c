/*
 * rp-address-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_address_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_address_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "network/rp-socket-interface-impl.h"
#include "network/rp-address-impl.h"

bool
rp_network_address_force_v6(void)
{
    LOGD("()");
    return false;
}

void
rp_network_address_ipv6_to_ipv4_compatible_address(const struct sockaddr_in6* sin6, struct sockaddr_in* sin)
{
    LOGD("(%p, %p)", sin6, sin);
    *sin = (struct sockaddr_in){AF_INET, sin6->sin6_port, {sin6->sin6_addr.s6_addr32[3]}, {}};
}

RpNetworkAddressInstanceConstSharedPtr
rp_network_address_address_from_sock_addr(const struct sockaddr_storage* ss, socklen_t ss_len, bool v6only/* = true*/)
{
    LOGD("(%p, %d, %u)", ss, ss_len, v6only);
    if (rp_network_address_force_v6())
    {
        NOISY_MSG_("setting v6only to false");
        v6only = false;
    }
    switch (ss->ss_family)
    {
        case AF_INET:
            const struct sockaddr_in* sin = (const struct sockaddr_in*)ss;
            return rp_network_address_instance_factory_create_instance_ptr(sin);
        case AF_INET6:
            const struct sockaddr_in6* sin6 = (const struct sockaddr_in6*)ss;
            if (!v6only && IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr))
            {
                struct sockaddr_in sin;
                rp_network_address_ipv6_to_ipv4_compatible_address(sin6, &sin);
                return rp_network_address_instance_factory_create_instance_ptr(&sin);
            }
            return rp_network_address_instance_factory_create_instance_ptr_2(sin6, v6only);
        case AF_UNIX:
            const struct sockaddr_un* sun = (const struct sockaddr_un*)ss;
            return rp_network_address_instance_factory_create_instance_ptr_3(sun, ss_len);
        default:
            return NULL;
    }
}

const RpNetworkAddressSocketInterface*
rp_network_address_impl_sock_interface_or_default(const RpNetworkAddressSocketInterface* sock_interface)
{
    extern RpNetworkAddressSocketInterfaceImplFactory* default_network_address_socket_interface_impl_factory;
    LOGD("(%p)", sock_interface);
    return sock_interface == NULL ?
        rp_network_address_socket_interface_impl_factory_get(default_network_address_socket_interface_impl_factory) :
        sock_interface;
}
