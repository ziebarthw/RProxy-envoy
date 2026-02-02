/*
 * rp-cluster-factory-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "upstream/rp-upstream-impl.h"
#include "rp-cluster-configuration.h"
#include "rp-cluster-factory.h"
#include "rp-load-balancer.h"
#include "rp-net-dns.h"

G_BEGIN_DECLS


#define RP_TYPE_CLUSTER_FACTORY_CONTEXT_IMPL rp_cluster_factory_context_impl_get_type()
G_DECLARE_FINAL_TYPE(RpClusterFactoryContextImpl, rp_cluster_factory_context_impl, RP, CLUSTER_FACTORY_CONTEXT_IMPL, GObject)

RpClusterFactoryContextImpl* rp_cluster_factory_context_impl_new(RpServerFactoryContext* server_context,
                                                                    RpClusterManager* cm,
                                                                    RpLazyCreateDnsResolver dns_resolver_fn,
                                                                    gpointer dns_resolver_arg,
                                                                    bool added_via_api);


/**
 * Base class for all cluster factory implementation. This class can be directly extended if the
 * custom cluster does not have any custom configuration. For custom cluster with custom
 * configuration, use ConfigurableClusterFactoryBase instead.
 */
#define RP_TYPE_CLUSTER_FACTORY_IMPL_BASE rp_cluster_factory_impl_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpClusterFactoryImplBase, rp_cluster_factory_impl_base, RP, CLUSTER_FACTORY_IMPL_BASE, GObject)

struct _RpClusterFactoryImplBaseClass {
    GObjectClass parent_class;

    PairClusterSharedPtrThreadAwareLoadBalancerPtr
                                    (*create_cluster_impl)(RpClusterFactoryImplBase*,
                                    const RpClusterCfg*,
                                    RpClusterFactoryContext*);
};

PairClusterSharedPtrThreadAwareLoadBalancerPtr
rp_cluster_factory_impl_base_create(const RpClusterCfg* cluster,
                                    RpServerFactoryContext* server_context,
                                    RpClusterManager* cm,
                                    RpLazyCreateDnsResolver dns_resolver_fn,
                                    gpointer dns_resolver_arg,
                                    bool added_via_api);
RpNetworkDnsResolverSharedPtr rp_cluster_factory_impl_base_select_dns_resolver(RpClusterFactoryImplBase* self,
                                                                                const RpClusterCfg* cluster,
                                                                                RpClusterFactoryContext* context);


/**
 * Common base class for custom cluster factory with custom configuration.
 * @param ConfigProto is the configuration protobuf.
 */
#define RP_TYPE_CONFIGURABLE_CLUSTER_FACTORY_BASE rp_configurable_cluster_factory_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpConfigurableClusterFactoryBase, rp_configurable_cluster_factory_base, RP, CONFIGURABLE_CLUSTER_FACTORY_BASE, RpClusterFactoryImplBase)

struct _RpConfigurableClusterFactoryBaseClass {
    RpClusterFactoryImplBaseClass parent_class;

#if 0
    gpointer (*create_empty_config_proto)(RpConfigurableClusterFactoryBase*);
#endif//0
    PairClusterSharedPtrThreadAwareLoadBalancerPtr
        (*create_cluster_with_config)(RpConfigurableClusterFactoryBase*, const RpClusterCfg*, gconstpointer, RpClusterFactoryContext*);
};
#if 0
gpointer rp_configurable_cluster_factory_base_create_empty_config_proto(RpConfigurableClusterFactoryBase* self);
#endif//0

G_END_DECLS
