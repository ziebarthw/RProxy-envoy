/*
 * rp-load-balancer.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_load_balancer_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_load_balancer_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-load-balancer.h"

G_DEFINE_INTERFACE(RpAsyncHostSelectionHandle, rp_async_host_selection_handle, G_TYPE_OBJECT)
static void
rp_async_host_selection_handle_default_init(RpAsyncHostSelectionHandleInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpLoadBalancerContext, rp_load_balancer_context, G_TYPE_OBJECT)
static void
rp_load_balancer_context_default_init(RpLoadBalancerContextInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpLoadBalancer, rp_load_balancer, G_TYPE_OBJECT)
static void
rp_load_balancer_default_init(RpLoadBalancerInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpLoadBalancerFactory, rp_load_balancer_factory, G_TYPE_OBJECT)
static void
rp_load_balancer_factory_default_init(RpLoadBalancerFactoryInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpThreadAwareLoadBalancer, rp_thread_aware_load_balancer, G_TYPE_OBJECT)
static void
rp_thread_aware_load_balancer_default_init(RpThreadAwareLoadBalancerInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
