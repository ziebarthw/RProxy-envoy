/*
 * rp-dfp-cluster-thread-aware-load-balancer.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_dfp_cluster_thread_aware_load_balancer_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_dfp_cluster_thread_aware_load_balancer_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "dynamic_forward_proxy/rp-cluster.h"

struct _RpDfpClusterThreadAwareLoadBalancer {
    GObject parent_instance;

    SHARED_PTR(RpDfpClusterLoadBalancerFactory) m_factory;
};

static void thread_aware_load_balancer_iface_init(RpThreadAwareLoadBalancerInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpDfpClusterThreadAwareLoadBalancer, rp_dfp_cluster_thread_aware_load_balancer, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_THREAD_AWARE_LOAD_BALANCER, thread_aware_load_balancer_iface_init)
)

static RpLoadBalancerFactorySharedPtr
factory_i(RpThreadAwareLoadBalancer* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_LOAD_BALANCER_FACTORY(
            RP_DFP_CLUSTER_THREAD_AWARE_LOAD_BALANCER(self)->m_factory);
}

static RpStatusCode_e
initialize_i(RpThreadAwareLoadBalancer* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return RpStatusCode_Ok;
}

static void
thread_aware_load_balancer_iface_init(RpThreadAwareLoadBalancerInterface* iface)
{
    LOGD("(%p)", iface);
    iface->factory = factory_i;
    iface->initialize = initialize_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpDfpClusterThreadAwareLoadBalancer* self = RP_DFP_CLUSTER_THREAD_AWARE_LOAD_BALANCER(obj);
    g_clear_object(&self->m_factory);

    G_OBJECT_CLASS(rp_dfp_cluster_thread_aware_load_balancer_parent_class)->dispose(obj);
}

static void
rp_dfp_cluster_thread_aware_load_balancer_class_init(RpDfpClusterThreadAwareLoadBalancerClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_dfp_cluster_thread_aware_load_balancer_init(RpDfpClusterThreadAwareLoadBalancer* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpDfpClusterThreadAwareLoadBalancer*
rp_dfp_cluster_thread_aware_load_balancer_new(RpDfpCluster* cluster)
{
    LOGD("(%p)", cluster);
    g_return_val_if_fail(RP_IS_DFP_CLUSTER(cluster), NULL);
    RpDfpClusterThreadAwareLoadBalancer* self = g_object_new(RP_TYPE_DFP_CLUSTER_THREAD_AWARE_LOAD_BALANCER, NULL);
    self->m_factory = rp_dfp_cluster_load_balancer_factory_new(cluster);
    return self;
}
