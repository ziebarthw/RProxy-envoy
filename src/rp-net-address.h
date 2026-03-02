/*
 * rp-net-address.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "rproxy.h"
#include "rp-http-utility.h"

G_BEGIN_DECLS

typedef struct _RpNetworkAddressSocketInterface RpNetworkAddressSocketInterface;
typedef struct _RpNetworkAddressInstance RpNetworkAddressInstance;


/**
 * Interface for an Ipv4 address.
 */
#define RP_TYPE_NETWORK_ADDRESS_IPV4 rp_network_address_ipv4_get_type()
G_DECLARE_INTERFACE(RpNetworkAddressIpv4, rp_network_address_ipv4, RP, NETWORK_ADDRESS_IPV4, GObject)

struct _RpNetworkAddressIpv4Interface {
    GTypeInterface parent_iface;

    guint32 (*address)(RpNetworkAddressIpv4*);
};

static inline guint32
rp_network_address_ipv4_address(RpNetworkAddressIpv4* self)
{
    return RP_IS_NETWORK_ADDRESS_IPV4(self) ? RP_NETWORK_ADDRESS_IPV4_GET_IFACE(self)->address(self) : 0;
}


/**
 * Interface for an Ipv6 address.
 */
#define RP_TYPE_NETWORK_ADDRESS_IPV6 rp_network_address_ipv6_get_type()
G_DECLARE_INTERFACE(RpNetworkAddressIpv6, rp_network_address_ipv6, RP, NETWORK_ADDRESS_IPV6, GObject)

typedef struct _IPv6UInt128 IPv6UInt128;
struct _IPv6UInt128 {
    guint64 high;  // Upper 64 bits (bits 127-64)
    guint64 low;   // Lower 64 bits (bits 63-0)
};
extern const IPv6UInt128 IPv6UInt128_ZERO;

struct _RpNetworkAddressIpv6Interface {
    GTypeInterface parent_iface;

    IPv6UInt128 (*address)(RpNetworkAddressIpv6*);
    guint32 (*scope_id)(RpNetworkAddressIpv6*);
    bool (*v6only)(RpNetworkAddressIpv6*);
    RpNetworkAddressInstance* (*v4_compatible_address)(RpNetworkAddressIpv6*);
    RpNetworkAddressInstance* (*address_without_scope_id)(RpNetworkAddressIpv6*);
};

typedef enum {
    RpIpVersion_v4,
    RpIpVersion_v6
} RpIpVersion_e;

static inline IPv6UInt128
rp_network_address_ipv6_address(RpNetworkAddressIpv6* self)
{
    return RP_IS_NETWORK_ADDRESS_IPV6(self) ?
        RP_NETWORK_ADDRESS_IPV6_GET_IFACE(self)->address(self) : IPv6UInt128_ZERO;
}
static inline guint32
rp_network_address_ipv6_scope_id(RpNetworkAddressIpv6* self)
{
    return RP_IS_NETWORK_ADDRESS_IPV6(self) ? RP_NETWORK_ADDRESS_IPV6_GET_IFACE(self)->scope_id(self) : 0;
}
static inline bool
rp_network_address_ipv6_v6only(RpNetworkAddressIpv6* self)
{
    return RP_IS_NETWORK_ADDRESS_IPV6(self) ? RP_NETWORK_ADDRESS_IPV6_GET_IFACE(self)->v6only(self) : false;
}
static inline RpNetworkAddressInstance*
rp_network_address_ipv6_v4_compatible_address(RpNetworkAddressIpv6* self)
{
    return RP_IS_NETWORK_ADDRESS_IPV6(self) ?
        RP_NETWORK_ADDRESS_IPV6_GET_IFACE(self)->v4_compatible_address(self) : NULL;
}
static inline RpNetworkAddressInstance*
rp_network_address_ipv6_address_without_scope_id(RpNetworkAddressIpv6* self)
{
    return RP_IS_NETWORK_ADDRESS_IPV6(self) ?
        RP_NETWORK_ADDRESS_IPV6_GET_IFACE(self)->address_without_scope_id(self) : NULL;
}


/**
 * Interface for a generic IP address.
 */
#define RP_TYPE_NETWORK_ADDRESS_IP rp_network_address_ip_get_type()
G_DECLARE_INTERFACE(RpNetworkAddressIp, rp_network_address_ip, RP, NETWORK_ADDRESS_IP, GObject)

