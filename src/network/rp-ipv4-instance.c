/*
 * rp-ipv4-instance.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <arpa/inet.h>
#include "macrologger.h"

#if (defined(rp_network_address_ipv4_instance_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_network_address_ipv4_instance_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "network/rp-address-impl.h"

#define PARENT_ADDRESS_INSTANCE_IFACE(s) \
    ((RpNetworkAddressInstanceInterface*)g_type_interface_peek_parent(RP_NETWORK_ADDRESS_INSTANCE_GET_IFACE(s)))

typedef struct _RpNetworkAddressIpv4Helper RpNetworkAddressIpv4Helper;
struct _RpNetworkAddressIpv4Helper {
    struct sockaddr_in m_address;
};
static inline guint32
rp_ipv4_helper_address(RpNetworkAddressIpv4Helper* self)
{
    return self->m_address.sin_addr.s_addr;
}

typedef struct _RpNetworkAddressIpHelper RpNetworkAddressIpHelper;
struct _RpNetworkAddressIpHelper {
    RpNetworkAddressIpv4Helper m_ipv4;
    char* m_friendly_address;
};

struct _RpNetworkAddressIpv4Instance {
    RpNetworkAddressInstanceBase parent_instance;

    RpNetworkAddressIpHelper m_ip;
};

static void network_address_instance_iface_init(RpNetworkAddressInstanceInterface* iface);
static void network_address_ip_iface_init(RpNetworkAddressIpInterface* iface);
static void network_address_ipv4_iface_init(RpNetworkAddressIpv4Interface* iface);

G_DEFINE_TYPE_WITH_CODE(RpNetworkAddressIpv4Instance, rp_network_address_ipv4_instance, RP_TYPE_NETWORK_ADDRESS_INSTANCE_BASE,
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_ADDRESS_INSTANCE, network_address_instance_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_ADDRESS_IP, network_address_ip_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_ADDRESS_IPV4, network_address_ipv4_iface_init)
)

static void
init_helper(RpNetworkAddressIpv4Instance* self, const struct sockaddr_in* address)
{
    NOISY_MSG_("(%p)", self);
    memset(&self->m_ip.m_ipv4.m_address, 0, sizeof(self->m_ip.m_ipv4.m_address));
    self->m_ip.m_ipv4.m_address = *address;
    self->m_ip.m_friendly_address = rp_network_address_ipv4_sockaddr_to_string(address);
    char buf[G_ASCII_DTOSTR_BUF_SIZE];
    gchar* port = g_ascii_dtostr(buf, sizeof(buf), ntohs(address->sin_port));
    gsize port_len = strlen(port);
    GString* friendly_name = g_string_new_len(self->m_ip.m_friendly_address, strlen(self->m_ip.m_friendly_address) + 1 + port_len);
    g_string_append_c(friendly_name, ':');
    g_string_append_len(friendly_name, port, port_len);
    *(rp_network_address_instance_base_friendly_name_(RP_NETWORK_ADDRESS_INSTANCE_BASE(self))) = friendly_name;
}

static guint32
address_i(RpNetworkAddressIpv4* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_NETWORK_ADDRESS_IPV4_INSTANCE(self)->m_ip.m_ipv4.m_address.sin_addr.s_addr;
}

static void
network_address_ipv4_iface_init(RpNetworkAddressIpv4Interface* iface)
{
    LOGD("(%p)", iface);
    iface->address = address_i;
}

static const char*
address_as_string_i(RpNetworkAddressIp* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_NETWORK_ADDRESS_IPV4_INSTANCE(self)->m_ip.m_friendly_address;
}

static bool
is_any_address_i(RpNetworkAddressIp* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_NETWORK_ADDRESS_IPV4_INSTANCE(self)->m_ip.m_ipv4.m_address.sin_addr.s_addr == INADDR_ANY;
}

static bool
is_unicast_address_i(RpNetworkAddressIp* self)
{
    NOISY_MSG_("(%p)", self);
    RpNetworkAddressIpv4Instance* me = RP_NETWORK_ADDRESS_IPV4_INSTANCE(self);
    return !rp_network_address_ip_is_any_address(self) &&
            (me->m_ip.m_ipv4.m_address.sin_addr.s_addr != INADDR_BROADCAST) &&
            !((me->m_ip.m_ipv4.m_address.sin_addr.s_addr & htonl(0xf0000000)) == htonl(0xf0000000));
}

static RpNetworkAddressIpv4*
ipv4_i(RpNetworkAddressIp* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_NETWORK_ADDRESS_IPV4(self);
}

static RpNetworkAddressIpv6*
ipv6_i(RpNetworkAddressIp* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return NULL;
}

static guint32
port_i(RpNetworkAddressIp* self)
{
    NOISY_MSG_("(%p)", self);
    return ntohs(RP_NETWORK_ADDRESS_IPV4_INSTANCE(self)->m_ip.m_ipv4.m_address.sin_port);
}

static RpIpVersion_e
version_i(RpNetworkAddressIp* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return RpIpVersion_v4;
}

static void
network_address_ip_iface_init(RpNetworkAddressIpInterface* iface)
{
    LOGD("(%p)", iface);
    iface->address_as_string = address_as_string_i;
    iface->is_any_address = is_any_address_i;
    iface->is_unicast_address = is_unicast_address_i;
    iface->ipv4 = ipv4_i;
    iface->ipv6 = ipv6_i;
    iface->port = port_i;
    iface->version = version_i;
}

static const RpNetworkAddressIp*
ip_i(RpNetworkAddressInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_NETWORK_ADDRESS_IP(self);
}

static const RpNetworkAddressPipe*
pipe_i(RpNetworkAddressInstance* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return NULL;
}

static const RpNetworkAddressRProxyInternalAddress*
rproxy_internal_address_i(RpNetworkAddressInstance* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return NULL;
}

static const struct sockaddr*
sock_addr_i(RpNetworkAddressInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return (const struct sockaddr*)&RP_NETWORK_ADDRESS_IPV4_INSTANCE(self)->m_ip.m_ipv4.m_address;
}

static socklen_t
sock_addr_len_i(RpNetworkAddressInstance* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return sizeof(struct sockaddr_in);
}

static string_view
address_type_i(RpNetworkAddressInstance* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return string_view_ctor("default", 7);
}

static const char*
as_string_i(RpNetworkAddressInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_ADDRESS_INSTANCE_IFACE(self)->as_string(self);
}

static string_view
as_string_view_i(RpNetworkAddressInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_ADDRESS_INSTANCE_IFACE(self)->as_string_view(self);
}

static const char*
logical_name_i(RpNetworkAddressInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_ADDRESS_INSTANCE_IFACE(self)->logical_name(self);
}

static const RpNetworkAddressSocketInterface*
socket_interface_i(RpNetworkAddressInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_ADDRESS_INSTANCE_IFACE(self)->socket_interface(self);
}

static void
network_address_instance_iface_init(RpNetworkAddressInstanceInterface* iface)
{
    LOGD("(%p)", iface);
    iface->ip = ip_i;
    iface->pipe = pipe_i;
    iface->rproxy_internal_address = rproxy_internal_address_i;
    iface->sock_addr = sock_addr_i;
    iface->sock_addr_len = sock_addr_len_i;
    iface->address_type = address_type_i;
    iface->as_string = as_string_i;
    iface->as_string_view = as_string_view_i;
    iface->logical_name = logical_name_i;
    iface->socket_interface = socket_interface_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpNetworkAddressIpv4Instance* self = RP_NETWORK_ADDRESS_IPV4_INSTANCE(obj);
    g_clear_pointer(&self->m_ip.m_friendly_address, g_free);

    G_OBJECT_CLASS(rp_network_address_ipv4_instance_parent_class)->dispose(obj);
}

static void
rp_network_address_ipv4_instance_class_init(RpNetworkAddressIpv4InstanceClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_network_address_ipv4_instance_init(RpNetworkAddressIpv4Instance* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpNetworkAddressIpv4Instance*
rp_network_address_ipv4_instance_new(const struct sockaddr_in* address, const RpNetworkAddressSocketInterface* sock_interface)
{
    LOGD("(%p)", address);
    RpNetworkAddressIpv4Instance* self = g_object_new(RP_TYPE_NETWORK_ADDRESS_IPV4_INSTANCE,
                                                        "sock-interface", rp_network_address_impl_sock_interface_or_default(sock_interface),
                                                        "type", RpAddressType_IP,
                                                        NULL);
    init_helper(self, address);
    return self;
}

RpNetworkAddressIpv4Instance*
rp_network_address_ipv4_instance_new_2(const char* address, const RpNetworkAddressSocketInterface* sock_interface)
{
    LOGD("(%p(%s))", address, address);
    return rp_network_address_ipv4_instance_new_3(address, 0, sock_interface);
}

RpNetworkAddressIpv4Instance*
rp_network_address_ipv4_instance_new_3(const char* address, guint32 port, const RpNetworkAddressSocketInterface* sock_interface)
{
    LOGD("(%p(%s), %u)", address, address, port);
    RpNetworkAddressIpv4Instance* self = g_object_new(RP_TYPE_NETWORK_ADDRESS_IPV4_INSTANCE,
                                        "sock-interface", rp_network_address_impl_sock_interface_or_default(sock_interface),
                                        "type", RpAddressType_IP,
                                        NULL);
    memset(&self->m_ip.m_ipv4.m_address, 0, sizeof(self->m_ip.m_ipv4.m_address));
    self->m_ip.m_ipv4.m_address.sin_family = AF_INET;
    self->m_ip.m_ipv4.m_address.sin_port = htons(port);
    int rc = inet_pton(AF_INET, address, &self->m_ip.m_ipv4.m_address.sin_addr);
    if (rc != 1)
    {
        int err = errno;
        LOGE("error %d(%s)", err, g_strerror(err));
        g_clear_object(&self);
        return NULL;
    }

    GString* friendly_name_ = g_string_new(address);
    char buf[G_ASCII_DTOSTR_BUF_SIZE];
    gchar* port_str = g_ascii_dtostr(buf, sizeof(buf), port);
    g_string_append_c(friendly_name_, ':');
    g_string_append(friendly_name_, port_str);
    *(rp_network_address_instance_base_friendly_name_(RP_NETWORK_ADDRESS_INSTANCE_BASE(self))) = friendly_name_;
    self->m_ip.m_friendly_address = g_strdup(address);
    return self;
}

RpNetworkAddressIpv4Instance*
rp_network_address_ipv4_instance_new_4(guint32 port, const RpNetworkAddressSocketInterface* sock_interface)
{
    LOGD("(%u)", port);
    RpNetworkAddressIpv4Instance* self = g_object_new(RP_TYPE_NETWORK_ADDRESS_IPV4_INSTANCE,
                                        "sock-interface", rp_network_address_impl_sock_interface_or_default(sock_interface),
                                        "type", RpAddressType_IP,
                                        NULL);
    memset(&self->m_ip.m_ipv4.m_address, 0, sizeof(self->m_ip.m_ipv4.m_address));
    self->m_ip.m_ipv4.m_address.sin_family = AF_INET;
    self->m_ip.m_ipv4.m_address.sin_port = htons(port);
    self->m_ip.m_ipv4.m_address.sin_addr.s_addr = INADDR_ANY;
    GString* friendly_name_ = g_string_new("0.0.0.0:");
    char buf[G_ASCII_DTOSTR_BUF_SIZE];
    gchar* port_str = g_ascii_dtostr(buf, sizeof(buf), port);
    g_string_append(friendly_name_, port_str);
    *(rp_network_address_instance_base_friendly_name_(RP_NETWORK_ADDRESS_INSTANCE_BASE(self))) = friendly_name_;
    self->m_ip.m_friendly_address = g_strdup("0.0.0.0");
    return self;
}

char*
rp_network_address_ipv4_sockaddr_to_string(const struct sockaddr_in* addr)
{
    static const gsize buffer_size = 16; // Enough space to hold an IPv4 address in string form.
    LOGD("(%p)", addr);
    char str[buffer_size];
    char* start = str + buffer_size;
    guint32 ipv4_addr = ntohl(addr->sin_addr.s_addr);
    for (unsigned i = 4; i != 0; --i, ipv4_addr >>= 8)
    {
        guint32 octet = ipv4_addr & 0xff;
        if (octet == 0)
        {
            *--start = '0';
        }
        else
        {
            do
            {
                *--start = '0' + (octet % 10);
                octet /= 10;
            }
            while (octet != 0);
        }
        if (i != 1)
        {
            *--start = '.';
        }
    }
    gsize end = str + buffer_size - start;
    return g_strndup(str, end);
}
