/*
 * rp-strict-dns-cluster.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "upstream/rp-cluster-factory-impl.h"
#include "upstream/rp-upstream-impl.h"
#include "rp-cluster-configuration.h"
#include "rp-net-dns.h"

G_BEGIN_DECLS

typedef struct _RpDnsClusterCfg RpDnsClusterCfg;
struct _RpDnsClusterCfg {
    struct RefreshRate {
        guint64 base_interval;
        guint64 max_interval; // Default; 10 times base_interval value.
    } dns_failure_refresh_rate;
    guint64 dns_refresh_rate; // Default: DNS refresh rate.
    bool respect_dns_ttl;
    guint64 dns_jitter;
    RpDnsLookupFamily_e dns_lookup_family; // Default: RpDnsLookupFamily_V4_PREFERRED.//REVISIT
    bool all_addresses_in_single_endpoint;
};

static inline guint64
rp_dns_cluster_cfg_dns_refresh_rate(const RpDnsClusterCfg* self)
{
    return self->dns_refresh_rate;
}
static inline bool
rp_dns_cluster_cfg_respect_dns_ttl(const RpDnsClusterCfg* self)
{
    return self->respect_dns_ttl;
}
static inline RpDnsLookupFamily_e
rp_dns_cluster_cfg_dns_lookup_family(const RpDnsClusterCfg* self)
{
    return self->dns_lookup_family;
}


/**
 * Implementation of Upstream::Cluster that does periodic DNS resolution and updates the host
 * member set if the DNS members change.
 */
#define RP_TYPE_STRICT_DNS_CLUSTER_IMPL rp_strict_dns_cluster_impl_get_type()
G_DECLARE_FINAL_TYPE(RpStrictDnsClusterImpl, rp_strict_dns_cluster_impl, RP, STRICT_DNS_CLUSTER_IMPL, RpBaseDynamicClusterImpl)

RpStrictDnsClusterImpl* rp_strict_dns_cluster_impl_new(const RpClusterCfg* cluster,
                                                        const RpDnsClusterCfg* dns_cluster,
                                                        RpClusterFactoryContext* context,
                                                        RpNetworkDnsResolverSharedPtr dns_resolver,
                                                        RpStatusCode_e* creation_status);
PairClusterSharedPtrThreadAwareLoadBalancerPtr rp_strict_dns_cluster_impl_create(const RpClusterCfg* cluster,
                                                                                    const RpDnsClusterCfg* dns_cluster,
                                                                                    RpClusterFactoryContext* context,
                                                                                    RpNetworkDnsResolverSharedPtr dns_resolver);

/**
 * Factory for StrictDnsClusterImpl
 */
#define RP_TYPE_STRICT_DNS_CLUSTER_FACTORY rp_strict_dns_cluster_factory_get_type()
G_DECLARE_FINAL_TYPE(RpStrictDnsClusterFactory, rp_strict_dns_cluster_factory, RP, STRICT_DNS_CLUSTER_FACTORY, RpClusterFactoryImplBase)

RpStrictDnsClusterFactory* rp_strict_dns_cluster_factory_new(void);

G_END_DECLS
