/*
 * rp-dfp-cluster-load-balancer-factory.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_dfp_cluster_load_balancer_factory_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_dfp_cluster_load_balancer_factory_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "dynamic_forward_proxy/rp-cluster.h"

struct _RpDfpClusterLoadBalancerFactory {
    GObject parent_instance;

    RpDfpCluster* m_cluster;
};

static void load_balancer_factory_iface_init(RpLoadBalancerFactoryInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpDfpClusterLoadBalancerFactory, rp_dfp_cluster_load_balancer_factory, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_LOAD_BALANCER_FACTORY, load_balancer_factory_iface_init)
)

static RpLoadBalancerPtr
create_i(RpLoadBalancerFactory* self, RpLoadBalancerParams* params G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p)", self, params);
    return RP_LOAD_BALANCER(
            rp_dfp_cluster_load_balancer_new(RP_DFP_CLUSTER_LOAD_BALANCER_FACTORY(self)->m_cluster));
}

static void
load_balancer_factory_iface_init(RpLoadBalancerFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create = create_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_dfp_cluster_load_balancer_factory_parent_class)->dispose(obj);
}

static void
rp_dfp_cluster_load_balancer_factory_class_init(RpDfpClusterLoadBalancerFactoryClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_dfp_cluster_load_balancer_factory_init(RpDfpClusterLoadBalancerFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpDfpClusterLoadBalancerFactory*
rp_dfp_cluster_load_balancer_factory_new(RpDfpCluster* cluster)
{
    LOGD("(%p)", cluster);
    g_return_val_if_fail(RP_IS_DFP_CLUSTER(cluster), NULL);
    RpDfpClusterLoadBalancerFactory* self = g_object_new(RP_TYPE_DFP_CLUSTER_LOAD_BALANCER_FACTORY, NULL);
    self->m_cluster = cluster;
    return self;
}
