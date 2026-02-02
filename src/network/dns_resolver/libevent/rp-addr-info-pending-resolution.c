/*
 * rp-addr-info-pending-resolution.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _GNU_SOURCE // Only needed for vscode intellisense.
#define _GNU_SOURCE
#endif
#include "macrologger.h"

#if (defined(rp_addr_info_pending_resolution_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_addr_info_pending_resolution_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <sys/types.h>
#include <ifaddrs.h>

#include <arpa/inet.h>  // inet_ntop
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>

#include "rp-api-os-sys-calls.h"
#include "network/rp-address-impl.h"
#include "network/dns_resolver/libevent/rp-dns-impl.h"

typedef struct _AvailableInterfaces AvailableInterfaces;
struct _AvailableInterfaces {
    bool m_v4_available;
    bool m_v6_available;
};
static inline AvailableInterfaces
available_interfaces_ctor(bool v4_available, bool v6_available)
{
    AvailableInterfaces self = {
        .m_v4_available = v4_available,
        .m_v6_available = v6_available
    };
    return self;
}

struct _RpAddrInfoPendingResolution {
    RpPendingResolution parent_instance;

    evdns_base_t* m_dns_base;

    guint32 m_pending_resolutions;
    int m_family;
    RpDnsLookupFamily_e m_dns_lookup_family;
    AvailableInterfaces m_available_interfaces;
    bool m_dual_resolution;
};

G_DEFINE_FINAL_TYPE(RpAddrInfoPendingResolution, rp_addr_info_pending_resolution, RP_TYPE_PENDING_RESOLUTION)

static void start_resolution_impl(RpAddrInfoPendingResolution* self, int family);

typedef struct _RpInterfaceAddress RpInterfaceAddress;
struct _RpInterfaceAddress {
    char* m_interface_name;
    unsigned int m_interface_flags;
    RpNetworkAddressInstanceConstSharedPtr m_interface_addr;
};
typedef GPtrArray* RpInterfaceAddressVector;
RpInterfaceAddress*
rp_interface_address_new(const char* interface_name, unsigned int interface_flags, RpNetworkAddressInstanceConstSharedPtr interface_addr)
{
    LOGD("(%p(%s), %u, %p)", interface_name, interface_name, interface_flags, interface_addr);
    RpInterfaceAddress* self = g_new(RpInterfaceAddress, 1);
    self->m_interface_name = g_strdup(interface_name);
    self->m_interface_flags = interface_flags;
    self->m_interface_addr = interface_addr;
    return self;
}
void
rp_interface_address_free(RpInterfaceAddress* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(self != NULL);
    g_clear_pointer(&self->m_interface_name, g_free);
    g_clear_object(&self->m_interface_addr);
    g_free(self);
}

static inline RpSysCallIntResult
rp_sys_call_int_result_ctor(int rc, int errno_)
{
    RpSysCallIntResult self = {
        .m_return_value = rc,
        .m_errno = errno_
    };
    return self;
}

static RpSysCallIntResult
rp_os_syscalls_impl_getifaddrs(RpInterfaceAddressVector* interfaces)
{
    LOGD("(%p)", interfaces);
    struct ifaddrs* ifaddr;
    struct ifaddrs* ifa;

    int rc = getifaddrs(&ifaddr);
    if (rc == -1)
    {
        int err = errno;
        LOGE("error %d(%s)", err, g_strerror(err));
        return rp_sys_call_int_result_ctor(rc, err);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }

        if (ifa->ifa_addr->sa_family == AF_INET || ifa->ifa_addr->sa_family == AF_INET6)
        {
            const struct sockaddr_storage* ss = (struct sockaddr_storage*)ifa->ifa_addr;
            size_t ss_len = ifa->ifa_addr->sa_family == AF_INET ?
                                sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
            RpNetworkAddressInstanceConstSharedPtr address =
                rp_network_address_address_from_sock_addr(ss, ss_len, ifa->ifa_addr->sa_family == AF_INET6);
            if (address)
            {
                g_ptr_array_add(*interfaces, rp_interface_address_new(ifa->ifa_name, ifa->ifa_flags, address));
            }
        }
    }

    if (ifaddr)
    {
        freeifaddrs(ifaddr);
    }

    return rp_sys_call_int_result_ctor(rc, 0);
}

bool
rp_network_utility_is_loopback_address(RpNetworkAddressInstance* address)
{
    LOGD("(%p)", address);
    if (rp_network_address_instance_type(address) != RpNetworkAddressType_IP)
    {
        return false;
    }

    RpNetworkAddressIp* ip = rp_network_address_instance_ip(address);
    if (rp_network_address_ip_version(ip) == RpIpVersion_v4)
    {
        return rp_network_address_ipv4_address(rp_network_address_ip_ipv4(ip)) == htonl(INADDR_LOOPBACK);
    }
    else if (rp_network_address_ip_version(ip) == RpIpVersion_v6)
    {
        IPv6UInt128 addr = rp_network_address_ipv6_address(rp_network_address_ip_ipv6(ip));
        return memcmp(&addr, &in6addr_loopback, sizeof(in6addr_loopback)) == 0;
    }
    LOGE("bug - unexpected address type");
    return false;
}

static AvailableInterfaces
available_interfaces(RpAddrInfoPendingResolution* self)
{
    NOISY_MSG_("(%p)", self);
    g_autoptr(GPtrArray) interface_addresses =
        g_ptr_array_new_with_free_func((GDestroyNotify)rp_interface_address_free);
    RpSysCallIntResult rc = rp_os_syscalls_impl_getifaddrs(&interface_addresses);
    if (rc.m_return_value != 0)
    {
        LOGE("dns resolution for \"%s\" could not obtain interface information with error %d",
            rp_pending_resolution_dns_name_(RP_PENDING_RESOLUTION(self)), rc.m_errno);
        return available_interfaces_ctor(true, true);
    }

    AvailableInterfaces interfaces = available_interfaces_ctor(false, false);
    for (guint i = 0; i < interface_addresses->len; ++i)
    {
        RpInterfaceAddress* interface_address = g_ptr_array_index(interface_addresses, i);
        if (!rp_network_address_instance_ip(interface_address->m_interface_addr))
        {
            NOISY_MSG_("not ip address");
            continue;
        }

        if (rp_network_utility_is_loopback_address(interface_address->m_interface_addr))
        {
            NOISY_MSG_("loopback");
            continue;
        }

        switch (rp_network_address_ip_version(rp_network_address_instance_ip(interface_address->m_interface_addr)))
        {
            case RpIpVersion_v4:
            {
                interfaces.m_v4_available = true;
                if (interfaces.m_v6_available)
                {
                    return interfaces;
                }
                break;
            }
            case RpIpVersion_v6:
                interfaces.m_v6_available = true;
                if (interfaces.m_v4_available)
                {
                    return interfaces;
                }
                break;
        }
    }
    return interfaces;
}

static char*
make_key(const char* dns_name, int family)
{
    NOISY_MSG_("(%p(%s), %d)", dns_name, dns_name, family);
    return g_strdup_printf("%d|%s", family, dns_name);
}

#ifdef WITH_HIGH_LEVEL_DNS
static bool
is_response_with_no_records(int status)
{
    NOISY_MSG_("(%d)", status);
    return status == DNS_ERR_NODATA || status == DNS_ERR_NOTEXIST;
}
#else
static bool
is_response_with_no_records(int status, int count)
{
    NOISY_MSG_("(%d)", status);
    return status == DNS_ERR_NODATA || status == DNS_ERR_NOTEXIST || count == 0;
}
#endif//WITH_HIGH_LEVEL_DNS

#ifdef WITH_HIGH_LEVEL_DNS
static void
evdns_get_addr_info_cb(int errcode, struct evutil_addrinfo *addr, gpointer arg)
{
    NOISY_MSG_("(%d, %p, %p)", errcode, addr, arg);
    RpAddrInfoPendingResolution* self = RP_ADDR_INFO_PENDING_RESOLUTION(arg);
    RpPendingResolution* pending_resolution = RP_PENDING_RESOLUTION(self);
    RpDnsResolverImpl* parent_ = rp_pending_resolution_parent_(pending_resolution);
    g_assert(self->m_pending_resolutions > 0);
    --self->m_pending_resolutions;

    //TODO...parent_.stats_.resolve_total_.inc();...

    if (errcode != DNS_ERR_NONE)
    {
        //TODO...parent_.chargeGetAddrInfoErrorStats(status, timeouts);...

        if (!is_response_with_no_records(errcode))
        {
            LOGD("evdns_resolution_failure: dns resolution for \"%s\" failed with status %d",
                rp_pending_resolution_dns_name_(pending_resolution), errcode);
        }
        else
        {
            LOGD("evdns_resolution_no_records: dns resolution without records for \"%s\"",
                rp_pending_resolution_dns_name_(pending_resolution));
        }
    }

    if (!self->m_dual_resolution)
    {
        rp_pending_resolution_set_completed_(pending_resolution, true);

        if (errcode == DNS_ERR_REFUSED ||
            errcode == DNS_ERR_SERVERFAILED ||
            errcode == DNS_ERR_NOTIMPL)
        {
            rp_dns_resolver_impl_set_dirty_channel_(parent_, true);
        }
    }

    RpPendingResponse* pending_response_ = rp_pending_resolution_pending_response_(pending_resolution);
    if (errcode == DNS_ERR_NONE)
    {
        pending_response_->m_status = RpDnsResolutionStatus_COMPLETED;
        pending_response_->m_details = g_strconcat("evdns_success:", evutil_gai_strerror(errcode), NULL);

        if (addr)
        {
            bool can_process_v4 =
                (!rp_dns_resolver_impl_filter_unroutable_families(parent_) || self->m_available_interfaces.m_v4_available);
            bool can_process_v6 =
                (!rp_dns_resolver_impl_filter_unroutable_families(parent_) || self->m_available_interfaces.m_v6_available);

            //TODO...int min_ttl = INT_MAX;...

            //TODO...loop through CNAM and get min_ttl...

            for (struct evutil_addrinfo* ai = addr; ai; ai = ai->ai_next)
            {
                if (ai->ai_family == AF_INET && can_process_v4)
                {
                    struct sockaddr_in address;
                    memset(&address, 0, sizeof(address));
                    address.sin_family = AF_INET;
                    address.sin_port = 0;
                    address.sin_addr = ((struct sockaddr_in*)ai->ai_addr)->sin_addr;

                    RpNetworkAddressIpv4Instance* addr_ = rp_network_address_ipv4_instance_new(&address, NULL);
                    RpNetworkDnsResponse* response = rp_network_dns_response_new(RP_NETWORK_ADDRESS_INSTANCE(addr_), RP_DNS_DEFAULT_TTL/*TODO...get from ai somehow?*/);
                    pending_response_->m_address_list = g_list_prepend(pending_response_->m_address_list, response);
                }
                else if (ai->ai_family == AF_INET6 && can_process_v6)
                {
                    struct sockaddr_in6 address;
                    memset(&address, 0, sizeof(address));
                    address.sin6_family = AF_INET6;
                    address.sin6_port = 0;
                    address.sin6_addr = ((struct sockaddr_in6*)ai->ai_addr)->sin6_addr;

                    RpNetworkAddressIpv6Instance* addr_ = rp_network_address_ipv6_instance_new(&address, true, NULL);
                    RpNetworkDnsResponse* response = rp_network_dns_response_new(RP_NETWORK_ADDRESS_INSTANCE(addr_), RP_DNS_DEFAULT_TTL/*TODO...get from ai somehow?*/);
                    pending_response_->m_address_list = g_list_prepend(pending_response_->m_address_list, response);
                }
            }

            pending_response_->m_address_list = g_list_reverse(pending_response_->m_address_list);

            evutil_freeaddrinfo(addr);
        }

        if (pending_response_->m_address_list && self->m_dns_lookup_family != RpDnsLookupFamily_ALL)
        {
            rp_pending_resolution_set_completed_(pending_resolution, true);
        }
    }
    else if (is_response_with_no_records(errcode))
    {
        pending_response_->m_status = RpDnsResolutionStatus_COMPLETED;
        pending_response_->m_details = g_strconcat("evdns_norecords:", evutil_gai_strerror(errcode), NULL);
    }
    else
    {
        pending_response_->m_details = g_strconcat("evdns_failure:", evutil_gai_strerror(errcode), NULL);
    }

    if (errcode == DNS_ERR_TIMEOUT)
    {
        LOGD("DNS request timed out"); //TODO...%d times
    }

    if (rp_pending_resolution_completed_(pending_resolution))
    {
        rp_pending_resolution_finish_resolve(pending_resolution);
        return;
    }

    if (self->m_dual_resolution)
    {
        self->m_dual_resolution = false;

        if (self->m_dns_lookup_family == RpDnsLookupFamily_AUTO)
        {
            self->m_family = AF_INET;
            start_resolution_impl(self, AF_INET);
        }
        else if (self->m_dns_lookup_family == RpDnsLookupFamily_V4_PREFERRED)
        {
            self->m_family = AF_INET6;
            start_resolution_impl(self, AF_INET6);
        }
    }
}
#else
static inline struct sockaddr_in
init_sockaddr_in(struct in_addr* addr)
{
    struct sockaddr_in self = {0};
    self.sin_family = AF_INET;
    self.sin_addr = *addr;
    return self;
}
static inline struct sockaddr_in6
init_sockaddr_in6(struct in6_addr* addr)
{
    struct sockaddr_in6 self = {0};
    self.sin6_family = AF_INET6;
    self.sin6_addr = *addr;
    return self;
}
static void
evdns_get_addr_info_cb(int status, char type, int count, int ttl, void* addresses, gpointer arg)
{
    NOISY_MSG_("(%d, %d, %d, %d, %p, %p(%s))",
        status, type, count, ttl, addresses, arg, rp_pending_resolution_dns_name_(RP_PENDING_RESOLUTION(arg)));

    RpAddrInfoPendingResolution* self = RP_ADDR_INFO_PENDING_RESOLUTION(arg);
    RpPendingResolution* pending_resolution = RP_PENDING_RESOLUTION(self);
    RpDnsResolverImpl* parent_ = rp_pending_resolution_parent_(pending_resolution);
    g_assert(self->m_pending_resolutions > 0);
    --self->m_pending_resolutions;

    //TODO...parent_.stats_.resolve_total_.inc();...

    if (status != DNS_ERR_NONE)
    {
        //TODO...parent_.chargeGetAddrInfoErrorStats(status, timeouts);...

        if (!is_response_with_no_records(status, count))
        {
            LOGD("evdns_resolution_failure: dns resolution for \"%s\" failed with status %d",
                rp_pending_resolution_dns_name_(pending_resolution), status);
        }
        else
        {
            LOGD("evdns_resolution_no_records: dns resolution without records for \"%s\"",
                rp_pending_resolution_dns_name_(pending_resolution));
        }
    }

    if (!self->m_dual_resolution)
    {
        NOISY_MSG_("calling rp_pending_resolution_set_completed_(%p, %u)", pending_resolution, true);
        rp_pending_resolution_set_completed_(pending_resolution, true);

        if (status == DNS_ERR_REFUSED ||
            status == DNS_ERR_SERVERFAILED ||
            status == DNS_ERR_NOTIMPL)
        {
            rp_dns_resolver_impl_set_dirty_channel_(parent_, true);
        }
    }

    RpPendingResponse* pending_response_ = rp_pending_resolution_pending_response_(pending_resolution);
    if ((status == DNS_ERR_NONE) && !(ttl < 0))
    {
        pending_response_->m_status = RpDnsResolutionStatus_COMPLETED;
        pending_response_->m_details = g_strconcat("evdns_success:", evdns_err_to_string(status), NULL);

        switch (type)
        {
            case DNS_IPv4_A:
                if (!rp_dns_resolver_impl_filter_unroutable_families_(parent_) ||
                    self->m_available_interfaces.m_v4_available)
                {
                    struct in_addr* ips = addresses;

                    for (int i = 0; i < count; ++i)
                    {
                        struct sockaddr_in address = init_sockaddr_in(&ips[i]);
                        RpNetworkAddressIpv4Instance* addr_ = rp_network_address_ipv4_instance_new(&address, NULL);
                        RpNetworkDnsResponse* response = rp_network_dns_response_new(RP_NETWORK_ADDRESS_INSTANCE(addr_), ttl);
                        pending_response_->m_address_list = g_list_prepend(pending_response_->m_address_list, response);
                    }
                }
                break;
            case DNS_IPv6_AAAA:
                if (!rp_dns_resolver_impl_filter_unroutable_families_(parent_) ||
                    self->m_available_interfaces.m_v6_available)
                {
                    struct in6_addr* ips = addresses;

                    for (int i = 0; i < count; ++i)
                    {
                        struct sockaddr_in6 address = init_sockaddr_in6(&ips[i]);
                        RpNetworkAddressIpv6Instance* addr_ = rp_network_address_ipv6_instance_new(&address, true, NULL);
                        RpNetworkDnsResponse* response = rp_network_dns_response_new(RP_NETWORK_ADDRESS_INSTANCE(addr_), ttl);
                        pending_response_->m_address_list = g_list_prepend(pending_response_->m_address_list, response);
                    }
                }
                break;
            default:
                break;
        }

        if (pending_response_->m_address_list)
        {
            pending_response_->m_address_list = g_list_reverse(pending_response_->m_address_list);
        }

        if (pending_response_->m_address_list && self->m_dns_lookup_family != RpDnsLookupFamily_ALL)
        {
            NOISY_MSG_("calling rp_pending_resolution_set_completed_(%p, %u)", pending_resolution, true);
            rp_pending_resolution_set_completed_(pending_resolution, true);
        }
    }
    else if (is_response_with_no_records(status, count))
    {
        pending_response_->m_status = RpDnsResolutionStatus_COMPLETED;
        pending_response_->m_details = g_strconcat("evdns_norecords:", evdns_err_to_string(status), NULL);
    }
    else
    {
        pending_response_->m_details = g_strconcat("evdns_failure:", evdns_err_to_string(status), NULL);
    }

    if (status == DNS_ERR_TIMEOUT)
    {
        LOGD("DNS request timed out"); //TODO...%d times
    }

    if (rp_pending_resolution_completed_(pending_resolution))
    {
        rp_pending_resolution_finish_resolve(pending_resolution);
        return;
    }

    if (self->m_dual_resolution)
    {
        self->m_dual_resolution = false;

        if (self->m_dns_lookup_family == RpDnsLookupFamily_AUTO)
        {
            self->m_family = AF_INET;
            start_resolution_impl(self, AF_INET);
        }
        else if (self->m_dns_lookup_family == RpDnsLookupFamily_V4_PREFERRED)
        {
            self->m_family = AF_INET6;
            start_resolution_impl(self, AF_INET6);
        }
    }
}
#endif//WITH_HIGH_LEVEL_DNS

