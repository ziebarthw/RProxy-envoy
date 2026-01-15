/*
 * rp-per-host-upstream.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_per_host_upstream_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_per_host_upstream_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-per-host-http-conn-pool.h"
#include "rp-per-host-tcp-conn-pool.h"
#include "rp-per-host-upstream.h"

struct _RpPerHostGenericConnPoolFactory {
    RpHttpConnPool parent_instance;

};

static void generic_conn_pool_factory_iface_init(RpGenericConnPoolFactoryInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpPerHostGenericConnPoolFactory, rp_per_host_generic_conn_pool_factory, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_GENERIC_CONN_POOL_FACTORY, generic_conn_pool_factory_iface_init)
)

static RpGenericConnPool*
create_generic_conn_pool_i(RpGenericConnPoolFactory* self, RpHostConstSharedPtr host, RpThreadLocalCluster* thread_local_cluster,
                            RpUpstreamProtocol_e upstream_protocol, RpResourcePriority_e priority, evhtp_proto downstream_protocol,
                            RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %p, %p, %d, %d, %d, %p)",
        self, host, thread_local_cluster, upstream_protocol, priority, downstream_protocol, context);
    switch (upstream_protocol)
    {
        case RpUpstreamProtocol_HTTP:
            return RP_GENERIC_CONN_POOL(
                rp_per_host_http_conn_pool_new(host, thread_local_cluster, priority, downstream_protocol, context));
        case RpUpstreamProtocol_TCP:
            return RP_GENERIC_CONN_POOL(
                rp_per_host_tcp_conn_pool_new(host, thread_local_cluster, priority, downstream_protocol, context));
        default:
            LOGE("oops!");
            break;
    }
    LOGE("http only");
    return NULL;
}

static void
generic_conn_pool_factory_iface_init(RpGenericConnPoolFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_generic_conn_pool = create_generic_conn_pool_i;
}

static void
rp_per_host_generic_conn_pool_factory_class_init(RpPerHostGenericConnPoolFactoryClass* klass G_GNUC_UNUSED)
{
    LOGD("(%p)", klass);
}

static void
rp_per_host_generic_conn_pool_factory_init(RpPerHostGenericConnPoolFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpPerHostGenericConnPoolFactory*
rp_per_host_generic_conn_pool_factory_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_PER_HOST_GENERIC_CONN_POOL_FACTORY, NULL);
}
