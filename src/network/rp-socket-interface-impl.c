/*
 * rp-socket-interface-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "macrologger.h"

#if (defined(rp_socket_interface_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_socket_interface_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "network/rp-address-impl.h"
#include "network/rp-socket-interface-impl.h"

struct _RpNetworkAddressSocketInterfaceImpl {
    RpNetworkAddressSocketInterfaceBase parent_instance;

};

static void network_address_socket_interface_iface_init(RpNetworkAddressSocketInterfaceInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpNetworkAddressSocketInterfaceImpl, rp_network_address_socket_interface_impl, RP_TYPE_NETWORK_ADDRESS_SOCKET_INTERFACE_BASE,
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_ADDRESS_SOCKET_INTERFACE, network_address_socket_interface_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_SINGLETON_INSTANCE, NULL)
)

static RpIoHandlePtr
make_platform_specific_socket(RpNetworkAddressSocketInterfaceImpl* self, int socket_fd, bool socket_v6only, int domain)
{
    NOISY_MSG_("(%p, %d, %u, %d)", self, socket_fd, socket_v6only, domain);
return NULL;
}

static RpIoHandlePtr
make_socket(RpNetworkAddressSocketInterfaceImpl* self, int socket_fd, bool socket_v6only, int domain)
{
    NOISY_MSG_("(%p, %d, %u, %d)", self, socket_fd, socket_v6only, domain);
    return make_platform_specific_socket(self, socket_fd, socket_v6only, domain);
}

static bool
ip_family_supported_i(RpNetworkAddressSocketInterface* self G_GNUC_UNUSED, int domain)
{
    NOISY_MSG_("(%p)", self);
    int result = socket(domain, SOCK_STREAM, 0);
    if (result != -1)
    {
        close(result);
        return true;
    }
    return false;
}

static RpIoHandlePtr
socket_i(RpNetworkAddressSocketInterface* self, RpSocketType_e socket_type, RpAddressType_e addr_type, RpIpVersion_e version, bool socket_v6only)
{
    NOISY_MSG_("(%p, %d, %d, %d, %u)", self, socket_type, addr_type, version, socket_v6only);
    int protocol = 0;
    int flags = SOCK_NONBLOCK;

    if (socket_type == RpSocketType_Stream)
    {
        flags |= SOCK_STREAM;
    }
    else
    {
        flags |= SOCK_DGRAM;
    }

    int domain;
    if (addr_type == RpAddressType_IP)
    {
        if (version == RpIpVersion_v6 || rp_network_address_force_v6())
        {
            domain = AF_INET6;
        }
        else
        {
            domain = AF_INET;
        }
    }
    else if (addr_type == RpAddressType_PIPE)
    {
        domain = AF_UNIX;
    }
    else
    {
        LOGD("not implemented");
        return NULL;
    }

    int result = socket(domain, flags, protocol);
    if (result == -1)
    {
        int err = errno;
        LOGE("error %d(%s)", err, g_strerror(err));
        return NULL;
    }

    RpIoHandlePtr io_handle = make_socket(RP_NETWORK_ADDRESS_SOCKET_INTERFACE_IMPL(self), result, socket_v6only, domain);
    //TODO...?
    return io_handle;
}

static RpIoHandlePtr
socket_2_i(RpNetworkAddressSocketInterface* self, RpSocketType_e socket_type, RpNetworkAddressInstanceConstSharedPtr addr)
{
    NOISY_MSG_("(%p, %d, %p)", self, socket_type, addr);
    RpIpVersion_e ip_version = rp_network_address_instance_ip(addr) ? rp_network_address_instance_type(addr) : RpIpVersion_v4;
    int v6only = 0;
    if (rp_network_address_instance_type(addr) == RpAddressType_IP && ip_version == RpIpVersion_v6)
    {
        v6only = rp_network_address_ipv6_v6only(
                    rp_network_address_ip_ipv6((RpNetworkAddressIp*)
                        rp_network_address_instance_ip((RpNetworkAddressInstance*)addr)));
    }
    RpIoHandlePtr io_handle = socket_i(self, socket_type, rp_network_address_instance_type(addr), ip_version, v6only);
    if (io_handle && rp_network_address_instance_type(addr) == RpAddressType_IP && ip_version == RpIpVersion_v6 && !rp_network_address_force_v6())
    {
        int result = setsockopt(rp_io_handle_sockfd(io_handle), IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only));
        if (result == -1)
        {
            int err = errno;
            LOGE("error %d(%s)", err, g_strerror(err));
        }
    }
    return io_handle;
}

static void
network_address_socket_interface_iface_init(RpNetworkAddressSocketInterfaceInterface* iface)
{
    LOGD("(%p)", iface);
    iface->ip_family_supported = ip_family_supported_i;
    iface->socket = socket_i;
    iface->socket_2 = socket_2_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_network_address_socket_interface_impl_parent_class)->dispose(obj);
}

static void
rp_network_address_socket_interface_impl_class_init(RpNetworkAddressSocketInterfaceImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_network_address_socket_interface_impl_init(RpNetworkAddressSocketInterfaceImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpNetworkAddressSocketInterfaceImpl*
rp_network_address_socket_interface_impl_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_NETWORK_ADDRESS_SOCKET_INTERFACE_IMPL, NULL);
}
