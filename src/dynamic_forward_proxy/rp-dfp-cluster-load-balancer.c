/*
 * rp-dfp-cluster-load-balancer.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_dfp_cluster_load_balancer_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_dfp_cluster_load_balancer_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-headers.h"
#include "rp-header-utility.h"
#include "rp-state-filter.h"
#include "dynamic_forward_proxy/rp-cluster.h"

struct _RpDfpClusterLoadBalancer {
    GObject parent_instance;

    RpDfpCluster* m_cluster;
};

static void load_balancer_iface_init(RpLoadBalancerInterface* iface);
static void dfp_lb_iface_init(RpDfpLbInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpDfpClusterLoadBalancer, rp_dfp_cluster_load_balancer, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_DFP_LB, dfp_lb_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_LOAD_BALANCER, load_balancer_iface_init)
)

static inline RpHostSelectionResponse
null_host_selection_response(void)
{
    NOISY_MSG_("()");
    return rp_host_selection_response_ctor(NULL, NULL, NULL);
}

static RpHostSelectionResponse
choose_host_i(RpLoadBalancer* self, RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %p)", self, context);

    if (!context)
    {
        NOISY_MSG_("context is null");
        return null_host_selection_response();
    }

    guint32 default_port = /*TODO...is_secure ? ...*/80;

    RpStreamInfo* stream_info = rp_load_balancer_context_request_stream_info(context);
    RpFilterState* filter_state = stream_info ? rp_stream_info_filter_state(stream_info) : NULL;

    const char* raw_host = NULL;
    guint32 port = default_port;
    if (filter_state)
    {
        NOISY_MSG_("getting host from filter state");
        raw_host = rp_filter_state_get_data(filter_state, dynamic_host_key);

        gpointer dynamic_port_filter_state = rp_filter_state_get_data(filter_state, dynamic_port_key);
        if (dynamic_port_filter_state)
        {
            port = *((guint32*)dynamic_port_filter_state);
        }
    }
    else if (rp_load_balancer_context_downstream_headers(context))
    {
        NOISY_MSG_("getting host from downstream headers");
        raw_host = evhtp_header_find(rp_load_balancer_context_downstream_headers(context), RpHeaderValues.HostLegacy);
    }
    else if (rp_load_balancer_context_downstream_connection(context))
    {
        NOISY_MSG_("getting host from downstream connection");
        raw_host = rp_connection_socket_requested_server_name(RP_CONNECTION_SOCKET(rp_load_balancer_context_downstream_connection(context)));
    }

    if (!raw_host || !raw_host[0])
    {
        LOGD("host empty");
        return null_host_selection_response();
    }

    g_autofree char* hostname = normalize_host_for_dfp(raw_host, port);

    RpDfpClusterLoadBalancer* me = RP_DFP_CLUSTER_LOAD_BALANCER(self);
    if (rp_dfp_cluster_enable_sub_cluster(me->m_cluster))
    {
        NOISY_MSG_("calling rp_dfp_cluster_choose_host(%p, %p(%s), %p)", self, hostname, hostname, context);
        return rp_dfp_cluster_impl_choose_host(RP_DFP_CLUSTER_IMPL(me->m_cluster), hostname, context);
    }

    RpHostConstSharedPtr host = rp_dfp_lb_find_host_by_name(RP_DFP_LB(self), hostname);
    //TODO...
    return rp_host_selection_response_ctor(host, NULL, NULL);
}

static void
load_balancer_iface_init(RpLoadBalancerInterface* iface)
{
    LOGD("(%p)", iface);
    iface->choose_host = choose_host_i;
}

static RpHostConstSharedPtr
find_host_by_name_i(RpDfpLb* self, const char* host)
{
    NOISY_MSG_("(%p, %p(%s))", self, host, host);
    RpDfpClusterLoadBalancer* me = RP_DFP_CLUSTER_LOAD_BALANCER(self);
    return rp_dfp_cluster_impl_find_host_by_name(RP_DFP_CLUSTER_IMPL(me->m_cluster), host);
}

static void
dfp_lb_iface_init(RpDfpLbInterface* iface)
{
    LOGD("(%p)", iface);
    iface->find_host_by_name = find_host_by_name_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_dfp_cluster_load_balancer_parent_class)->dispose(obj);
}

static void
rp_dfp_cluster_load_balancer_class_init(RpDfpClusterLoadBalancerClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_dfp_cluster_load_balancer_init(RpDfpClusterLoadBalancer* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpDfpClusterLoadBalancer*
rp_dfp_cluster_load_balancer_new(RpDfpCluster* cluster)
{
    LOGD("(%p)", cluster);
    g_return_val_if_fail(RP_IS_DFP_CLUSTER(cluster), NULL);
    RpDfpClusterLoadBalancer* self = g_object_new(RP_TYPE_DFP_CLUSTER_LOAD_BALANCER, NULL);
    self->m_cluster = cluster;
    return self;
}
