/*
 * rp-ipv6-instance.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <arpa/inet.h>
#include "macrologger.h"

#if (defined(rp_network_address_ipv6_instance_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_network_address_ipv6_instance_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "network/rp-address-impl.h"

#define PARENT_ADDRESS_INSTANCE_IFACE(s) \
    ((RpNetworkAddressInstanceInterface*)g_type_interface_peek_parent(RP_NETWORK_ADDRESS_INSTANCE_GET_IFACE(s)))

// Helper for zero-initialization (empty IPv6 address ::/0)
const IPv6UInt128 IPv6UInt128_ZERO = { 0, 0 };

typedef struct _RpNetworkAddressIpv6Helper RpNetworkAddressIpv6Helper;
struct _RpNetworkAddressIpv6Helper {
    struct sockaddr_in6 m_address;
    bool m_v6only;
};
static inline RpNetworkAddressIpv6Helper
rp_ipv6_helper_ctor(void)
{
    RpNetworkAddressIpv6Helper self = {
        .m_v6only = true
    };
    memset(&self.m_address, 0, sizeof(self.m_address));
    return self;
}
static inline IPv6UInt128
rp_ipv6_helper_address(RpNetworkAddressIpv6Helper* self)
{
    IPv6UInt128 result = IPv6UInt128_ZERO;
    memcpy(&result, &self->m_address.sin6_addr.s6_addr[0], sizeof(result));
    return result;
}
static inline guint32
rp_ipv6_helper_scope_id(RpNetworkAddressIpv6Helper* self)
{
    return self->m_address.sin6_scope_id;
}
static inline guint32
rp_ipv6_helper_port(RpNetworkAddressIpv6Helper* self)
{
    return ntohs(self->m_address.sin6_port);
}
static inline char*
make_friendly_address(const struct sockaddr_in6* address)
{
    char str[INET6_ADDRSTRLEN];
    const char* ptr = inet_ntop(AF_INET6, &address->sin6_addr, str, INET6_ADDRSTRLEN);
    g_autoptr(GString) s = g_string_new_len(ptr, INET6_ADDRSTRLEN);
    if (address->sin6_scope_id != 0)
    {
        char buf[G_ASCII_DTOSTR_BUF_SIZE];
        gchar* scrope_id = g_ascii_dtostr(buf, sizeof(buf), ntohs(address->sin6_scope_id));
        g_string_append(s, scrope_id);
    }
    return g_string_free_and_steal(s);
}
static inline char*
rp_ipv6_helper_make_friendly_address(RpNetworkAddressIpv6Helper* self)
{
    return make_friendly_address(&self->m_address);
}
static inline bool
rp_ipv6_helper_v6only(RpNetworkAddressIpv6Helper* self)
{
    return self->m_v6only;
}
static inline RpNetworkAddressInstanceConstSharedPtr
rp_ipv6_helper_adddress_without_scope_id(RpNetworkAddressIpv6Helper* self)
{
    struct sockaddr_in6 ret_addr = self->m_address;
    ret_addr.sin6_scope_id = 0;
    return RP_NETWORK_ADDRESS_INSTANCE(
            rp_network_address_ipv6_instance_new(&ret_addr, self->m_v6only, NULL));
}
static inline RpNetworkAddressInstanceConstSharedPtr
rp_ipv6_helper_v4_compatible_address(RpNetworkAddressIpv6Helper* self)
{
    if (!self->m_v6only && IN6_IS_ADDR_V4MAPPED(&self->m_address.sin6_addr))
    {
        struct sockaddr_in sin;
        rp_network_address_ipv6_to_ipv4_compatible_address(&self->m_address, &sin);
        return RP_NETWORK_ADDRESS_INSTANCE(rp_network_address_ipv4_instance_new(&sin, NULL));
    }
    return NULL;
}

typedef struct _RpNetworkAddressIpHelper RpNetworkAddressIpHelper;
struct _RpNetworkAddressIpHelper {
    RpNetworkAddressIpv6Helper m_ipv6;
    char* m_friendly_address;
};

struct _RpNetworkAddressIpv6Instance {
    RpNetworkAddressInstanceBase parent_instance;

    RpNetworkAddressIpHelper m_ip;
};

static void network_address_instance_iface_init(RpNetworkAddressInstanceInterface* iface);
static void network_address_ip_iface_init(RpNetworkAddressIpInterface* iface);
static void network_address_ipv6_iface_init(RpNetworkAddressIpv6Interface* iface);

G_DEFINE_TYPE_WITH_CODE(RpNetworkAddressIpv6Instance, rp_network_address_ipv6_instance, RP_TYPE_NETWORK_ADDRESS_INSTANCE_BASE,
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_ADDRESS_INSTANCE, network_address_instance_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_ADDRESS_IP, network_address_ip_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_ADDRESS_IPV6, network_address_ipv6_iface_init)
)

static void
init_helper(RpNetworkAddressIpv6Instance* self, const struct sockaddr_in6* address, bool v6only)
{
    NOISY_MSG_("(%p, %p, %u)", self, address, v6only);
    self->m_ip.m_ipv6.m_address = *address;
    self->m_ip.m_friendly_address = rp_ipv6_helper_make_friendly_address(&self->m_ip.m_ipv6);
    self->m_ip.m_ipv6.m_v6only = v6only;
    g_autoptr(GString) friendly_name_ = g_string_new("");
    g_string_append_printf(friendly_name_, "[%s]:%u", self->m_ip.m_friendly_address, rp_ipv6_helper_port(&self->m_ip.m_ipv6));
    *(rp_network_address_instance_base_friendly_name_(RP_NETWORK_ADDRESS_INSTANCE_BASE(self))) = g_steal_pointer(&friendly_name_);
}

static IPv6UInt128
address_i(RpNetworkAddressIpv6* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_ipv6_helper_address(&RP_NETWORK_ADDRESS_IPV6_INSTANCE(self)->m_ip.m_ipv6);
}

static RpNetworkAddressInstanceConstSharedPtr
address_without_scope_id_i(RpNetworkAddressIpv6* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_ipv6_helper_adddress_without_scope_id(&RP_NETWORK_ADDRESS_IPV6_INSTANCE(self)->m_ip.m_ipv6);
}

static guint32
scope_id_i(RpNetworkAddressIpv6* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_ipv6_helper_scope_id(&RP_NETWORK_ADDRESS_IPV6_INSTANCE(self)->m_ip.m_ipv6);
}

static RpNetworkAddressInstanceConstSharedPtr
v4_compatible_address_i(RpNetworkAddressIpv6* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_ipv6_helper_v4_compatible_address(&RP_NETWORK_ADDRESS_IPV6_INSTANCE(self)->m_ip.m_ipv6);
}

static bool
v6only_i(RpNetworkAddressIpv6* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_ipv6_helper_v6only(&RP_NETWORK_ADDRESS_IPV6_INSTANCE(self)->m_ip.m_ipv6);
}

static void
network_address_ipv6_iface_init(RpNetworkAddressIpv6Interface* iface)
{
    LOGD("(%p)", iface);
    iface->address = address_i;
    iface->address_without_scope_id = address_without_scope_id_i;
    iface->scope_id = scope_id_i;
    iface->v4_compatible_address = v4_compatible_address_i;
    iface->v6only = v6only_i;
}

static const char*
address_as_string_i(RpNetworkAddressIp* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_NETWORK_ADDRESS_IPV6_INSTANCE(self)->m_ip.m_friendly_address;
}

static bool
is_any_address_i(RpNetworkAddressIp* self)
{
    NOISY_MSG_("(%p)", self);
    return memcmp(&RP_NETWORK_ADDRESS_IPV6_INSTANCE(self)->m_ip.m_ipv6.m_address.sin6_addr, &in6addr_any, sizeof(struct in6_addr)) == 0;
}

static bool
is_unicast_address_i(RpNetworkAddressIp* self)
{
    NOISY_MSG_("(%p)", self);
    RpNetworkAddressIpv6Instance* me = RP_NETWORK_ADDRESS_IPV6_INSTANCE(self);
    return !rp_network_address_ip_is_any_address(self) && !IN6_IS_ADDR_MULTICAST(&me->m_ip.m_ipv6.m_address.sin6_addr);
}

static RpNetworkAddressIpv4*
ipv4_i(RpNetworkAddressIp* self)
{
    NOISY_MSG_("(%p)", self);
    return NULL;
}

static RpNetworkAddressIpv6*
ipv6_i(RpNetworkAddressIp* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return RP_NETWORK_ADDRESS_IPV6(self);
}

static guint32
port_i(RpNetworkAddressIp* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_ipv6_helper_port(&RP_NETWORK_ADDRESS_IPV6_INSTANCE(self)->m_ip.m_ipv6);
}

static RpIpVersion_e
version_i(RpNetworkAddressIp* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return RpIpVersion_v6;
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
    return (const struct sockaddr*)&RP_NETWORK_ADDRESS_IPV6_INSTANCE(self)->m_ip.m_ipv6.m_address;
}

static socklen_t
sock_addr_len_i(RpNetworkAddressInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return sizeof(RP_NETWORK_ADDRESS_IPV6_INSTANCE(self)->m_ip.m_ipv6.m_address);
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

static RpAddressType_e
type_i(RpNetworkAddressInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_ADDRESS_INSTANCE_IFACE(self)->type(self);
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
    iface->logical_name = logical_name_i;
    iface->socket_interface = socket_interface_i;
    iface->type = type_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpNetworkAddressIpv6Instance* self = RP_NETWORK_ADDRESS_IPV6_INSTANCE(obj);
    g_clear_pointer(&self->m_ip.m_friendly_address, g_free);

    G_OBJECT_CLASS(rp_network_address_ipv6_instance_parent_class)->dispose(obj);
}

static void
rp_network_address_ipv6_instance_class_init(RpNetworkAddressIpv6InstanceClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_network_address_ipv6_instance_init(RpNetworkAddressIpv6Instance* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_ip.m_ipv6 = rp_ipv6_helper_ctor();
}

RpNetworkAddressIpv6Instance*
rp_network_address_ipv6_instance_new(const struct sockaddr_in6* address, bool v6only/* = true*/, const RpNetworkAddressSocketInterface* sock_interface)
{
    LOGD("(%p, %u, %p)", address, v6only, sock_interface);
    RpNetworkAddressIpv6Instance* self = g_object_new(RP_TYPE_NETWORK_ADDRESS_IPV6_INSTANCE,
                                                        "sock-interface", rp_network_address_impl_sock_interface_or_default(sock_interface),
                                                        "type", RpAddressType_IP,
                                                        NULL);
    init_helper(self, address, v6only);
    return self;
}

