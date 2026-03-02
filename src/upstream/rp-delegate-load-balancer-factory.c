/*
 * rp-delegate-load-balancer-factory.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_delegate_load_balancer_factory_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_delegate_load_balancer_factory_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-delegate-load-balancer-factory.h"

struct _RpDelegateLoadBalancerFactory {
    GObject parent_instance;

    GWeakRef m_parent_ref;
    RpLbCreateFn m_create_fn;
    bool m_recreate_on_host_change;
};

static void load_balancer_factory_iface_init(RpLoadBalancerFactoryInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpDelegateLoadBalancerFactory, rp_delegate_load_balancer_factory, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_LOAD_BALANCER_FACTORY, load_balancer_factory_iface_init)
)

static RpLoadBalancerPtr
create_i(RpLoadBalancerFactory* self, RpLoadBalancerParams* params)
{
    NOISY_MSG_("(%p(%s), %p)", self, G_OBJECT_TYPE_NAME(self), params);
    RpDelegateLoadBalancerFactory* me = RP_DELEGATE_LOAD_BALANCER_FACTORY(self);
    RpClusterSharedPtr parent = RP_CLUSTER(g_weak_ref_get(&me->m_parent_ref)); // Strong ref or null.
    if (!parent)
    {
        LOGE("parent is null");
        return NULL;
    }
    RpLoadBalancerPtr lb = me->m_create_fn(parent, params);
    g_object_unref(parent); // Drop the temporary strong ref.
    return lb;
}

static bool
recreate_on_host_change_i(RpLoadBalancerFactory* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_DELEGATE_LOAD_BALANCER_FACTORY(self)->m_recreate_on_host_change;
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

    RpDelegateLoadBalancerFactory* self = RP_DELEGATE_LOAD_BALANCER_FACTORY(obj);
    g_weak_ref_clear(&self->m_parent_ref);

    G_OBJECT_CLASS(rp_delegate_load_balancer_factory_parent_class)->dispose(obj);
}

static void
rp_delegate_load_balancer_factory_class_init(RpDelegateLoadBalancerFactoryClass* klass)
{
    LOGD("(%p)", klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_delegate_load_balancer_factory_init(RpDelegateLoadBalancerFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p(%s))", self, G_OBJECT_TYPE_NAME(self));
}

RpLoadBalancerFactory*
rp_delegate_load_balancer_factory_new(RpClusterSharedPtr parent, RpLbCreateFn create_fn, bool recreate_on_host_change)
{
    LOGD("(%p(%s), %p, %u)", parent, G_OBJECT_TYPE_NAME(parent), create_fn, recreate_on_host_change);

    g_return_val_if_fail(RP_IS_CLUSTER(parent), NULL);
    g_return_val_if_fail(create_fn != NULL, NULL);

    RpDelegateLoadBalancerFactory* self = g_object_new(RP_TYPE_DELEGATE_LOAD_BALANCER_FACTORY, NULL);
    g_weak_ref_init(&self->m_parent_ref, G_OBJECT(parent));
    self->m_create_fn = create_fn;
    self->m_recreate_on_host_change = recreate_on_host_change;
NOISY_MSG_("allocated load balancer factory %p", self);
    return RP_LOAD_BALANCER_FACTORY(self);
}
