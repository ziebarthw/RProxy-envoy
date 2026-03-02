/*
 * rp-simple-thread-aware-load-balancer.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_simple_thread_aware_load_balancer_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_simple_thread_aware_load_balancer_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-upstream-impl.h"

struct _RpSimpleThreadAwareLoadBalancer {
    GObject parent_instance;

    RpLoadBalancerFactorySharedPtr m_factory;
};

static void thread_aware_load_balancer_iface_init(RpThreadAwareLoadBalancerInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpSimpleThreadAwareLoadBalancer, rp_simple_thread_aware_load_balancer, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_THREAD_AWARE_LOAD_BALANCER, thread_aware_load_balancer_iface_init)
)

static RpLoadBalancerFactorySharedPtr
factory_i(RpThreadAwareLoadBalancer* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_SIMPLE_THREAD_AWARE_LOAD_BALANCER(self)->m_factory;
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

    RpSimpleThreadAwareLoadBalancer* self = RP_SIMPLE_THREAD_AWARE_LOAD_BALANCER(obj);
    g_clear_object(&self->m_factory);

    G_OBJECT_CLASS(rp_simple_thread_aware_load_balancer_parent_class)->dispose(obj);
}

static void
rp_simple_thread_aware_load_balancer_class_init(RpSimpleThreadAwareLoadBalancerClass* klass)
{
    LOGD("(%p)", klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_simple_thread_aware_load_balancer_init(RpSimpleThreadAwareLoadBalancer* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpThreadAwareLoadBalancerPtr
rp_simple_thread_aware_load_balancer_new(RpLoadBalancerFactorySharedPtr factory)
{
    LOGD("(%p)", factory);
    g_return_val_if_fail(RP_IS_LOAD_BALANCER_FACTORY(factory), NULL);
    RpSimpleThreadAwareLoadBalancer* self = g_object_new(RP_TYPE_SIMPLE_THREAD_AWARE_LOAD_BALANCER, NULL);
    // This owns factory.p
    self->m_factory = factory;
    return RP_THREAD_AWARE_LOAD_BALANCER(self);
}
