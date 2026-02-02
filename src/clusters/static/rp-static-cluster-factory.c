/*
 * rp-static-cluster-factory.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_static_cluster_factory_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_static_cluster_factory_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "clusters/static/rp-static-cluster.h"

struct _RpStaticClusterFactory {
    RpClusterFactoryImplBase parent_instance;

};

G_DEFINE_FINAL_TYPE(RpStaticClusterFactory, rp_static_cluster_factory, RP_TYPE_CLUSTER_FACTORY_IMPL_BASE)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_static_cluster_factory_parent_class)->dispose(obj);
}

OVERRIDE PairClusterSharedPtrThreadAwareLoadBalancerPtr
create_cluster_impl(RpClusterFactoryImplBase* self, const RpClusterCfg* cluster_config, RpClusterFactoryContext* context)
{
    NOISY_MSG_("(%p, %p, %p)", self, cluster_config, context);
    return rp_static_cluster_impl_create(cluster_config, context);
}

static void
cluster_factory_impl_base_class_init(RpClusterFactoryImplBaseClass* klass)
{
    LOGD("(%p)", klass);
    klass->create_cluster_impl = create_cluster_impl;
}

static void
rp_static_cluster_factory_class_init(RpStaticClusterFactoryClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    cluster_factory_impl_base_class_init(RP_CLUSTER_FACTORY_IMPL_BASE_CLASS(klass));
}

static void
rp_static_cluster_factory_init(RpStaticClusterFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpStaticClusterFactory*
rp_static_cluster_factory_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_STATIC_CLUSTER_FACTORY,
                        "name", "rproxy.cluster.static",
                        NULL);
}
