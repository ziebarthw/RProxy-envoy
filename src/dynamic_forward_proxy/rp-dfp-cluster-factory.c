/*
 * rp-dfp-cluster-factory.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_factory_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_factory_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "dynamic_forward_proxy/rp-cluster.h"

struct _RpDfpClusterFactory {
    RpClusterFactoryImplBase parent_instance;


};

G_DEFINE_TYPE(RpDfpClusterFactory, rp_dfp_cluster_factory, RP_TYPE_CLUSTER_FACTORY_IMPL_BASE)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_dfp_cluster_factory_parent_class)->dispose(obj);
}

static inline RpSingletonManager*
singleton_manager_from_cluster_factory_context(RpClusterFactoryContext* context)
{
    return rp_common_factory_context_singleton_manager(RP_COMMON_FACTORY_CONTEXT(
               rp_cluster_factory_context_server_factory_context(context)));
}

OVERRIDE PairClusterSharedPtrThreadAwareLoadBalancerPtr
create_cluster_impl(RpClusterFactoryImplBase* self, const RpClusterCfg* cluster_config, RpClusterFactoryContext* context)
{
    extern RpDfpClusterStoreFactory* default_cluster_store_factory;
    NOISY_MSG_("(%p, %p, %p)", self, cluster_config, context);

    RpStatusCode_e creation_status = RpStatusCode_Ok;
    RpDfpClusterImpl* new_cluster = rp_dfp_cluster_impl_new(cluster_config, context, &creation_status);
    if (creation_status != RpStatusCode_Ok)
    {
        LOGE("failed");
        return PairClusterSharedPtrThreadAwareLoadBalancerPtr_make(NULL, NULL);
    }

    RpDfpClusterStoreSharedPtr cluster_store = rp_dfp_cluster_store_factory_get(default_cluster_store_factory);
    const char* cluster_name = rp_cluster_info_name(rp_cluster_info(RP_CLUSTER(new_cluster)));
    rp_dfp_cluster_store_save(cluster_store, cluster_name, RP_DFP_CLUSTER(new_cluster));
    RpThreadAwareLoadBalancerPtr lb = RP_THREAD_AWARE_LOAD_BALANCER(
                                        rp_dfp_cluster_thread_aware_load_balancer_new(RP_DFP_CLUSTER(new_cluster)));
    return PairClusterSharedPtrThreadAwareLoadBalancerPtr_make(
            RP_CLUSTER(new_cluster), g_steal_pointer(&lb));
}

static void
cluster_factory_impl_base_class_init(RpClusterFactoryImplBaseClass* klass)
{
    LOGD("(%p)", klass);
    klass->create_cluster_impl = create_cluster_impl;
}

static void
rp_dfp_cluster_factory_class_init(RpDfpClusterFactoryClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    cluster_factory_impl_base_class_init(RP_CLUSTER_FACTORY_IMPL_BASE_CLASS(klass));
}

static void
rp_dfp_cluster_factory_init(RpDfpClusterFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpDfpClusterFactory*
rp_dfp_cluster_factory_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_DFP_CLUSTER_FACTORY,
                        "name", "rproxy.cluster.dynamic_forward_proxy",
                        NULL);
}
