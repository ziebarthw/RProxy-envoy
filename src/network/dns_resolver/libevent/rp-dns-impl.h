/*
 * rp-dns-impl.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-net-dns.h"
#include "rp-net-dns-resolver.h"

G_BEGIN_DECLS

#define RP_DNS_DEFAULT_TTL 60


/**
 * RpDnsResolverImpl
 */
#define RP_TYPE_DNS_RESOLVER_IMPL rp_dns_resolver_impl_get_type()
G_DECLARE_FINAL_TYPE(RpDnsResolverImpl, rp_dns_resolver_impl, RP, DNS_RESOLVER_IMPL, GObject)

RpDnsResolverImpl* rp_dns_resolver_impl_new(RpDispatcher* dispatcher);
void rp_dns_resolver_impl_set_dirty_channel_(RpDnsResolverImpl* self, bool dirty_channel);
bool rp_dns_resolver_impl_filter_unroutable_families_(RpDnsResolverImpl* self);
GHashTable* rp_dns_resolver_impl_inflight_(RpDnsResolverImpl* self);


/**
 * Network::ActiveDnsQuery
 */
#define RP_TYPE_PENDING_RESOLUTION rp_pending_resolution_get_type()
G_DECLARE_DERIVABLE_TYPE(RpPendingResolution, rp_pending_resolution, RP, PENDING_RESOLUTION, GObject)

struct _RpPendingResolutionClass {
    GObjectClass parent_class;

};

typedef struct _RpPendingResponse RpPendingResponse;
struct _RpPendingResponse {
    RpDnsResolutionStatus_e m_status;
    GList* /* <DnsResponse> */ m_address_list;
    GList* /* <PendingResolution> */ m_waiters;
    char* m_details;
};
static inline RpPendingResponse
rp_pending_response_ctor(RpDnsResolutionStatus_e status, GList* address_list, GList* waiters, char* details)
{
    RpPendingResponse self = {
        .m_status = status,
        .m_address_list = address_list,
        .m_waiters = waiters,
        .m_details = details
    };
    return self;
}

RpPendingResolution* rp_pending_resolution_new(RpDnsResolverImpl* parent,
                                                RpNetworkDnsResolveCb cb,
                                                gpointer arg,
                                                RpDispatcher* dispatcher,
                                                const char* dns_name,
                                                RpDnsLookupFamily_e dns_lookup_family);
bool rp_pending_resolution_completed_(RpPendingResolution* self);
void rp_pending_resolution_set_completed_(RpPendingResolution* self,
                                            bool completed);
const char* rp_pending_resolution_dns_name_(RpPendingResolution* self);
RpDnsResolverImpl* rp_pending_resolution_parent_(RpPendingResolution* self);
RpPendingResponse* rp_pending_resolution_pending_response_(RpPendingResolution* self);
const char* rp_pending_resolution_key_(RpPendingResolution* self);
void rp_pending_resolution_add_waiter(RpPendingResolution* self, RpPendingResolution* waiter);
void rp_pending_resolution_finish_resolve(RpPendingResolution* self);
void rp_pending_resolution_set_owned(RpPendingResolution* self, bool owned);


/**
 * RpAddrInfoPendingResolution
 */
#define RP_TYPE_ADDR_INFO_PENDING_RESOLUTION rp_addr_info_pending_resolution_get_type()
G_DECLARE_FINAL_TYPE(RpAddrInfoPendingResolution, rp_addr_info_pending_resolution, RP, ADDR_INFO_PENDING_RESOLUTION, RpPendingResolution)

RpAddrInfoPendingResolution* rp_addr_info_pending_resolution_new(RpDnsResolverImpl* parent,
                                                                    RpNetworkDnsResolveCb cb,
                                                                    gpointer arg,
                                                                    RpDispatcher* dispatcher,
                                                                    evdns_base_t* dns_base,
                                                                    const char* dns_name,
                                                                    RpDnsLookupFamily_e dns_lookup_family);
void rp_addr_info_pending_resolution_start_resolution(RpAddrInfoPendingResolution* self,
                                                        GHashTable* inflight);


/**
 * RpLibeventDnsResolverImplFactory
 */
#define RP_TYPE_LIBEVENT_DNS_RESOLVER_FACTORY rp_libevent_dns_resolver_factory_get_type()
G_DECLARE_FINAL_TYPE(RpLibeventDnsResolverFactory, rp_libevent_dns_resolver_factory, RP, LIBEVENT_DNS_RESOLVER_FACTORY, GObject)

RpLibeventDnsResolverFactory* rp_libevent_dns_resolver_factory_new(void);


G_END_DECLS
