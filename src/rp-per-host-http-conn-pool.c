/*
 * rp-per-host-http-conn-pool.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_per_host_http_conn_pool_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_per_host_http_conn_pool_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-per-host-http-upstream.h"
#include "rp-per-host-http-conn-pool.h"

struct _RpPerHostHttpConnPool {
    RpHttpConnPool parent_instance;

};

// https://github.com/envoyproxy/envoy/blob/main/test/integration/upstreams/per_host_upstream_config.h
// class PerHostHttpConnPool : public Extensions::Upstreams::Http::Http::HttpConnPool

G_DEFINE_FINAL_TYPE(RpPerHostHttpConnPool, rp_per_host_http_conn_pool, RP_TYPE_HTTP_CONN_POOL)

static void
on_pool_ready_i(RpHttpConnPoolCallbacks* self, RpRequestEncoder* callbacks_encoder, RpHostDescription* host, RpStreamInfo* info, evhtp_proto protocol)
{
    NOISY_MSG_("(%p, %p, %p, %p, %d)", self, callbacks_encoder, host, info, protocol);
g_clear_object(rp_http_conn_pool_conn_pool_stream_handle_(RP_HTTP_CONN_POOL(self)));
//*rp_http_conn_pool_conn_pool_stream_handle_(RP_HTTP_CONN_POOL(self)) = NULL;
    RpGenericConnectionPoolCallbacks* callbacks_ = rp_http_conn_pool_callbacks_(RP_HTTP_CONN_POOL(self));
    RpUpstreamToDownstream* upstream_request = rp_generic_connection_pool_callbacks_upstream_to_downstream(callbacks_);
    RpPerHostHttpUpstream* upstream = rp_per_host_http_upstream_new(upstream_request, callbacks_encoder, host);

    RpConnectionInfoProvider* provider = rp_stream_connection_info_provider(
        rp_stream_encoder_get_stream(RP_STREAM_ENCODER(callbacks_encoder)));
    rp_generic_connection_pool_callbacks_on_pool_ready(callbacks_, RP_GENERIC_UPSTREAM(upstream), host, provider, info, protocol);
}

static void
on_pool_failure_i(RpHttpConnPoolCallbacks* self, RpPoolFailureReason_e reason, const char* failure_reason, RpHostDescription* host_description)
{
    NOISY_MSG_("(%p, %d, %p(%s), %p)",
        self, reason, failure_reason, failure_reason, host_description);
    RpGenericConnectionPoolCallbacks* callbacks_ = rp_http_conn_pool_callbacks_(RP_HTTP_CONN_POOL(self));
    rp_generic_connection_pool_callbacks_on_pool_failure(callbacks_, reason, failure_reason, host_description);
}

static void
http_conn_pool_callbacks_iface_init(RpHttpConnPoolCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_pool_ready = on_pool_ready_i;
    iface->on_pool_failure = on_pool_failure_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_per_host_http_conn_pool_parent_class)->dispose(obj);
}

static void
http_conn_pool_class_init(RpHttpConnPoolClass* klass)
{
    LOGD("(%p)", klass);
    http_conn_pool_callbacks_iface_init(g_type_interface_peek(klass, RP_TYPE_HTTP_CONN_POOL_CALLBACKS));
}

static void
rp_per_host_http_conn_pool_class_init(RpPerHostHttpConnPoolClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    http_conn_pool_class_init(RP_HTTP_CONN_POOL_CLASS(klass));
}

static void
rp_per_host_http_conn_pool_init(RpPerHostHttpConnPool* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpPerHostHttpConnPool*
rp_per_host_http_conn_pool_new(RpHost* host, RpThreadLocalCluster* thread_local_cluster,
                                RpResourcePriority_e priority, evhtp_proto downstream_protocol, RpLoadBalancerContext* context)
{
    LOGD("(%p, %p, %d, %d, %p)", host, thread_local_cluster, priority, downstream_protocol, context);
    g_return_val_if_fail(RP_IS_HOST(host), NULL);
    g_return_val_if_fail(RP_IS_THREAD_LOCAL_CLUSTER(thread_local_cluster), NULL);
    return g_object_new(RP_TYPE_PER_HOST_HTTP_CONN_POOL,
                        "host", host,
                        "thread-local-cluster", thread_local_cluster,
                        "priority", priority,
                        "downstream-protocol", downstream_protocol,
                        "context", context,
                        NULL);
}
