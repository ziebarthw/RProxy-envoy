/*
 * rp-cluster-initialization-object.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_initialization_object_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_initialization_object_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-cluster-manager-impl.h"

struct _RpClusterInitializationObject {
    GObject parent_instance;

    RpClusterInfoConstSharedPtr m_cluster_info;
    RpLoadBalancerFactorySharedPtr m_load_balancer_factory;
    RpHostMapConstSharedPtr m_cross_priority_host_map;
    //TODO...
};

G_DEFINE_TYPE(RpClusterInitializationObject, rp_cluster_initialization_object, G_TYPE_OBJECT)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_cluster_initialization_object_parent_class)->dispose(obj);
}

static void
rp_cluster_initialization_object_class_init(RpClusterInitializationObjectClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_cluster_initialization_object_init(RpClusterInitializationObject* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpClusterInitializationObject*
rp_cluster_initialization_object_new(const RpThreadLocalClusterUpdateParams* params, RpClusterInfoConstSharedPtr cluster_info,
                                        RpLoadBalancerFactorySharedPtr load_balancer_factory, RpHostMapConstSharedPtr map/*, TODO...*/)
{
    LOGD("(%p, %p, %p, %p)", params, cluster_info, load_balancer_factory, map);
    RpClusterInitializationObject* self = g_object_new(RP_TYPE_CLUSTER_INITIALIZATION_OBJECT, NULL);
    self->m_cluster_info = cluster_info;
    self->m_load_balancer_factory = load_balancer_factory;
    self->m_cross_priority_host_map = map;
    //TODO...Copy the update since the map is empty....
    return self;
}
