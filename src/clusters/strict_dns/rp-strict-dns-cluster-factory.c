/*
 * rp-strict-dns-cluster-factory.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_strict_dns_cluster_factory_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_strict_dns_cluster_factory_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "network/dns_resolver/libevent/rp-dns-impl.h"
#include "clusters/strict_dns/rp-strict-dns-cluster.h"

struct _RpStrictDnsClusterFactory {
    RpClusterFactoryImplBase parent_instance;

};

G_DEFINE_FINAL_TYPE(RpStrictDnsClusterFactory, rp_strict_dns_cluster_factory, RP_TYPE_CLUSTER_FACTORY_IMPL_BASE)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_strict_dns_cluster_factory_parent_class)->dispose(obj);
}

OVERRIDE PairClusterSharedPtrThreadAwareLoadBalancerPtr
create_cluster_impl(RpClusterFactoryImplBase* self, const RpClusterCfg* cluster, RpClusterFactoryContext* context)
{
    NOISY_MSG_("(%p, %p, %p)", self, cluster, context);
    RpDnsClusterCfg proto_config = {
        .dns_failure_refresh_rate = {
            .base_interval = 60,
            .max_interval = 10*60
        },
        .dns_refresh_rate = 60,
        .respect_dns_ttl = false,
        .dns_jitter = 0,
        .dns_lookup_family = RpDnsLookupFamily_V4_PREFERRED,//REVISIT - config-driven!
        .all_addresses_in_single_endpoint = true
    };
    RpNetworkDnsResolverSharedPtr dns_resolver = rp_cluster_factory_impl_base_select_dns_resolver(self, cluster, context);
    return rp_strict_dns_cluster_impl_create(cluster, &proto_config, context, RP_NETWORK_DNS_RESOLVER(dns_resolver));
}

static void
cluster_factory_impl_base_class_init(RpClusterFactoryImplBaseClass* klass)
{
    LOGD("(%p)", klass);
    klass->create_cluster_impl = create_cluster_impl;
}

static void
rp_strict_dns_cluster_factory_class_init(RpStrictDnsClusterFactoryClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    cluster_factory_impl_base_class_init(RP_CLUSTER_FACTORY_IMPL_BASE_CLASS(klass));
}

static void
rp_strict_dns_cluster_factory_init(RpStrictDnsClusterFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpStrictDnsClusterFactory*
rp_strict_dns_cluster_factory_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_STRICT_DNS_CLUSTER_FACTORY, NULL);
}
