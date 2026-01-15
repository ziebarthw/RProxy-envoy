/*
 * rp-net-address.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_net_address_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_net_address_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-net-address.h"

G_DEFINE_INTERFACE(RpNetworkAddressIpv4, rp_network_address_ipv4, G_TYPE_OBJECT)
static void
rp_network_address_ipv4_default_init(RpNetworkAddressIpv4Interface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpNetworkAddressIpv6, rp_network_address_ipv6, G_TYPE_OBJECT)
static void
rp_network_address_ipv6_default_init(RpNetworkAddressIpv6Interface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpNetworkAddressIp, rp_network_address_ip, G_TYPE_OBJECT)
static void
rp_network_address_ip_default_init(RpNetworkAddressIpInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpNetworkAddressPipe, rp_network_address_pipe, G_TYPE_OBJECT)
static void
rp_network_address_pipe_default_init(RpNetworkAddressPipeInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpNetworkAddressRProxyInternalAddress, rp_network_address_rproxy_internal_address, G_TYPE_OBJECT)
static void
rp_network_address_rproxy_internal_address_default_init(RpNetworkAddressRProxyInternalAddressInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpNetworkAddressInstance, rp_network_address_instance, G_TYPE_OBJECT)
static void
rp_network_address_instance_default_init(RpNetworkAddressInstanceInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
