/*
 * rp-configurable-cluster-factory-base.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_configurable_cluster_factory_base_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_configurable_cluster_factory_base_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-cluster-factory-impl.h"

G_DEFINE_ABSTRACT_TYPE(RpConfigurableClusterFactoryBase, rp_configurable_cluster_factory_base, RP_TYPE_CLUSTER_FACTORY_IMPL_BASE)

OVERRIDE PairClusterSharedPtrThreadAwareLoadBalancerPtr
create_cluster_impl(RpClusterFactoryImplBase* self, const RpClusterCfg* cluster, RpClusterFactoryContext* context)
{
    NOISY_MSG_("(%p, %p, %p)", self, cluster, context);
    RpConfigurableClusterFactoryBase* me = RP_CONFIGURABLE_CLUSTER_FACTORY_BASE(self);
#if 0
    gpointer config = rp_configurable_cluster_factory_base_create_empty_config_proto(me);
#endif//0

    gconstpointer config = rp_cluster_type_cfg_typed_config(
                            rp_cluster_cfg_cluster_type(cluster));

    return RP_CONFIGURABLE_CLUSTER_FACTORY_BASE_GET_CLASS(self)->create_cluster_with_config(me, cluster, config, context);
}

static void
cluster_factory_impl_base_class_init(RpClusterFactoryImplBaseClass* klass)
{
    LOGD("(%p)", klass);
    klass->create_cluster_impl = create_cluster_impl;
}

static void
rp_configurable_cluster_factory_base_class_init(RpConfigurableClusterFactoryBaseClass* klass)
{
    LOGD("(%p)", klass);
    cluster_factory_impl_base_class_init(RP_CLUSTER_FACTORY_IMPL_BASE_CLASS(klass));
}

static void
rp_configurable_cluster_factory_base_init(RpConfigurableClusterFactoryBase* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}
