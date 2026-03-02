/*
 * rp-cluster-provided-lb-factory.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_provided_lb_factory_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_provided_lb_factory_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-cluster-provided-lb-factory.h"

struct _RpClusterProvidedLbFactory {
    RpTypedLoadBalancerFactoryBase parent_instance;

};

static void typed_load_balancer_factory_iface_init(RpTypedLoadBalancerFactoryInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpClusterProvidedLbFactory, rp_cluster_provided_lb_factory, RP_TYPE_TYPED_LOAD_BALANCER_FACTORY_BASE,
    G_IMPLEMENT_INTERFACE(RP_TYPE_TYPED_LOAD_BALANCER_FACTORY, typed_load_balancer_factory_iface_init)
)

static RpThreadAwareLoadBalancerPtr
create_i(RpTypedLoadBalancerFactory* self, RpLoadBalancerConfig* lb_config, RpClusterInfoConstSharedPtr cluster_info, RpPrioritySet* priority_set, RpTimeSource* time_source)
{
    NOISY_MSG_("(%p, %p, %p, %p, %p)", self, lb_config, cluster_info, priority_set, time_source);
    // Cluster provided load balancer has empty implementation. Because it is a special case to
    // tell the cluster to use the load balancer provided by the cluster.
    return NULL;
}

static RpLoadBalancerConfig*
load_legacy_i(RpTypedLoadBalancerFactory* self G_GNUC_UNUSED, RpServerFactoryContext* factory_context G_GNUC_UNUSED, const RpClusterCfg* cluster G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p, %p)", self, factory_context, cluster);
    return rp_cluster_provided_lb_config_new();
}

static void
typed_load_balancer_factory_iface_init(RpTypedLoadBalancerFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create = create_i;
    iface->load_legacy = load_legacy_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_cluster_provided_lb_factory_parent_class)->dispose(obj);
}

static void
rp_cluster_provided_lb_factory_class_init(RpClusterProvidedLbFactoryClass* klass)
{
    LOGD("(%p)", klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_cluster_provided_lb_factory_init(RpClusterProvidedLbFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpTypedLoadBalancerFactory*
rp_cluster_provided_lb_factory_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_CLUSTER_PROVIDED_LB_FACTORY,
                        "name", "rproxy.load_balancing_policies.cluster_provided",
                        NULL);
}
