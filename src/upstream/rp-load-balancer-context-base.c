/*
 * rp-load-balancer-context-base.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_load_balancer_context_base_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_load_balancer_context_base_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-load-balancer-context-base.h"

static void load_balancer_context_iface_init(RpLoadBalancerContextInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpLoadBalancerContextBase, rp_load_balancer_context_base, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_LOAD_BALANCER_CONTEXT, load_balancer_context_iface_init)
)

static RpNetworkConnection*
downstream_connection_i(RpLoadBalancerContext* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return NULL;
}

static RpStreamInfo*
request_stream_info_i(RpLoadBalancerContext* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return NULL;
}

static evhtp_headers_t*
downstream_headers_i(RpLoadBalancerContext* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return NULL;
}

static bool
should_select_another_host_i(RpLoadBalancerContext* self G_GNUC_UNUSED, RpHost* host G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p)", self, host);
    return false;
}

static guint32
host_selection_retry_count_i(RpLoadBalancerContext* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return 1;
}

static void
on_async_host_selection_i(RpLoadBalancerContext* self, RpHost* host, const char* details)
{
    NOISY_MSG_("(%p, %p, %p(%s))", self, host, details, details);
}

static void
load_balancer_context_iface_init(RpLoadBalancerContextInterface* iface)
{
    LOGD("(%p)", iface);
    iface->downstream_connection = downstream_connection_i;
    iface->request_stream_info = request_stream_info_i;
    iface->downstream_headers = downstream_headers_i;
    iface->should_select_another_host = should_select_another_host_i;
    iface->host_selection_retry_count = host_selection_retry_count_i;
    iface->on_async_host_selection = on_async_host_selection_i;
}

static void
rp_load_balancer_context_base_class_init(RpLoadBalancerContextBaseClass* klass G_GNUC_UNUSED)
{
    LOGD("(%p)", klass);

}

static void
rp_load_balancer_context_base_init(RpLoadBalancerContextBase* self G_GNUC_UNUSED)
{
    LOGD("(%p)", self);
}