static void
start_resolution_impl(RpAddrInfoPendingResolution* self, int family)
{
    NOISY_MSG_("(%p, %d)", self, family);
    ++self->m_pending_resolutions;
    //TODO...parent_.stats_.pending_resolution_.inc();
    RpPendingResolution* pending_resolution = RP_PENDING_RESOLUTION(self);
    RpDnsResolverImpl* parent_ = rp_pending_resolution_parent_(pending_resolution);
    if (rp_dns_resolver_impl_filter_unroutable_families_(parent_) && family != AF_UNSPEC)
    {
        switch (family)
        {
            case AF_INET:
                if (!self->m_available_interfaces.m_v4_available)
                {
                    LOGD("filtered v4 lookup");
#ifdef WITH_HIGH_LEVEL_DNS
                    evdns_get_addr_info_cb(DNS_ERR_NOTIMPL, NULL, self);
#else
                    evdns_get_addr_info_cb(DNS_ERR_NOTIMPL, DNS_IPv4_A, 0, -1, NULL, self);
#endif//WITH_HIGH_LEVEL_DNS
                    return;
                }
                break;
            case AF_INET6:
                if (!self->m_available_interfaces.m_v6_available)
                {
                    LOGD("filtered v6 lookup");
#ifdef WITH_HIGH_LEVEL_DNS
                    evdns_get_addr_info_cb(DNS_ERR_NOTIMPL, NULL, self);
#else
                    evdns_get_addr_info_cb(DNS_ERR_NOTIMPL, DNS_IPv6_AAAA, 0, -1, NULL, self);
#endif//WITH_HIGH_LEVEL_DNS
                    return;
                }
                break;
            default:
                LOGE("unexpected IP family %d", family);
                break;
        }
    }

    const char* dns_name_ = rp_pending_resolution_dns_name_(pending_resolution);
#ifdef WITH_HIGH_LEVEL_DNS
    struct evutil_addrinfo hints = {0};
    hints.ai_family = family;
    hints.ai_flags = EVUTIL_AI_CANONNAME;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    evdns_getaddrinfo(self->m_dns_base, dns_name_, NULL/*servname*/, &hints, evdns_get_addr_info_cb, self);
#else
    switch (family)
    {
        case AF_INET:
            evdns_base_resolve_ipv4(self->m_dns_base, dns_name_, DNS_QUERY_NO_SEARCH, evdns_get_addr_info_cb, self);
            break;
        case AF_INET6:
            evdns_base_resolve_ipv6(self->m_dns_base, dns_name_, DNS_QUERY_NO_SEARCH, evdns_get_addr_info_cb, self);
            break;
        case AF_UNSPEC:
            evdns_base_resolve_ipv4(self->m_dns_base, dns_name_, DNS_QUERY_NO_SEARCH, evdns_get_addr_info_cb, self);
            evdns_base_resolve_ipv6(self->m_dns_base, dns_name_, DNS_QUERY_NO_SEARCH, evdns_get_addr_info_cb, self);
            ++self->m_pending_resolutions;
            break;
        default:
            LOGE("invalid family %d", family);
            break;
    }
#endif//WITH_HIGH_LEVEL_DNS
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpAddrInfoPendingResolution* self = RP_ADDR_INFO_PENDING_RESOLUTION(obj);
    g_assert(self->m_pending_resolutions == 0);

    G_OBJECT_CLASS(rp_addr_info_pending_resolution_parent_class)->dispose(obj);
}

