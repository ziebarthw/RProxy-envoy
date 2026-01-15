/*
 * rp-cluster-data.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_data_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_data_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-time.h"
#include "upstream/rp-cluster-manager-impl.h"

struct _RpClusterData {
    GObject parent_instance;

    RpClusterCfg* m_cluster_config;
    SHARED_PTR(RpCluster) m_cluster;
    RpThreadAwareLoadBalancerPtr m_thread_aware_lb;
    RpSystemTime m_last_updated;

    bool m_added_or_updated : 1;
    bool m_required_for_ads : 1;
};

static void cluster_manager_cluster_iface_init(RpClusterManagerClusterInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpClusterData, rp_cluster_data, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_CLUSTER_MANAGER_CLUSTER, cluster_manager_cluster_iface_init)
)

static RpCluster*
cluster_i(RpClusterManagerCluster* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_DATA(self)->m_cluster;
}

static RpLoadBalancerFactorySharedPtr
load_balancer_factory_i(RpClusterManagerCluster* self)
{
    NOISY_MSG_("(%p)", self);
    RpThreadAwareLoadBalancerPtr thread_aware_lb_ = RP_CLUSTER_DATA(self)->m_thread_aware_lb;
    if (thread_aware_lb_)
    {
        return rp_thread_aware_load_balancer_factory(thread_aware_lb_);
    }
    return NULL;
}

static bool
added_or_updated_i(RpClusterManagerCluster* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_DATA(self)->m_added_or_updated;
}

static void
set_added_or_updated_i(RpClusterManagerCluster* self)
{
    NOISY_MSG_("(%p)", self);
    g_assert(!RP_CLUSTER_DATA(self)->m_added_or_updated);
    RP_CLUSTER_DATA(self)->m_added_or_updated = true;
}

static bool
required_for_ads_i(RpClusterManagerCluster* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_DATA(self)->m_required_for_ads;
}

static void
cluster_manager_cluster_iface_init(RpClusterManagerClusterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->cluster = cluster_i;
    iface->load_balancer_factory = load_balancer_factory_i;
    iface->added_or_updated = added_or_updated_i;
    iface->set_added_or_updated = set_added_or_updated_i;
    iface->required_for_ads = required_for_ads_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpClusterData* self = RP_CLUSTER_DATA(obj);
    g_clear_object(&self->m_cluster);

    G_OBJECT_CLASS(rp_cluster_data_parent_class)->dispose(obj);
}

static void
rp_cluster_data_class_init(RpClusterDataClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_cluster_data_init(RpClusterData* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpClusterData*
rp_cluster_data_new(RpClusterCfg* cluster_config, bool added_via_api, SHARED_PTR(RpCluster) cluster, RpTimeSource* time_source)
{
    LOGD("(%p, %u, %p, %p)", cluster_config, added_via_api, cluster, time_source);

    g_return_val_if_fail(cluster_config != NULL, NULL);
    g_return_val_if_fail(RP_IS_CLUSTER(cluster), NULL);
    g_return_val_if_fail(RP_IS_TIME_SOURCE(time_source), NULL);

    RpClusterData* self = g_object_new(RP_TYPE_CLUSTER_DATA, NULL);
    self->m_cluster_config = cluster_config;
    self->m_cluster = g_object_ref(cluster); // Keep a reference to the cluster.
    self->m_last_updated = rp_time_source_system_time(time_source);
    return self;
}

RpThreadAwareLoadBalancerPtr*
rp_cluster_data_thread_aware_lb_(RpClusterData* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CLUSTER_DATA(self), NULL);
    return &self->m_thread_aware_lb;
}
