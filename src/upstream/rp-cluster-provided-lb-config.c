/*
 * rp-cluster-provided-lb-config.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_provided_lb_config_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_provided_lb_config_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-cluster-provided-lb-factory.h"

struct _RpClusterProvidedLbConfig {
    GObject parent_instance;

};

G_DEFINE_TYPE_WITH_CODE(RpClusterProvidedLbConfig, rp_cluster_provided_lb_config, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_LOAD_BALANCER_CONFIG, NULL)
)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_cluster_provided_lb_config_parent_class)->dispose(obj);
}

static void
rp_cluster_provided_lb_config_class_init(RpClusterProvidedLbConfigClass* klass)
{
    LOGD("(%p)", klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_cluster_provided_lb_config_init(RpClusterProvidedLbConfig* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpLoadBalancerConfigPtr
rp_cluster_provided_lb_config_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_CLUSTER_PROVIDED_LB_CONFIG, NULL);
}