static void
rp_addr_info_pending_resolution_class_init(RpAddrInfoPendingResolutionClass* klass)
{
    LOGD("(%p)", klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_addr_info_pending_resolution_init(RpAddrInfoPendingResolution* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_dns_lookup_family = AF_INET;
    self->m_pending_resolutions = 0;
    self->m_dual_resolution = false;
}

static int
get_family(RpDnsLookupFamily_e dns_lookup_family)
{
    NOISY_MSG_("(%d)", dns_lookup_family);
    switch (dns_lookup_family)
    {
        case RpDnsLookupFamily_V4_ONLY:
        case RpDnsLookupFamily_V4_PREFERRED:
            return AF_INET;
        case RpDnsLookupFamily_V6_ONLY:
        case RpDnsLookupFamily_AUTO:
            return AF_INET6;
        case RpDnsLookupFamily_ALL:
        default:
            return AF_UNSPEC;
    }
}

static inline RpAddrInfoPendingResolution*
constructed(RpAddrInfoPendingResolution* self)
{
    NOISY_MSG_("(%p)", self);

    if (self->m_dns_lookup_family == RpDnsLookupFamily_AUTO ||
        self->m_dns_lookup_family == RpDnsLookupFamily_V4_PREFERRED ||
        self->m_dns_lookup_family == RpDnsLookupFamily_ALL)
    {
        self->m_dual_resolution = true;
    }
    return self;
}

RpAddrInfoPendingResolution*
rp_addr_info_pending_resolution_new(RpDnsResolverImpl* parent, RpNetworkDnsResolveCb cb, gpointer arg,
                                    RpDispatcher* dispatcher, evdns_base_t* dns_base, const char* dns_name, RpDnsLookupFamily_e dns_lookup_family)
{
    LOGD("(%p, %p, %p, %p, %p(%s), %d)",
        parent, cb, arg, dispatcher, dns_name, dns_name, dns_lookup_family);
    int family = get_family(dns_lookup_family);
    g_autofree char* key = make_key(dns_name, family);
    RpAddrInfoPendingResolution* self = g_object_new(RP_TYPE_ADDR_INFO_PENDING_RESOLUTION,
                                                        "parent", parent,
                                                        "cb", cb,
                                                        "arg", arg,
                                                        "dispatcher", dispatcher,
                                                        "dns-name", dns_name,
                                                        "key", key,
                                                        NULL);
    self->m_dns_base = dns_base;
    self->m_dns_lookup_family = dns_lookup_family;
    self->m_family = family;
    return constructed(self);
}

/**
 * Start a name resolution. This function runs in the main/dns thread.
 */
void
rp_addr_info_pending_resolution_start_resolution(RpAddrInfoPendingResolution* self, GHashTable* inflight)
{
    LOGD("(%p, %p)", self, inflight);

    g_return_if_fail(RP_IS_ADDR_INFO_PENDING_RESOLUTION(self));
    g_return_if_fail(inflight != NULL);

    const char* key = rp_pending_resolution_key_(RP_PENDING_RESOLUTION(self));
    RpPendingResolution* pending_resolution = g_hash_table_lookup(inflight, key);
    if (!pending_resolution)
    {
        self->m_available_interfaces = available_interfaces(self);
        g_hash_table_insert(inflight, g_strdup(key), g_object_ref(self));
        start_resolution_impl(self, self->m_family);
        pending_resolution = RP_PENDING_RESOLUTION(self);
    }
    rp_pending_resolution_add_waiter(pending_resolution, RP_PENDING_RESOLUTION(self));
}
