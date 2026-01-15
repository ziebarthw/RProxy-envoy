/*
 * rp-static-load-balancer-factory.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_static_load_balancer_factory_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_static_load_balancer_factory_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "clusters/static/rp-static-cluster.h"

struct _RpStaticLoadBalancerFactory {
    GObject parent_instance;

    RpStaticClusterImpl* m_parent;
};

static void load_balancer_factory_iface_init(RpLoadBalancerFactoryInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpStaticLoadBalancerFactory, rp_static_load_balancer_factory, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_LOAD_BALANCER_FACTORY, load_balancer_factory_iface_init)
)

static RpLoadBalancerPtr
create_i(RpLoadBalancerFactory* self, RpLoadBalancerParams* params)
{
    NOISY_MSG_("(%p, %p)", self, params);
    return RP_LOAD_BALANCER(RP_STATIC_LOAD_BALANCER_FACTORY(self)->m_parent);
}

static bool
recreate_on_host_change_i(RpLoadBalancerFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return false;
}

static void
load_balancer_factory_iface_init(RpLoadBalancerFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create = create_i;
    iface->recreate_on_host_change = recreate_on_host_change_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_static_load_balancer_factory_parent_class)->dispose(obj);
}

static void
rp_static_load_balancer_factory_class_init(RpStaticLoadBalancerFactoryClass* klass)
{
    LOGD("(%p)", klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_static_load_balancer_factory_init(RpStaticLoadBalancerFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpStaticLoadBalancerFactory*
rp_static_load_balancer_factory_new(RpStaticClusterImpl* parent)
{
    LOGD("(%p)", parent);
    g_return_val_if_fail(parent != NULL, NULL);
    RpStaticLoadBalancerFactory* self = g_object_new(RP_TYPE_STATIC_LOAD_BALANCER_FACTORY, NULL);
    self->m_parent = parent;
    return self;
}
