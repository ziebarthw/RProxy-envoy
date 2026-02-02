/*
 * rp-net-dns.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-cluster-configuration.h"
#include "rp-typed-config.h"
#include "rp-net-address.h"
#include "rp-time.h"

G_BEGIN_DECLS

typedef enum {
    RpCancelReason_QUERY_ABANDONED,
    RpCancelReason_TIMEOUT
} RpCancelReason_e;

typedef struct _RpNetworkActiveDnsQueryTrace RpNetworkActiveDnsQueryTrace;
struct _RpNetworkActiveDnsQueryTrace {
    guint8 m_trace;
    RpMonotonicTime m_time;
};


/**
 * An active async DNS query.
 */
#define RP_TYPE_NETWORK_ACTIVE_DNS_QUERY rp_network_active_dns_query_get_type()
G_DECLARE_INTERFACE(RpNetworkActiveDnsQuery, rp_network_active_dns_query, RP, NETWORK_ACTIVE_DNS_QUERY, GObject)

struct _RpNetworkActiveDnsQueryInterface {
    GTypeInterface parent_iface;

    void (*cancel)(RpNetworkActiveDnsQuery*, RpCancelReason_e);
    void (*add_trace)(RpNetworkActiveDnsQuery*, guint8);
    char* (*get_traces)(RpNetworkActiveDnsQuery*);
};

static inline void
rp_network_active_dns_query_cancel(RpNetworkActiveDnsQuery* self, RpCancelReason_e reason)
{
    if (RP_IS_NETWORK_ACTIVE_DNS_QUERY(self)) \
        RP_NETWORK_ACTIVE_DNS_QUERY_GET_IFACE(self)->cancel(self, reason);
}
static inline void
rp_network_active_dns_query_add_trace(RpNetworkActiveDnsQuery* self, guint8 trace)
{
    if (RP_IS_NETWORK_ACTIVE_DNS_QUERY(self)) \
        RP_NETWORK_ACTIVE_DNS_QUERY_GET_IFACE(self)->add_trace(self, trace);
}
static inline char*
rp_network_active_dns_get_traces(RpNetworkActiveDnsQuery* self)
{
    return RP_IS_NETWORK_ACTIVE_DNS_QUERY(self) ?
        RP_NETWORK_ACTIVE_DNS_QUERY_GET_IFACE(self)->get_traces(self) : NULL;
}


/**
 * DNS A/AAAA record response.
 */
typedef struct _RpAddrInfoResponse RpAddrInfoResponse;
struct _RpAddrInfoResponse {
  RpNetworkAddressInstanceConstSharedPtr m_address;
  guint64 m_ttl;
};


/**
 * DNS SRV record response.
 */
typedef struct _RpSrvResponse RpSrvResponse;
struct _RpSrvResponse {
  const char* m_host;
  guint16 m_port;
  guint16 m_priority;
  guint16 m_weight;
};
static inline RpSrvResponse
rp_srv_response_ctor(const char* host, guint16 port, guint16 priority, guint16 weight)
{
    RpSrvResponse self = {
        .m_host = host,
        .m_port = port,
        .m_priority = priority,
        .m_weight = weight
    };
    return self;
}


/**
 * Final status for a DNS resolution.
 * DNS resolution can return result statuses like NODATA、SERVFAIL and NONAME,
 * which indicate successful completion of the query but
 * no results, and `Completed` is a more accurate way of reflecting that.
 */
typedef enum {
    RpDnsResolutionStatus_COMPLETED,
    RpDnsResolutionStatus_FAILURE
} RpDnsResolutionStatus_e;


typedef enum {
    RpRecordType_A,
    RpRecordType_AAAA,
    RpRecordType_SRV
} RpRecordType_e;


#define RP_TYPE_NETWORK_DNS_RESPONSE rp_network_dns_response_get_type()
G_DECLARE_FINAL_TYPE(RpNetworkDnsResponse, rp_network_dns_response, RP, NETWORK_DNS_RESPONSE, GObject)

RpNetworkDnsResponse* rp_network_dns_response_new(RpNetworkAddressInstanceConstSharedPtr address, guint64 ttl_secs);
RpNetworkDnsResponse* rp_network_dns_response_new_2(const char* host, guint16 port, guint16 priority, guint16 weight);
const RpAddrInfoResponse* rp_network_dns_response_addr_info(RpNetworkDnsResponse* self);
const RpSrvResponse* rp_network_dns_response_srv(RpNetworkDnsResponse* self);


/**
 * Called when a resolution attempt is complete.
 * @param status supplies the final status of the resolution.
 * @param details supplies the details for the current address' resolution.
 * @param response supplies the list of resolved IP addresses and TTLs.
 */
typedef void (*RpNetworkDnsResolveCb)(RpDnsResolutionStatus_e, char* details, GList*, gpointer);


/**
 * An asynchronous DNS resolver.
 */
#define RP_TYPE_NETWORK_DNS_RESOLVER rp_network_dns_resolver_get_type()
G_DECLARE_INTERFACE(RpNetworkDnsResolver, rp_network_dns_resolver, RP, NETWORK_DNS_RESOLVER, GObject)

struct _RpNetworkDnsResolverInterface {
    GTypeInterface parent_iface;

    RpNetworkActiveDnsQuery* (*resolve)(RpNetworkDnsResolver*,
                                        RpDispatcher*,
                                        const char*,
                                        RpDnsLookupFamily_e,
                                        RpNetworkDnsResolveCb,
                                        gpointer);
    void (*reset_networking)(RpNetworkDnsResolver*);
};

typedef SHARED_PTR(RpNetworkDnsResolver) RpNetworkDnsResolverSharedPtr;
typedef RpNetworkDnsResolverSharedPtr (*RpLazyCreateDnsResolver)(gpointer arg);

static inline RpNetworkActiveDnsQuery*
rp_dns_resolver_resolve(RpNetworkDnsResolver* self, RpDispatcher* dispatcher, const char* dns_name, RpDnsLookupFamily_e dns_lookup_family, RpNetworkDnsResolveCb cb, gpointer arg)
{
    return RP_IS_NETWORK_DNS_RESOLVER(self) ?
        RP_NETWORK_DNS_RESOLVER_GET_IFACE(self)->resolve(self, dispatcher, dns_name, dns_lookup_family, cb, arg) :
        NULL;
}
static inline void
rp_dns_resolver_reset_networking(RpNetworkDnsResolver* self)
{
    if (RP_IS_NETWORK_DNS_RESOLVER(self)) \
        RP_NETWORK_DNS_RESOLVER_GET_IFACE(self)->reset_networking(self);
}


G_END_DECLS
