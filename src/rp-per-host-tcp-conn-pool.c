/*
 * rp-per-host-tcp-conn-pool.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_per_host_tcp_conn_pool_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_per_host_tcp_conn_pool_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-per-host-tcp-upstream.h"
#include "rp-per-host-tcp-conn-pool.h"

struct _RpPerHostTcpConnPool {
    RpTcpConnPool parent_instance;

};

// https://github.com/envoyproxy/envoy/blob/main/test/integration/upstreams/per_host_upstream_config.h
// class PerHostTcpConnPool : public Extensions::Upstreams::Http::Tcp::TcpConnPool

G_DEFINE_FINAL_TYPE(RpPerHostTcpConnPool, rp_per_host_tcp_conn_pool, RP_TYPE_TCP_CONN_POOL)

static void
on_pool_ready_i(RpTcpConnPoolCallbacks* self, RpTcpConnPoolConnectionData* conn_data, RpHostDescription* host)
{
    NOISY_MSG_("(%p, %p, %p)", self, conn_data, host);
    g_clear_object(rp_tcp_conn_pool_conn_pool_upstream_handle_(RP_TCP_CONN_POOL(self)));
    RpNetworkConnection* latched_conn = RP_NETWORK_CONNECTION(rp_tcp_conn_pool_connection_data_connection(conn_data));
    RpGenericConnectionPoolCallbacks* callbacks_ = rp_tcp_conn_pool_callbacks_(RP_TCP_CONN_POOL(self));
    RpUpstreamToDownstream* upstream_request = rp_generic_connection_pool_callbacks_upstream_to_downstream(callbacks_);
    RpPerHostTcpUpstream* upstream = rp_per_host_tcp_upstream_new(upstream_request, conn_data);
    rp_generic_connection_pool_callbacks_on_pool_ready(callbacks_,
                                                        RP_GENERIC_UPSTREAM(upstream),
                                                        host,
                                                        rp_network_connection_connection_info_provider(latched_conn),
                                                        rp_network_connection_stream_info(latched_conn),
                                                        EVHTP_PROTO_INVALID);
}

static void
on_pool_failure_i(RpTcpConnPoolCallbacks* self, RpPoolFailureReason_e reason, const char* failure_reason, RpHostDescription* host_description)
{
    NOISY_MSG_("(%p, %d, %p(%s), %p)",
        self, reason, failure_reason, failure_reason, host_description);
    RpGenericConnectionPoolCallbacks* callbacks_ = rp_tcp_conn_pool_callbacks_(RP_TCP_CONN_POOL(self));
    rp_generic_connection_pool_callbacks_on_pool_failure(callbacks_, reason, failure_reason, host_description);
}

static void
tcp_conn_pool_callbacks_iface_init(RpTcpConnPoolCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_pool_ready = on_pool_ready_i;
    iface->on_pool_failure = on_pool_failure_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_per_host_tcp_conn_pool_parent_class)->dispose(obj);
}

static void
tcp_conn_pool_class_init(RpTcpConnPoolClass* klass)
{
    LOGD("(%p)", klass);
    tcp_conn_pool_callbacks_iface_init(g_type_interface_peek(klass, RP_TYPE_TCP_CONN_POOL_CALLBACKS));
}

static void
rp_per_host_tcp_conn_pool_class_init(RpPerHostTcpConnPoolClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    tcp_conn_pool_class_init(RP_TCP_CONN_POOL_CLASS(klass));
}

static void
rp_per_host_tcp_conn_pool_init(RpPerHostTcpConnPool* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpPerHostTcpConnPool*
rp_per_host_tcp_conn_pool_new(RpHost* host, RpThreadLocalCluster* thread_local_cluster,
                                RpResourcePriority_e priority, evhtp_proto downstream_protocol, RpLoadBalancerContext* context)
{
    LOGD("(%p, %p, %d, %d, %p)", host, thread_local_cluster, priority, downstream_protocol, context);
    g_return_val_if_fail(RP_IS_HOST(host), NULL);
    g_return_val_if_fail(RP_IS_THREAD_LOCAL_CLUSTER(thread_local_cluster), NULL);
    return g_object_new(RP_TYPE_PER_HOST_TCP_CONN_POOL,
                        "host", host,
                        "thread-local-cluster", thread_local_cluster,
                        "priority", priority,
                        "downstream-protocol", downstream_protocol,
                        "context", context,
                        NULL);
}
