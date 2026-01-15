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
#include "upstream/rp-upstream-impl.h"
#include "upstream/rp-priority-conn-pool-map.h"
#include "upstream/rp-cluster-manager-impl.h"

struct _RpClusterEntry {
    GObject parent_instance;

    RpThreadLocalClusterManagerImpl* m_parent;
    RpPrioritySetImpl* m_priority_set;
    RpClusterInfoConstSharedPtr m_cluster_info;
    RpLoadBalancerFactorySharedPtr m_lb_factory;
    RpLoadBalancerPtr m_lb;
};

static void thread_local_cluster_iface_init(RpThreadLocalClusterInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpClusterEntry, rp_cluster_entry, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_THREAD_LOCAL_CLUSTER, thread_local_cluster_iface_init)
)

static void
drain_conn_pools(RpClusterEntry* self)
{
    NOISY_MSG_("(%p)", self);
}

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
NOISY_MSG_("calling rp_load_balancer_choose_host(%p, %p)", me->m_lb, context);
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

static RpClusterInfoConstSharedPtr
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

// Custom hash function for byte arrays
static inline int
byte_array_hash(gconstpointer v, gsize size)
{
    const uint8_t* data = (const uint8_t*)v;
    // You'd need to know the length - this is a simple example
    // In practice, you might want to pass a struct containing data and length
    int hash = 0;
    for (gsize i = 0; i < size; i++)
    {
        hash = (hash << 5) - hash + data[i];
    }
    return hash;
}

typedef struct _RpConnectionPoolFactoryCtx RpConnectionPoolFactoryCtx;
struct _RpConnectionPoolFactoryCtx {
    RpClusterEntry* m_cluster_entry;
    SHARED_PTR(RpHost) m_host;
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
http_conn_pool_impl(RpClusterEntry* self, SHARED_PTR(RpHost) host, RpResourcePriority_e priority, evhtp_proto downstream_protocol, RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %p, %d, %d, %p)", self, host, priority, downstream_protocol, context);

    if (!host)
    {
        NOISY_MSG_("host is null");
        return NULL;
    }

    evhtp_proto* upstream_protocols = rp_cluster_info_upstream_http_protocol(
        rp_host_description_cluster(RP_HOST_DESCRIPTION(host)), downstream_protocol);
    //TODO...

    // For now, just use the downstream protocol...
    guint8 hash_key[4];
    gsize hash_size = 0;
    hash_key[hash_size++] = upstream_protocols[0];
    int key = byte_array_hash(hash_key, hash_size);

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
                                                &key,
                                                conn_pool_factory_cb,
                                                &ctx);
}

static RpTcpConnPoolInstance*
tcp_conn_pool_impl(RpClusterEntry* self, RpHostConstSharedPtr host, RpResourcePriority_e priority, RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %p, %d, %p)", self, host, priority, context);
    RpClusterEntry* me = RP_CLUSTER_ENTRY(self);
    guint8 hash_key[4];
    gsize hash_size = 0;

    hash_key[hash_size++] = priority;
    int key = byte_array_hash(hash_key, hash_size);

    GHashTable* map = rp_thread_local_cluster_manager_impl_host_tcp_conn_pool_map_(me->m_parent);
    RpTcpConnPoolsContainer* container = g_hash_table_lookup(map, host);
    if (!container)
    {
        NOISY_MSG_("creating container");
        container = rp_tcp_conn_pools_container_new(host);
    }
    RpTcpConnPoolInstancePtr pool = g_hash_table_lookup(container->m_pools, &key);
    if (!pool)
    {
        RpDispatcher* dispatcher = rp_thread_local_cluster_manager_impl_dispatcher_(me->m_parent);
        RpClusterManagerImpl* parent_ = rp_thread_local_cluster_manager_impl_parent_(me->m_parent);
        RpClusterManagerFactory* factory = rp_cluster_manager_impl_factory_(parent_);
        pool = rp_cluster_manager_factory_allocate_tcp_conn_pool(factory, dispatcher, host, priority);
        g_hash_table_insert(container->m_pools, &key, pool);
    }
    return pool;
}

static void
maybe_pre_connect(RpClusterEntry* self)
{
    NOISY_MSG_("(%p)", self);
    //TODO...
}

typedef struct _RpHttpPreConnectionCtx RpHttpPreConnectionCtx;
struct _RpHttpPreConnectionCtx {
    RpClusterEntry* m_cluster_entry;
    RpResourcePriority_e m_priority;
    evhtp_proto m_protocol;
    RpLoadBalancerContext* m_context;
};

