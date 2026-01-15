/*
 * rp-address-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <sys/un.h>
#include "rp-net-address.h"

G_BEGIN_DECLS

bool rp_network_address_force_v6(void);
RpNetworkAddressInstanceConstSharedPtr rp_network_address_address_from_sock_addr(const struct sockaddr_storage* ss,
                                                            socklen_t len,
                                                            bool v6only/* = true*/);
const RpNetworkAddressSocketInterface* rp_network_address_impl_sock_interface_or_default(const RpNetworkAddressSocketInterface* sock_interface);
void rp_network_address_ipv6_to_ipv4_compatible_address(const struct sockaddr_in6* sin6,
                                                        struct sockaddr_in* sin);


/**
 * Base class for all address types.
 */
#define RP_TYPE_NETWORK_ADDRESS_INSTANCE_BASE rp_network_address_instance_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpNetworkAddressInstanceBase, rp_network_address_instance_base, RP, NETWORK_ADDRESS_INSTANCE_BASE, GObject)

struct _RpNetworkAddressInstanceBaseClass {
    GObjectClass parent_class;

};

UNIQUE_PTR(GString)* rp_network_address_instance_base_friendly_name_(RpNetworkAddressInstanceBase* self);


// Create an address instance. Upon failure, return an error status without throwing.
RpNetworkAddressInstanceConstSharedPtr rp_network_address_instance_factory_create_instance_ptr(const struct sockaddr_in* addr);
RpNetworkAddressInstanceConstSharedPtr rp_network_address_instance_factory_create_instance_ptr_2(const struct sockaddr_in6* addr, bool v6only);
RpNetworkAddressInstanceConstSharedPtr rp_network_address_instance_factory_create_instance_ptr_3(const struct sockaddr_un* addr, socklen_t ss_len);


/**
 * Implementation of an IPv4 address.
 */
#define RP_TYPE_NETWORK_ADDRESS_IPV4_INSTANCE rp_network_address_ipv4_instance_get_type()
G_DECLARE_FINAL_TYPE(RpNetworkAddressIpv4Instance, rp_network_address_ipv4_instance, RP, NETWORK_ADDRESS_IPV4_INSTANCE, RpNetworkAddressInstanceBase)

RpNetworkAddressIpv4Instance* rp_network_address_ipv4_instance_new(const struct sockaddr_in* address,
                                                                    const RpNetworkAddressSocketInterface* sock_interface);
RpNetworkAddressIpv4Instance* rp_network_address_ipv4_instance_new_2(const char* address,
                                                                        const RpNetworkAddressSocketInterface* sock_interface);
RpNetworkAddressIpv4Instance* rp_network_address_ipv4_instance_new_3(const char* address,
                                                                        guint32 port,
                                                                        const RpNetworkAddressSocketInterface* sock_interface);
RpNetworkAddressIpv4Instance* rp_network_address_ipv4_instance_new_4(guint32 port,
                                                                        const RpNetworkAddressSocketInterface* sock_interface);
char* rp_network_address_ipv4_sockaddr_to_string(const struct sockaddr_in* addr);


/**
 * Implementation of an IPv6 address.
 */
#define RP_TYPE_NETWORK_ADDRESS_IPV6_INSTANCE rp_network_address_ipv6_instance_get_type()
G_DECLARE_FINAL_TYPE(RpNetworkAddressIpv6Instance, rp_network_address_ipv6_instance, RP, NETWORK_ADDRESS_IPV6_INSTANCE, RpNetworkAddressInstanceBase)

RpNetworkAddressIpv6Instance* rp_network_address_ipv6_instance_new(const struct sockaddr_in6* address,
                                                                    bool v6only/* = true*/,
                                                                    const RpNetworkAddressSocketInterface* sock_interface);
RpNetworkAddressIpv6Instance* rp_network_address_ipv6_instance_new_2(const char* address,
                                                                        const RpNetworkAddressSocketInterface* sock_interface);
RpNetworkAddressIpv6Instance* rp_network_address_ipv6_instance_new_3(const char* address,
                                                                        guint32 port,
                                                                        const RpNetworkAddressSocketInterface* sock_interface,
                                                                        bool v6only/* = true*/);
RpNetworkAddressIpv6Instance* rp_network_address_ipv6_instance_new_4(guint32 port,
                                                                        const RpNetworkAddressSocketInterface* sock_interface);
char* rp_network_address_ipv6_sockaddr_to_string(const struct sockaddr_in6* addr);


/**
 * Implementation of a pipe address (unix domain socket on unix).
 */
#define RP_TYPE_NETWORK_ADDRESS_PIPE_INSTANCE rp_network_address_pipe_instance_get_type()
G_DECLARE_FINAL_TYPE(RpNetworkAddressPipeInstance, rp_network_address_pipe_instance, RP, NETWORK_ADDRESS_PIPE_INSTANCE, RpNetworkAddressInstanceBase)

UNIQUE_PTR(RpNetworkAddressPipeInstance) rp_network_address_pipe_instance_create(const struct sockaddr_un* address,
                                                                                    socklen_t ss_len,
                                                                                    mode_t mode/* = 0*/,
                                                                                    const RpNetworkAddressSocketInterface* sock_interface);
UNIQUE_PTR(RpNetworkAddressPipeInstance) rp_network_address_pipe_instance_create_2(const char* pipe_path,
                                                                                    mode_t/* = 0*/,
                                                                                    const RpNetworkAddressSocketInterface* sock_interface);


#define RP_TYPE_NETWORK_ADDRESS_RPROXY_INTERNAL_INSTANCE rp_network_address_rproxy_internal_instance_get_type()
G_DECLARE_FINAL_TYPE(RpNetworkAddressRProxyInternalInstance, rp_network_address_rproxy_internal_instance, RP, NETWORK_ADDRESS_RPROXY_INTERNAL_INSTANCE, RpNetworkAddressInstanceBase)

RpNetworkAddressRProxyInternalInstance* rp_network_address_rproxy_internal_instance_new(const char* address_id,
                                                                                        const char* endpoint_id/* = ""*/,
                                                                                        const RpNetworkAddressSocketInterface* sock_interface);


G_END_DECLS
