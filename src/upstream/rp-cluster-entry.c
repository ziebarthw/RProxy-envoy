/*
 * rp-cluster-entry.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_entry_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_entry_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rproxy.h"
#include "rp-host-set-ptr-vector.h"
#include "upstream/rp-upstream-impl.h"
#include "upstream/rp-priority-conn-pool-map.h"
#include "upstream/rp-cluster-manager-impl.h"

struct _RpClusterEntry {
    GObject parent_instance;

    RpThreadLocalClusterManagerImpl* m_parent;
    RpPrioritySetImpl* m_priority_set;
    RpClusterInfoSharedPtr m_cluster_info;
    RpLoadBalancerFactorySharedPtr m_lb_factory;
    RpLoadBalancerPtr m_lb;
};

static void thread_local_cluster_iface_init(RpThreadLocalClusterInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpClusterEntry, rp_cluster_entry, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_THREAD_LOCAL_CLUSTER, thread_local_cluster_iface_init)
)

static bool
allow_lb_choose_host(RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p)", context);

    if (!context)
    {
        NOISY_MSG_("context is null");
        return true;
    }

    RpOverrideHost override_host = rp_load_balancer_context_override_host_to_select(context);
    if (!override_host.first)
    {
        NOISY_MSG_("override host is null");
        return true;
    }

    return !override_host.second;
}

static RpHostSelectionResponse
choose_host_i(RpThreadLocalCluster* self, RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %p)", self, context);

    //TODO...

    if (allow_lb_choose_host(context))
    {
        RpClusterEntry* me = RP_CLUSTER_ENTRY(self);
        RpHostSelectionResponse host_selection = rp_load_balancer_choose_host(me->m_lb, context);
        if (host_selection.m_host || host_selection.m_cancelable)
        {
            NOISY_MSG_("selected host %p", host_selection.m_host);
            return host_selection;
        }
        //TODO...cluster_info_->trafficStats()->upstream_cx_non_healthy_.inc();
        LOGD("no healthy host");
        return host_selection;
    }

    //TODO...cluster_info_->trafficStats()->upstream_cx_non_healthy_.inc();
    LOGD("no healthy host");
    return rp_host_selection_response_ctor(NULL, NULL, NULL);
}

static RpClusterInfoSharedPtr
info_i(RpThreadLocalCluster* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_ENTRY(self)->m_cluster_info;
}

static RpLoadBalancer*
load_balancer_i(RpThreadLocalCluster* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_ENTRY(self)->m_lb;
}


typedef struct _RpConnectionPoolFactoryCtx RpConnectionPoolFactoryCtx;
struct _RpConnectionPoolFactoryCtx {
    RpClusterEntry* m_cluster_entry;
    RpHost* m_host;
    RpResourcePriority_e m_priority;
    evhtp_proto m_downstream_protocol;
    RpLoadBalancerContext* m_context;
};

static RpHttpConnectionPoolInstancePtr
conn_pool_factory_cb(gpointer user_data)
{
    NOISY_MSG_("(%p)", user_data);
    RpConnectionPoolFactoryCtx* ctx = (RpConnectionPoolFactoryCtx*)user_data;
    RpClusterEntry* self = ctx->m_cluster_entry;
    RpClusterManagerImpl* parent_ = rp_thread_local_cluster_manager_impl_parent_(self->m_parent);
    RpClusterManagerFactory* factory_ = rp_cluster_manager_impl_factory_(parent_);
    return rp_cluster_manager_factory_allocate_conn_pool(factory_,
                                                            rp_thread_local_cluster_manager_impl_dispatcher_(self->m_parent),
                                                            ctx->m_host,
                                                            ctx->m_priority,
                                                            &ctx->m_downstream_protocol);
}

static RpHttpConnectionPoolInstancePtr
http_conn_pool_impl(RpClusterEntry* self, RpHost* host, RpResourcePriority_e priority, evhtp_proto downstream_protocol, RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %p, %d, %d, %p)", self, host, priority, downstream_protocol, context);

    if (!host)
    {
        NOISY_MSG_("host is null");
        return NULL;
    }

    evhtp_proto* upstream_protocols = rp_cluster_info_upstream_http_protocol(
        rp_host_description_cluster((RpHostDescriptionConstSharedPtr)host), downstream_protocol);
    //TODO...

    // For now, just use the upstream protocol...
    g_autoptr(GBytes) key = rp_conn_pool_key_new(upstream_protocols[0], NULL, 0, "", NULL);

    g_free(upstream_protocols);

    RpConnectionPoolFactoryCtx ctx = {
        .m_cluster_entry = self,
        .m_host = host,
        .m_priority = priority,
        .m_downstream_protocol = downstream_protocol
    };
    RpConnPoolsContainer* container = rp_thread_local_cluster_manager_impl_get_http_conn_pools_container(self->m_parent, host, true);
    return rp_priority_conn_pool_map_get_pool(container->m_pools,
                                                priority,
                                                key,
                                                conn_pool_factory_cb,
                                                &ctx);
}

static RpTcpConnPoolInstance*
tcp_conn_pool_impl(RpClusterEntry* self, RpHost* host, RpResourcePriority_e priority, RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %p, %d, %p)", self, host, priority, context);

    RpClusterEntry* me = RP_CLUSTER_ENTRY(self);
    GHashTable* map = rp_thread_local_cluster_manager_impl_host_tcp_conn_pool_map_(me->m_parent);
    RpTcpConnPoolsContainer* container = g_hash_table_lookup(map, host);
    if (!container)
    {
        NOISY_MSG_("creating container");
        container = rp_tcp_conn_pools_container_new(host);
        g_hash_table_insert(map, (gpointer)host, container);
    }
    g_autoptr(GBytes) key = rp_conn_pool_key_new(priority, NULL, 0, "", NULL);
    RpTcpConnPoolInstancePtr pool = g_hash_table_lookup(container->m_pools, key);
    if (!pool)
    {
        RpDispatcher* dispatcher = rp_thread_local_cluster_manager_impl_dispatcher_(me->m_parent);
        RpClusterManagerImpl* parent_ = rp_thread_local_cluster_manager_impl_parent_(me->m_parent);
        RpClusterManagerFactory* factory = rp_cluster_manager_impl_factory_(parent_);
        pool = rp_cluster_manager_factory_allocate_tcp_conn_pool(factory, dispatcher, host, priority);
        g_hash_table_insert(container->m_pools, key, pool);
    }
    return pool;
}

static void
maybe_pre_connect(RpClusterEntry* self)
{
    NOISY_MSG_("(%p)", self);
    //TODO...
}

typedef struct _RpHttpPreConnectCbCtx RpHttpPreConnectCbCtx;
struct _RpHttpPreConnectCbCtx {
    RpClusterEntry* m_cluster_entry;
    RpResourcePriority_e m_priority;
    evhtp_proto m_protocol;
    RpLoadBalancerContext* m_context;
};
static inline RpHttpPreConnectCbCtx
rp_http_pre_connect_cb_ctx_ctor(RpClusterEntry* cluster_entry, RpResourcePriority_e priority, evhtp_proto protocol, RpLoadBalancerContext* context)
{
    RpHttpPreConnectCbCtx self = {
        .m_cluster_entry = cluster_entry,
        .m_priority = priority,
        .m_protocol = protocol,
        .m_context = context
    };
    return self;
}
static inline RpHttpPreConnectCbCtx*
rp_http_pre_connect_cb_ctx_new(RpClusterEntry* cluster_entry, RpResourcePriority_e priority, evhtp_proto protocol, RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %d, %d, %p)", cluster_entry, priority, protocol, context);
    RpHttpPreConnectCbCtx* self = g_new(RpHttpPreConnectCbCtx, 1);
    *self = rp_http_pre_connect_cb_ctx_ctor(cluster_entry, priority, protocol, context);
    return self;
}
static inline void
rp_http_pre_connect_cb_ctx_free(RpHttpPreConnectCbCtx* self)
{
    NOISY_MSG_("(%p)", self);
    g_free(self);
}
static inline RpHttpPreConnectCbCtx
rp_http_pre_connect_cb_ctx_captures(RpHttpPreConnectCbCtx* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttpPreConnectCbCtx captures = rp_http_pre_connect_cb_ctx_ctor(self->m_cluster_entry,
                                                                        self->m_priority,
                                                                        self->m_protocol,
                                                                        self->m_context);
    rp_http_pre_connect_cb_ctx_free(g_steal_pointer(&self));
    return captures;
}

static void
http_pre_connect_cb(RpHttpConnectionPoolInstancePtr pool, gpointer user_data)
{
    NOISY_MSG_("(%p, %p)", pool, user_data);
//    rp_connection_pool_instance_maybe_preconnect(RP_CONNECTION_POOL_INSTANCE(pool), 1.0);
    RpHttpPreConnectCbCtx* ctx = user_data;
    RpHttpPreConnectCbCtx captures = rp_http_pre_connect_cb_ctx_captures(g_steal_pointer(&ctx));
    maybe_pre_connect(captures.m_cluster_entry);
}

static RpHttpPoolData*
http_conn_pool_i(RpThreadLocalCluster* self, RpHost* host, RpResourcePriority_e priority, evhtp_proto downstream_protocol, RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %p, %d, %d, %p)", self, host, priority, downstream_protocol, context);
    RpClusterEntry* me = RP_CLUSTER_ENTRY(self);
    RpHttpConnectionPoolInstancePtr pool = http_conn_pool_impl(me,
                                                                host,
                                                                priority,
                                                                downstream_protocol,
                                                                context);
    if (!pool)
    {
        NOISY_MSG_("pool is null");
        return NULL;
    }

    RpHttpPreConnectCbCtx* ctx = rp_http_pre_connect_cb_ctx_new(me, priority, downstream_protocol, context);
    return rp_http_pool_data_new(http_pre_connect_cb, ctx, pool);
}

typedef struct _RpTcpPreConnectCbCtx RpTcpPreConnectCbCtx;
struct _RpTcpPreConnectCbCtx {
    RpClusterEntry* m_cluster_entry;
    RpResourcePriority_e m_priority;
    RpLoadBalancerContext* m_context;
};
static inline RpTcpPreConnectCbCtx
rp_tcp_pre_connect_cb_ctx_ctor(RpClusterEntry* cluster_entry, RpResourcePriority_e priority, RpLoadBalancerContext* context)
{
    RpTcpPreConnectCbCtx self = {
        .m_cluster_entry = cluster_entry,
        .m_priority = priority,
        .m_context = context
    };
    return self;
}
static inline RpTcpPreConnectCbCtx*
rp_tcp_pre_connect_cb_ctx_new(RpClusterEntry* cluster_entry, RpResourcePriority_e priority, RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %d, %p)", cluster_entry, priority, context);
    RpTcpPreConnectCbCtx* self = g_new(RpTcpPreConnectCbCtx, 1);
    *self = rp_tcp_pre_connect_cb_ctx_ctor(cluster_entry, priority, context);
    return self;
}
static inline void
rp_tcp_pre_connect_cb_ctx_free(RpTcpPreConnectCbCtx* self)
{
    NOISY_MSG_("(%p)", self);
    g_free(self);
}
static inline RpTcpPreConnectCbCtx
rp_tcp_pre_connect_cb_ctx_captures(RpTcpPreConnectCbCtx* self)
{
    NOISY_MSG_("(%p)", self);
    RpTcpPreConnectCbCtx captures = rp_tcp_pre_connect_cb_ctx_ctor(self->m_cluster_entry,
                                                                    self->m_priority,
                                                                    self->m_context);
    rp_tcp_pre_connect_cb_ctx_free(self);
    return captures;
}

static void
tcp_pre_connect_cb(RpTcpConnPoolInstancePtr pool, gpointer user_data)
{
    NOISY_MSG_("(%p, %p)", pool, user_data);
//    rp_connection_pool_instance_maybe_preconnect(RP_CONNECTION_POOL_INSTANCE(pool), 1.0);
    RpTcpPreConnectCbCtx* ctx = user_data;
    RpTcpPreConnectCbCtx captures = rp_tcp_pre_connect_cb_ctx_captures(ctx);
    maybe_pre_connect(captures.m_cluster_entry);
}

static RpTcpPoolData*
tcp_conn_pool_i(RpThreadLocalCluster* self, RpHost* host, RpResourcePriority_e priority, RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %p, %d, %p)", self, host, priority, context);
    if (!host)
    {
        NOISY_MSG_("host is null");
        return NULL;
    }
    RpClusterEntry* me = RP_CLUSTER_ENTRY(self);
    RpTcpConnPoolInstance* pool = tcp_conn_pool_impl(me, host, priority, context);
    if (!pool)
    {
        NOISY_MSG_("pool is null");
        return NULL;
    }

    RpTcpPreConnectCbCtx* ctx = rp_tcp_pre_connect_cb_ctx_new(me, priority, context);
    return rp_tcp_pool_data_new(tcp_pre_connect_cb, ctx, pool);
}

static RpPrioritySet*
priority_set_i(RpThreadLocalCluster* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_PRIORITY_SET(RP_CLUSTER_ENTRY(self)->m_priority_set);
}

static void
thread_local_cluster_iface_init(RpThreadLocalClusterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->choose_host = choose_host_i;
    iface->info = info_i;
    iface->load_balancer = load_balancer_i;
    iface->http_conn_pool = http_conn_pool_i;
    iface->tcp_conn_pool = tcp_conn_pool_i;
    iface->priority_set = priority_set_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpClusterEntry* self = RP_CLUSTER_ENTRY(obj);
    rp_cluster_entry_drain_conn_pools(self);
    g_clear_object(&self->m_lb);
    g_clear_object(&self->m_priority_set);
    g_clear_object(&self->m_cluster_info);

    G_OBJECT_CLASS(rp_cluster_entry_parent_class)->dispose(obj);
}

static void
rp_cluster_entry_class_init(RpClusterEntryClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_cluster_entry_init(RpClusterEntry* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpClusterEntry*
constructed(RpClusterEntry* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_priority_set = rp_priority_set_impl_new();
    rp_priority_set_impl_get_or_create_host_set(self->m_priority_set, 0);

    g_assert(self->m_lb_factory != NULL);
    RpLoadBalancerParams params = {
        .priority_set = RP_PRIORITY_SET(self->m_priority_set),
        .local_priority_set = rp_thread_local_cluster_manager_impl_local_priority_set_(self->m_parent)
    };
    self->m_lb = rp_load_balancer_factory_create(self->m_lb_factory, &params);
    return self;
}

RpClusterEntry*
rp_cluster_entry_new(RpThreadLocalClusterManagerImpl* parent, RpClusterInfoConstSharedPtr cluster, RpLoadBalancerFactorySharedPtr lb_factory)
{
    LOGD("(%p(%s), %p(%s), %p(%s))",
        parent, G_OBJECT_TYPE_NAME(parent), cluster, G_OBJECT_TYPE_NAME(cluster), lb_factory, lb_factory ? G_OBJECT_TYPE_NAME(lb_factory) : "");

    g_return_val_if_fail(RP_IS_THREAD_LOCAL_CLUSTER_MANAGER_IMPL(parent), NULL);
    g_return_val_if_fail(RP_IS_CLUSTER_INFO((RpClusterInfo*)cluster), NULL);
    g_return_val_if_fail(RP_IS_LOAD_BALANCER_FACTORY(lb_factory), NULL);

    g_autoptr(RpClusterEntry) self = g_object_new(RP_TYPE_CLUSTER_ENTRY, NULL);
    self->m_parent = parent;
    self->m_lb_factory = lb_factory;
    if (!constructed(self))
    {
        LOGE("failed");
        return NULL;
    }
    rp_cluster_info_set_object(&self->m_cluster_info, cluster);
    return g_steal_pointer(&self);
}

void
rp_cluster_entry_drain_conn_pools_hosts_removed(RpClusterEntry* self, const RpHostVector* hosts_removed)
{
    LOGD("(%p, %p)", self, hosts_removed);
    g_return_if_fail(RP_IS_CLUSTER_ENTRY(self));
    for (guint i = 0; i < rp_host_vector_len(hosts_removed); ++i)
    {
        RpHost* host = rp_host_vector_get(hosts_removed, i);
        rp_thread_local_cluster_manager_impl_drain_or_close_conn_pools(self->m_parent, host, RpDrainBehavior_DrainAndDelete);
    }
}

void
rp_cluster_entry_drain_conn_pools(RpClusterEntry* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_CLUSTER_ENTRY(self));
    const RpHostSetPtrVector* host_sets_per_priority = rp_priority_set_host_sets_per_priority(RP_PRIORITY_SET(self->m_priority_set));
    for (guint i = 0; i < rp_host_set_ptr_vector_size(host_sets_per_priority); ++i)
    {
        RpHostSetPtr host_set = rp_host_set_ptr_vector_get(host_sets_per_priority, i);
        RpHostVector* hosts = rp_host_set_ref_hosts(host_set); // Ref.
        rp_cluster_entry_drain_conn_pools_hosts_removed(self, hosts);
        rp_host_vector_unref(hosts);
    }
}

void
rp_cluster_entry_update_hosts(RpClusterEntry* self, const char* name, guint32 priority, RpPrioritySetUpdateHostsParams* update_hosts_params,
                                const RpHostVector* hosts_added, const RpHostVector* hosts_removed,
                                bool* weighted_health_priority_health, guint32* overprovisioning_factor,
                                RpHostMapSnap* cross_priority_host_map)
{
    LOGD("(%p, %p(%s), %u, %p, %p, %p, %p, %p, %p)",
        self, name, name, priority, update_hosts_params,
        hosts_added, hosts_removed, weighted_health_priority_health, overprovisioning_factor, cross_priority_host_map);
    LOGD("membership update for TLS cluster \"%s\", added %u, removed %u", name, rp_host_vector_len(hosts_added), rp_host_vector_len(hosts_removed));
    rp_priority_set_update_hosts(RP_PRIORITY_SET(self->m_priority_set),
                                    priority,
                                    update_hosts_params,
                                    hosts_added,
                                    hosts_removed,
                                    weighted_health_priority_health,
                                    overprovisioning_factor,
                                    cross_priority_host_map);
    //TODO...if (lb_factory_ != nullptr && lb_factory_->recreateOnHostChange())...
}