struct _RpNetworkAddressIpInterface {
    GTypeInterface parent_iface;

    const char* (*address_as_string)(RpNetworkAddressIp*);
    bool (*is_any_address)(RpNetworkAddressIp*);
    bool (*is_unicast_address)(RpNetworkAddressIp*);
    RpNetworkAddressIpv4* (*ipv4)(RpNetworkAddressIp*);
    RpNetworkAddressIpv6* (*ipv6)(RpNetworkAddressIp*);
    guint32 (*port)(RpNetworkAddressIp*);
    RpIpVersion_e (*version)(RpNetworkAddressIp*);
};

static inline const char*
rp_network_address_ip_address_as_string(RpNetworkAddressIp* self)
{
    return RP_IS_NETWORK_ADDRESS_IP(self) ?
        RP_NETWORK_ADDRESS_IP_GET_IFACE(self)->address_as_string(self) : NULL;
}
static inline bool
rp_network_address_ip_is_any_address(RpNetworkAddressIp* self)
{
    return RP_IS_NETWORK_ADDRESS_IP(self) ?
        RP_NETWORK_ADDRESS_IP_GET_IFACE(self)->is_any_address(self) : false;
}
static inline bool
rp_network_address_ip_is_unicast_address(RpNetworkAddressIp* self)
{
    return RP_IS_NETWORK_ADDRESS_IP(self) ?
        RP_NETWORK_ADDRESS_IP_GET_IFACE(self)->is_unicast_address(self) : false;
}
static inline RpNetworkAddressIpv4*
rp_network_address_ip_ipv4(RpNetworkAddressIp* self)
{
    return RP_IS_NETWORK_ADDRESS_IP(self) ?
        RP_NETWORK_ADDRESS_IP_GET_IFACE(self)->ipv4(self) : NULL;
}
static inline RpNetworkAddressIpv6*
rp_network_address_ip_ipv6(RpNetworkAddressIp* self)
{
    return RP_IS_NETWORK_ADDRESS_IP(self) ?
        RP_NETWORK_ADDRESS_IP_GET_IFACE(self)->ipv6(self) : NULL;
}
static inline guint32
rp_network_address_ip_port(RpNetworkAddressIp* self)
{
    return RP_IS_NETWORK_ADDRESS_IP(self) ?
        RP_NETWORK_ADDRESS_IP_GET_IFACE(self)->port(self) : 0;
}
static inline RpIpVersion_e
rp_network_address_ip_version(RpNetworkAddressIp* self)
{
    return RP_IS_NETWORK_ADDRESS_IP(self) ?
        RP_NETWORK_ADDRESS_IP_GET_IFACE(self)->version(self) : RpIpVersion_v4;
}


/**
 * Interface for a generic Pipe address.
 */
#define RP_TYPE_NETWORK_ADDRESS_PIPE rp_network_address_pipe_get_type()
G_DECLARE_INTERFACE(RpNetworkAddressPipe, rp_network_address_pipe, RP, NETWORK_ADDRESS_PIPE, GObject)

struct _RpNetworkAddressPipeInterface {
    GTypeInterface parent_iface;

    bool (*abstract_namespace)(RpNetworkAddressPipe*);
    mode_t (*mode)(RpNetworkAddressPipe*);
};

static inline bool
rp_network_address_pipe_abstract_namespace(RpNetworkAddressPipe* self)
{
    return RP_IS_NETWORK_ADDRESS_PIPE(self) ?
        RP_NETWORK_ADDRESS_PIPE_GET_IFACE(self)->abstract_namespace(self) : false;
}
static inline mode_t
rp_network_address_pipe_mode(RpNetworkAddressPipe* self)
{
    return RP_IS_NETWORK_ADDRESS_PIPE(self) ?
        RP_NETWORK_ADDRESS_PIPE_GET_IFACE(self)->mode(self) : 0;
}


/**
 * Interface for a generic internal address.
 */
#define RP_TYPE_NETWORK_ADDRESS_RPROXY_INTERNAL_ADDRESS rp_network_address_rproxy_internal_address_get_type()
G_DECLARE_INTERFACE(RpNetworkAddressRProxyInternalAddress, rp_network_address_rproxy_internal_address, RP, NETWORK_ADDRESS_RPROXY_INTERNAL_ADDRESS, GObject)

