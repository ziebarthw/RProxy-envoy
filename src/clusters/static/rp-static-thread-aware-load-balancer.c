/*
 * rp-static-thread-aware-load-balancer.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_static_thread_aware_load_balancer_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_static_thread_aware_load_balancer_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "clusters/static/rp-static-cluster.h"

struct _RpStaticThreadAwareLoadBalancer {
    GObject parent_instance;

    SHARED_PTR(RpStaticLoadBalancerFactory) m_factory;
};

static void thread_aware_load_balancer_iface_init(RpThreadAwareLoadBalancerInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpStaticThreadAwareLoadBalancer, rp_static_thread_aware_load_balancer, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_THREAD_AWARE_LOAD_BALANCER, thread_aware_load_balancer_iface_init)
)

static RpLoadBalancerFactorySharedPtr
factory_i(RpThreadAwareLoadBalancer* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_LOAD_BALANCER_FACTORY(RP_STATIC_THREAD_AWARE_LOAD_BALANCER(self)->m_factory);
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

    RpStaticThreadAwareLoadBalancer* self = RP_STATIC_THREAD_AWARE_LOAD_BALANCER(obj);
    g_clear_object(&self->m_factory);

    G_OBJECT_CLASS(rp_static_thread_aware_load_balancer_parent_class)->dispose(obj);
}

static void
rp_static_thread_aware_load_balancer_class_init(RpStaticThreadAwareLoadBalancerClass* klass)
{
    LOGD("(%p)", klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_static_thread_aware_load_balancer_init(RpStaticThreadAwareLoadBalancer* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpStaticThreadAwareLoadBalancer*
rp_static_thread_aware_load_balancer_new(RpStaticClusterImpl* parent)
{
    LOGD("(%p)", parent);
    g_return_val_if_fail(parent != NULL, NULL);
    RpStaticThreadAwareLoadBalancer* self = g_object_new(RP_TYPE_STATIC_THREAD_AWARE_LOAD_BALANCER, NULL);
    self->m_factory = rp_static_load_balancer_factory_new(parent);
    return self;
}
