/*
 * rp-thread-local-cluster.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-http-conn-pool.h"
#include "rp-http-pool-data.h"
#include "rp-tcp-conn-pool.h"
#include "rp-tcp-pool-data.h"
#include "rp-load-balancer.h"
#include "rp-upstream.h"

G_BEGIN_DECLS

/**
 * A thread local cluster instance that can be used for direct load balancing and host set
 * interactions. In general, an instance of ThreadLocalCluster can only be safely used in the
 * direct call context after it is retrieved from the cluster manager. See ClusterManager::get()
 * for more information.
 */
#define RP_TYPE_THREAD_LOCAL_CLUSTER rp_thread_local_cluster_get_type()
G_DECLARE_INTERFACE(RpThreadLocalCluster, rp_thread_local_cluster, RP, THREAD_LOCAL_CLUSTER, GObject)

struct _RpThreadLocalClusterInterface {
    GTypeInterface parent_iface;

    RpClusterInfo* (*info)(RpThreadLocalCluster*);
    RpHostSelectionResponse (*choose_host)(RpThreadLocalCluster*,
                                            RpLoadBalancerContext*);
    RpHttpPoolData* (*http_conn_pool)(RpThreadLocalCluster*,
                                        RpHost*,
                                        RpResourcePriority_e,
                                        evhtp_proto,
                                        RpLoadBalancerContext*);
    RpTcpPoolData* (*tcp_conn_pool)(RpThreadLocalCluster*,
                                    RpHost*,
                                    RpResourcePriority_e,
                                    RpLoadBalancerContext*);
};

static inline RpClusterInfo*
rp_thread_local_cluster_info(RpThreadLocalCluster* self)
{
    return RP_IS_THREAD_LOCAL_CLUSTER(self) ?
        RP_THREAD_LOCAL_CLUSTER_GET_IFACE(self)->info(self) : NULL;
}
static inline RpHostSelectionResponse
rp_thread_local_cluster_choose_host(RpThreadLocalCluster* self, RpLoadBalancerContext* context)
{
    return RP_IS_THREAD_LOCAL_CLUSTER(self) ?
        RP_THREAD_LOCAL_CLUSTER_GET_IFACE(self)->choose_host(self, context) :
        rp_host_selection_response_ctor(NULL, NULL, NULL);
}
static inline RpHttpPoolData*
rp_thread_local_cluster_http_conn_pool(RpThreadLocalCluster* self, RpHost* host, RpResourcePriority_e priority,
                                        evhtp_proto downstream_protocol, RpLoadBalancerContext* context)
{
    return RP_IS_THREAD_LOCAL_CLUSTER(self) ?
        RP_THREAD_LOCAL_CLUSTER_GET_IFACE(self)->http_conn_pool(self, host, priority, downstream_protocol, context) :
        NULL;
}
static inline RpTcpPoolData*
rp_thread_local_cluster_tcp_conn_pool(RpThreadLocalCluster* self, RpHost* host, RpResourcePriority_e priority, RpLoadBalancerContext* context)
{
    return RP_IS_THREAD_LOCAL_CLUSTER(self) ?
        RP_THREAD_LOCAL_CLUSTER_GET_IFACE(self)->tcp_conn_pool(self, host, priority, context) :
        NULL;
}

G_END_DECLS