struct _RpNetworkAddressRProxyInternalAddressInterface {
    GTypeInterface parent_iface;

    const char* (*address_id)(RpNetworkAddressRProxyInternalAddress*);
    const char* (*endpoint_id)(RpNetworkAddressRProxyInternalAddress*);
};

static inline const char*
rp_rproxy_internal_address_address_id(RpNetworkAddressRProxyInternalAddress* self)
{
    return RP_IS_NETWORK_ADDRESS_RPROXY_INTERNAL_ADDRESS(self) ?
        RP_NETWORK_ADDRESS_RPROXY_INTERNAL_ADDRESS_GET_IFACE(self)->address_id(self) : NULL;
}
static inline const char*
rp_rproxy_internal_address_endpoint_id(RpNetworkAddressRProxyInternalAddress* self)
{
    return RP_IS_NETWORK_ADDRESS_RPROXY_INTERNAL_ADDRESS(self) ?
        RP_NETWORK_ADDRESS_RPROXY_INTERNAL_ADDRESS_GET_IFACE(self)->endpoint_id(self) : NULL;
}

typedef enum {
    RpNetworkAddressType_IP,
    RpNetworkAddressType_PIPE,
    RpNetworkAddressType_RPROXY_INTERNAL
} RpNetworkAddressType_e;


/**
 * Interface for all network addresses.
 */
#define RP_TYPE_NETWORK_ADDRESS_INSTANCE rp_network_address_instance_get_type()
G_DECLARE_INTERFACE(RpNetworkAddressInstance, rp_network_address_instance, RP, NETWORK_ADDRESS_INSTANCE, GObject)

struct _RpNetworkAddressInstanceInterface {
    GTypeInterface parent_iface;

    const char* (*as_string)(const RpNetworkAddressInstance*);
    string_view (*as_string_view)(const RpNetworkAddressInstance*);
    const char* (*logical_name)(const RpNetworkAddressInstance*);
    RpNetworkAddressIp* (*ip)(const RpNetworkAddressInstance*);
    RpNetworkAddressPipe* (*pipe)(const RpNetworkAddressInstance*);
    RpNetworkAddressRProxyInternalAddress* (*rproxy_internal_address)(const RpNetworkAddressInstance*);
    const struct sockaddr* (*sock_addr)(const RpNetworkAddressInstance*);
    socklen_t (*sock_addr_len)(const RpNetworkAddressInstance*);
    RpNetworkAddressType_e (*type)(const RpNetworkAddressInstance*);
    string_view (*address_type)(const RpNetworkAddressInstance*);
    RpNetworkAddressSocketInterface* (*socket_interface)(const RpNetworkAddressInstance*);
};

typedef const SHARED_PTR(RpNetworkAddressInstance) RpNetworkAddressInstanceConstSharedPtr;
typedef SHARED_PTR(RpNetworkAddressInstance) RpNetworkAddressInstanceSharedPtr;