RpNetworkAddressIpv6Instance*
rp_network_address_ipv6_instance_new_2(const char* address, const RpNetworkAddressSocketInterface* sock_interface)
{
    LOGD("(%p(%s), %p)", address, address, sock_interface);
    return rp_network_address_ipv6_instance_new_3(address, 0, sock_interface, true);
}

RpNetworkAddressIpv6Instance*
rp_network_address_ipv6_instance_new_3(const char* address, guint32 port, const RpNetworkAddressSocketInterface* sock_interface, bool v6only/* = true*/)
{
    LOGD("(%p(%s), %u, %p, %u)", address, address, v6only, sock_interface, v6only);
    RpNetworkAddressIpv6Instance* self = g_object_new(RP_TYPE_NETWORK_ADDRESS_IPV6_INSTANCE,
                                                        "sock-interface", rp_network_address_impl_sock_interface_or_default(sock_interface),
                                                        "type", RpAddressType_IP,
                                                        NULL);
    struct sockaddr_in6 addr_in;
    memset(&addr_in, 0, sizeof(addr_in));
    addr_in.sin6_family = AF_INET6;
    addr_in.sin6_port = htons(port);
    if (!address || !address[0]) // !address.empty()
    {
        if (inet_pton(AF_INET6, address, &addr_in.sin6_addr) != 1)
        {
            int err = errno;
            LOGE("error %d(%s)", err, g_strerror(err));
            g_clear_object(&self);
            return NULL;
        }
    }
    else
    {
        addr_in.sin6_addr = in6addr_any;
    }
    init_helper(self, &addr_in, v6only);
    return self;
}

RpNetworkAddressIpv6Instance*
rp_network_address_ipv6_instance_new_4(guint32 port, const RpNetworkAddressSocketInterface* sock_interface)
{
    LOGD("(%u, %p)", port, sock_interface);
    return rp_network_address_ipv6_instance_new_3("", port, sock_interface, true);
}

char*
rp_network_address_ipv6_sockaddr_to_string(const struct sockaddr_in6* addr)
{
    LOGD("(%p)", addr);
    return make_friendly_address(addr);
}
