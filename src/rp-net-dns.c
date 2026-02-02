/*
 * rp-net-dns.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_net_dns_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_net_dns_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-net-dns.h"

G_DEFINE_INTERFACE(RpNetworkActiveDnsQuery, rp_network_active_dns_query, G_TYPE_OBJECT)
static void
rp_network_active_dns_query_default_init(RpNetworkActiveDnsQueryInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpNetworkDnsResolver, rp_network_dns_resolver, G_TYPE_OBJECT)
static void
rp_network_dns_resolver_default_init(RpNetworkDnsResolverInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}


struct _RpNetworkDnsResponse {
    GObject parent_instance;

    union {
        RpAddrInfoResponse m_addr_info;
        RpSrvResponse m_srv;
    } m_response;
};

G_DEFINE_TYPE(RpNetworkDnsResponse, rp_network_dns_response, G_TYPE_OBJECT)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_network_dns_response_parent_class)->dispose(obj);
}

static void
rp_network_dns_response_class_init(RpNetworkDnsResponseClass* klass)
{
    LOGD("(%p)", klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_network_dns_response_init(RpNetworkDnsResponse* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpNetworkDnsResponse*
rp_network_dns_response_new(RpNetworkAddressInstanceConstSharedPtr address, guint64 ttl_secs)
{
    LOGD("(%p, %zu)", address, ttl_secs);
    RpNetworkDnsResponse* self = g_object_new(RP_TYPE_NETWORK_DNS_RESPONSE, NULL);
    RpAddrInfoResponse addr_info = {
        .m_address = address,
        .m_ttl = /*TODO...MIN(MAX(...))*/ ttl_secs
    };
    self->m_response.m_addr_info = addr_info;
    return self;
}

RpNetworkDnsResponse*
rp_network_dns_response_new_2(const char* host, guint16 port, guint16 priority, guint16 weight)
{
    LOGD("(%p(%s), %u, %u, %u)", host, host, port, priority, weight);
    RpNetworkDnsResponse* self = g_object_new(RP_TYPE_NETWORK_DNS_RESPONSE, NULL);
    self->m_response.m_srv = rp_srv_response_ctor(host, port, priority, weight);
    return self;
}

const RpAddrInfoResponse*
rp_network_dns_response_addr_info(RpNetworkDnsResponse* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_NETWORK_DNS_RESPONSE(self), NULL);
    return &self->m_response.m_addr_info;
}

const RpSrvResponse*
rp_network_dns_response_srv(RpNetworkDnsResponse* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_NETWORK_DNS_RESPONSE(self), NULL);
    return &self->m_response.m_srv;
}