static inline gboolean
rp_network_address_instance_is_network_address_instance(RpNetworkAddressInstanceConstSharedPtr self)
{
    g_return_val_if_fail(self != NULL, false);
    return RP_IS_NETWORK_ADDRESS_INSTANCE((RpNetworkAddressInstance*)self);
}
static inline void
rp_network_address_instance_set_object(RpNetworkAddressInstanceSharedPtr* dst, RpNetworkAddressInstanceConstSharedPtr src)
{
    g_return_if_fail(dst != NULL);
    g_set_object((GObject**)dst, (GObject*)src);
}
static inline RpNetworkAddressInstanceInterface*
rp_network_address_instance_iface(RpNetworkAddressInstanceConstSharedPtr self)
{
    g_return_val_if_fail(rp_network_address_instance_is_network_address_instance(self), NULL);
    return RP_NETWORK_ADDRESS_INSTANCE_GET_IFACE((GObject*)self);
}
static inline const char*
rp_network_address_instance_as_string(RpNetworkAddressInstanceConstSharedPtr self)
{
    g_return_val_if_fail(rp_network_address_instance_is_network_address_instance(self), NULL);
    RpNetworkAddressInstanceInterface* iface = rp_network_address_instance_iface(self);
    return iface->as_string ? iface->as_string(self) : NULL;
}
static inline string_view
rp_network_address_instance_as_string_view(RpNetworkAddressInstanceConstSharedPtr self)
{
    g_return_val_if_fail(rp_network_address_instance_is_network_address_instance(self), string_view_ctor("", 0));
    RpNetworkAddressInstanceInterface* iface = rp_network_address_instance_iface(self);
    return iface->as_string_view ?
        iface->as_string_view(self) : string_view_ctor("", 0);
}
static inline const char*
rp_network_address_instance_logical_name(RpNetworkAddressInstanceConstSharedPtr self)
{
    g_return_val_if_fail(rp_network_address_instance_is_network_address_instance(self), NULL);
    RpNetworkAddressInstanceInterface* iface = rp_network_address_instance_iface(self);
    return iface->logical_name ? iface->logical_name(self) : NULL;
}
static inline RpNetworkAddressIp*
rp_network_address_instance_ip(RpNetworkAddressInstanceConstSharedPtr self)
{
    g_return_val_if_fail(rp_network_address_instance_is_network_address_instance(self), NULL);
    RpNetworkAddressInstanceInterface* iface = rp_network_address_instance_iface(self);
    return iface->ip ? iface->ip(self) : NULL;
}
static inline RpNetworkAddressPipe*
rp_network_address_instance_pipe(RpNetworkAddressInstanceConstSharedPtr self)
{
    g_return_val_if_fail(rp_network_address_instance_is_network_address_instance(self), NULL);
    RpNetworkAddressInstanceInterface* iface = rp_network_address_instance_iface(self);
    return iface->pipe ? iface->pipe(self) : NULL;
}
static inline RpNetworkAddressRProxyInternalAddress*
rp_network_address_instance_rproxy_internal_address(RpNetworkAddressInstanceConstSharedPtr self)
{
    g_return_val_if_fail(rp_network_address_instance_is_network_address_instance(self), NULL);
    RpNetworkAddressInstanceInterface* iface = rp_network_address_instance_iface(self);
    return iface->rproxy_internal_address ?
        iface->rproxy_internal_address(self) : NULL;
}
static inline const struct sockaddr*
rp_network_address_instance_sock_addr(RpNetworkAddressInstanceConstSharedPtr self)
{
    g_return_val_if_fail(rp_network_address_instance_is_network_address_instance(self), NULL);
    RpNetworkAddressInstanceInterface* iface = rp_network_address_instance_iface(self);
    return iface->sock_addr ? iface->sock_addr(self) : NULL;
}
static inline socklen_t
rp_network_address_instance_sock_addr_len(RpNetworkAddressInstanceConstSharedPtr self)
{
    g_return_val_if_fail(rp_network_address_instance_is_network_address_instance(self), 0);
    RpNetworkAddressInstanceInterface* iface = rp_network_address_instance_iface(self);
    return iface->sock_addr_len ? iface->sock_addr_len(self) : 0;
}
static inline RpNetworkAddressType_e
rp_network_address_instance_type(RpNetworkAddressInstanceConstSharedPtr self)
{
    g_return_val_if_fail(rp_network_address_instance_is_network_address_instance(self), RpNetworkAddressType_IP);
    RpNetworkAddressInstanceInterface* iface = rp_network_address_instance_iface(self);
    return iface->type ? iface->type(self) : RpNetworkAddressType_IP;
}
static inline string_view
rp_network_address_instance_address_type(RpNetworkAddressInstanceConstSharedPtr self)
{
    g_return_val_if_fail(rp_network_address_instance_is_network_address_instance(self), string_view_ctor("", 0));
    RpNetworkAddressInstanceInterface* iface = rp_network_address_instance_iface(self);
    return iface->address_type ? iface->address_type(self) : string_view_ctor("", 0);
}
static inline RpNetworkAddressSocketInterface*
rp_network_address_instance_socket_interface(RpNetworkAddressInstanceConstSharedPtr self)
{
    g_return_val_if_fail(rp_network_address_instance_is_network_address_instance(self), NULL);
    RpNetworkAddressInstanceInterface* iface = rp_network_address_instance_iface(self);
    return iface->socket_interface ? iface->socket_interface(self) : NULL;
}


G_END_DECLS