static void
http_pre_connect(RpHttpConnectionPoolInstancePtr pool, gconstpointer user_data)
{
    NOISY_MSG_("(%p, %p)", pool, user_data);
//    rp_connection_pool_instance_maybe_preconnect(RP_CONNECTION_POOL_INSTANCE(pool), 1.0);
    RpHttpPreConnectionCtx* ctx = (RpHttpPreConnectionCtx*)user_data;
    maybe_pre_connect(ctx->m_cluster_entry);
}

static RpHttpPoolData*
http_conn_pool_i(RpThreadLocalCluster* self, SHARED_PTR(RpHost) host, RpResourcePriority_e priority, evhtp_proto downstream_protocol, RpLoadBalancerContext* context)
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

    RpHttpPreConnectionCtx ctx = {
        .m_cluster_entry = me,
        .m_priority = priority,
        .m_protocol = downstream_protocol,
        .m_context = context
    };
    return rp_http_pool_data_new(http_pre_connect, &ctx, pool);
}

typedef struct _RpTcpPreConnectionCtx RpTcpPreConnectionCtx;
struct _RpTcpPreConnectionCtx {
    RpClusterEntry* m_cluster_entry;
    RpResourcePriority_e m_priority;
    RpLoadBalancerContext* m_context;
};

static void
tcp_pre_connect(RpTcpConnPoolInstancePtr pool, gpointer user_data)
{
    NOISY_MSG_("(%p, %p)", pool, user_data);
//    rp_connection_pool_instance_maybe_preconnect(RP_CONNECTION_POOL_INSTANCE(pool), 1.0);
    RpHttpPreConnectionCtx* ctx = (RpHttpPreConnectionCtx*)user_data;
    maybe_pre_connect(ctx->m_cluster_entry);
}

static RpTcpPoolData*
tcp_conn_pool_i(RpThreadLocalCluster* self, RpHostConstSharedPtr host, RpResourcePriority_e priority, RpLoadBalancerContext* context)
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

    RpTcpPreConnectionCtx ctx = {
        .m_cluster_entry = me,
        .m_priority = priority,
        .m_context = context
    };
    return rp_tcp_pool_data_new(tcp_pre_connect, &ctx, pool);
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
    drain_conn_pools(self);
    g_clear_object(&self->m_lb);

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

if (self->m_lb_factory)
{
    RpLoadBalancerParams params = {
        .priority_set = RP_PRIORITY_SET(self->m_priority_set),
        .local_priority_set = rp_thread_local_cluster_manager_impl_local_priority_set_(self->m_parent)
    };
    self->m_lb = rp_load_balancer_factory_create(self->m_lb_factory, &params);
NOISY_MSG_("lb %p", self->m_lb);
}
    return self;
}

RpClusterEntry*
rp_cluster_entry_new(RpThreadLocalClusterManagerImpl* parent, RpClusterInfoConstSharedPtr cluster, RpLoadBalancerFactorySharedPtr lb_factory)
{
    LOGD("(%p, %p, %p)", parent, cluster, lb_factory);

    g_return_val_if_fail(RP_IS_THREAD_LOCAL_CLUSTER_MANAGER_IMPL(parent), NULL);
    g_return_val_if_fail(RP_IS_CLUSTER_INFO(cluster), NULL);
//TODO...    g_return_val_if_fail(RP_IS_LOAD_BALANCER_FACTORY(lb_factory), NULL);

    RpClusterEntry* self = g_object_new(RP_TYPE_CLUSTER_ENTRY, NULL);
    self->m_parent = parent;
    self->m_cluster_info = cluster;
    self->m_lb_factory = lb_factory;
    return constructed(self);
}

void
rp_cluster_entry_drain_conn_pools_hosts_removed(RpClusterEntry* self, RpHostVector* hosts_removed)
{
    LOGD("(%p, %p)", self, hosts_removed);
    g_return_if_fail(RP_IS_CLUSTER_ENTRY(self));
    g_return_if_fail(hosts_removed != NULL);
    for (guint index = 0; index < hosts_removed->len; ++index)
    {
        RpHostSharedPtr host = g_ptr_array_index(hosts_removed, index);
        rp_thread_local_cluster_manager_impl_drain_or_close_conn_pools(self->m_parent, host, RpDrainBehavior_DrainAndDelete);
    }
}

void
rp_cluster_entry_drain_conn_pools(RpClusterEntry* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_CLUSTER_ENTRY(self));
    RpHostSetPtrVector host_sets_per_priority = rp_priority_set_host_sets_per_priority(RP_PRIORITY_SET(self->m_priority_set));
    for (guint i = 0; i < host_sets_per_priority->len; ++i)
    {
        RpHostSetPtr host_set = g_ptr_array_index(host_sets_per_priority, i);
        RpHostVector* hosts = rp_host_set_hosts(host_set);
        rp_cluster_entry_drain_conn_pools_hosts_removed(self, hosts);
    }
}
