/*
 * rp-thread-local-cluster-manager-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_thread_local_cluster_manager_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_thread_local_cluster_manager_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-thread-local-object.h"
#include "upstream/rp-cluster-discovery-manager.h"
#include "upstream/rp-cluster-manager-impl.h"

struct _RpThreadLocalClusterManagerImpl {
    GObject parent_instance;

    RpClusterManagerImpl* m_parent;
    SHARED_PTR(RpDispatcher) m_thread_local_dispatcher;

    UNIQUE_PTR(GHashTable) m_thread_local_clusters;

    // These maps are owned by the ThreadLocalClusterManagerImpl instead of the ClusterEntry
    // to prevent lifetime/ownership issues when a cluster is dynamically removed.
    UNIQUE_PTR(GHashTable) /* <RpHostConstSharedPtr, RpConnPoolsContainer> */    m_host_http_conn_pool_map;
    UNIQUE_PTR(GHashTable) /* <RpHostConstSharedPtr, RpTcpConnPoolsContainer> */ m_host_tcp_conn_pool_map;
    UNIQUE_PTR(GHashTable) /* <RpHostConstSharedPtr, RpTcpConnectionsMap> */     m_host_tcp_conn_map;

    RpPrioritySet* m_local_priority_set;

    UNIQUE_PTR(GList) m_update_callbacks;
};

static void thread_local_object_iface_init(RpThreadLocalObjectInterface* iface);
static void cluster_lifecycle_callback_handler_iface_init(RpClusterLifecycleCallbackHandlerInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpThreadLocalClusterManagerImpl, rp_thread_local_cluster_manager_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_CLUSTER_LIFECYCLE_CALLBACK_HANDLER, cluster_lifecycle_callback_handler_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_THREAD_LOCAL_OBJECT, thread_local_object_iface_init)
)

static void
thread_local_object_iface_init(RpThreadLocalObjectInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

static RpClusterUpdateCallbacksHandlePtr
add_cluster_update_callbacks_i(RpClusterLifecycleCallbackHandler* self, RpClusterUpdateCallbacks* cb)
{
    NOISY_MSG_("(%p, %p)", self, cb);
    RpThreadLocalClusterManagerImpl* me = RP_THREAD_LOCAL_CLUSTER_MANAGER_IMPL(self);
    return RP_CLUSTER_UPDATE_CALLBACKS_HANDLE(
            rp_cluster_update_callbacks_handler_impl_new(cb, &me->m_update_callbacks));
}

static void
cluster_lifecycle_callback_handler_iface_init(RpClusterLifecycleCallbackHandlerInterface* iface)
{
    LOGD("(%p)", iface);
    iface->add_cluster_update_callbacks = add_cluster_update_callbacks_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpThreadLocalClusterManagerImpl* self = RP_THREAD_LOCAL_CLUSTER_MANAGER_IMPL(obj);
    g_clear_pointer(&self->m_thread_local_clusters, g_hash_table_unref);
    g_clear_pointer(&self->m_host_http_conn_pool_map, g_hash_table_unref);
    g_clear_pointer(&self->m_host_tcp_conn_pool_map, g_hash_table_unref);
    g_clear_pointer(&self->m_host_tcp_conn_map, g_hash_table_unref);
    g_list_free_full(g_steal_pointer(&self->m_update_callbacks), g_object_unref);

    rp_dispatcher_clear_deferred_delete_list(self->m_thread_local_dispatcher);

    G_OBJECT_CLASS(rp_thread_local_cluster_manager_impl_parent_class)->dispose(obj);
}

static void
rp_thread_local_cluster_manager_impl_class_init(RpThreadLocalClusterManagerImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_thread_local_cluster_manager_impl_init(RpThreadLocalClusterManagerImpl* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_thread_local_clusters = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
    self->m_host_http_conn_pool_map = g_hash_table_new_full(NULL, NULL, g_object_unref, (GDestroyNotify)rp_conn_pools_container_free);
    self->m_host_tcp_conn_pool_map = g_hash_table_new_full(NULL, NULL, g_object_unref, (GDestroyNotify)rp_tcp_conn_pools_container_free);
    self->m_host_tcp_conn_map = g_hash_table_new_full(NULL, NULL, g_object_unref, (GDestroyNotify)rp_tcp_connections_map_free);
}

RpThreadLocalClusterManagerImpl*
rp_thread_local_cluster_manager_impl_new(RpClusterManagerImpl* parent, RpDispatcher* dispatcher, RpLocalClusterParams* local_cluster_params)
{
    LOGD("(%p, %p, %p)", parent, dispatcher, local_cluster_params);

    g_return_val_if_fail(RP_IS_CLUSTER_MANAGER_IMPL(parent), NULL);
    g_return_val_if_fail(RP_IS_DISPATCHER(dispatcher), NULL);

    RpThreadLocalClusterManagerImpl* self = g_object_new(RP_TYPE_THREAD_LOCAL_CLUSTER_MANAGER_IMPL, NULL);
    self->m_parent = parent;
    self->m_thread_local_dispatcher = dispatcher;
    if (local_cluster_params)
    {
        const char* local_cluster_name = rp_cluster_info_name(local_cluster_params->m_info);
        RpClusterEntry* cluster_entry = rp_cluster_entry_new(self,
                                                                local_cluster_params->m_info,
                                                                local_cluster_params->m_load_balancer_factory);
        g_hash_table_insert(self->m_thread_local_clusters, g_strdup(local_cluster_name), cluster_entry);
        self->m_local_priority_set = rp_thread_local_cluster_priority_set(RP_THREAD_LOCAL_CLUSTER(cluster_entry));
    }
    return self;
}

void
rp_thread_local_cluster_manager_impl_remove_tcp_conn(RpThreadLocalClusterManagerImpl* self, SHARED_PTR(RpHost) host, RpNetworkClientConnection* connection)
{
    LOGD("(%p, %p, %p)", self, host, connection);

    g_return_if_fail(RP_IS_THREAD_LOCAL_CLUSTER_MANAGER_IMPL(self));
    g_return_if_fail(RP_IS_HOST(host));
    g_return_if_fail(RP_IS_NETWORK_CLIENT_CONNECTION(connection));

    RpTcpConnectionsMap* connections_map = g_hash_table_lookup(self->m_host_tcp_conn_map, host);
    RpTcpConnContainer* container = g_hash_table_lookup(connections_map->m_connections, connection);
    if (g_hash_table_remove(connections_map->m_connections, container))
    {
        rp_dispatcher_deferred_delete(
            rp_network_connection_dispatcher(RP_NETWORK_CONNECTION(connection)), G_OBJECT(container));
        if (rp_tcp_connections_map_empty(connections_map))
        {
            NOISY_MSG_("removing %p", connections_map);
            g_hash_table_remove(self->m_host_tcp_conn_map, connections_map);
        }
    }
}

RpConnPoolsContainer*
rp_thread_local_cluster_manager_impl_get_http_conn_pools_container(RpThreadLocalClusterManagerImpl* self,
                                                                                            RpHostConstSharedPtr host,
                                                                                            bool allocate)
{
    LOGD("(%p, %p, %u)", self, host, allocate);

    g_return_val_if_fail(RP_IS_THREAD_LOCAL_CLUSTER_MANAGER_IMPL(self), NULL);
    g_return_val_if_fail(RP_IS_HOST(host), NULL);

    RpConnPoolsContainer* container = g_hash_table_lookup(self->m_host_http_conn_pool_map, host);
    if (!container)
    {
        if (!allocate)
        {
            NOISY_MSG_("returning");
            return NULL;
        }
        container = rp_conn_pools_container_new(self->m_thread_local_dispatcher, host);
        g_hash_table_insert(self->m_host_http_conn_pool_map, host, container);
    }
    return container;
}

RpClusterEntry*
rp_thread_local_cluster_manager_impl_initialize_cluster_inline_if_exists(RpThreadLocalClusterManagerImpl* self,
                                                                                            const char* cluster)
{
    LOGD("(%p, %p(%s))", self, cluster, cluster);
    g_return_val_if_fail(RP_IS_THREAD_LOCAL_CLUSTER_MANAGER_IMPL(self), NULL);
    g_return_val_if_fail(cluster != NULL, NULL);
    //TODO...???????
    return NULL;
}

RpPrioritySet*
rp_thread_local_cluster_manager_impl_local_priority_set_(RpThreadLocalClusterManagerImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_THREAD_LOCAL_CLUSTER_MANAGER_IMPL(self), NULL);
    return self->m_local_priority_set;
}

RpClusterManagerImpl*
rp_thread_local_cluster_manager_impl_parent_(RpThreadLocalClusterManagerImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_THREAD_LOCAL_CLUSTER_MANAGER_IMPL(self), NULL);
    return self->m_parent;
}

RpDispatcher*
rp_thread_local_cluster_manager_impl_dispatcher_(RpThreadLocalClusterManagerImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_THREAD_LOCAL_CLUSTER_MANAGER_IMPL(self), NULL);
    return self->m_thread_local_dispatcher;
}

GHashTable*
rp_thread_local_cluster_manager_impl_thread_local_clusters_(RpThreadLocalClusterManagerImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_THREAD_LOCAL_CLUSTER_MANAGER_IMPL(self), NULL);
    return self->m_thread_local_clusters;
}

void
rp_thread_local_cluster_manager_impl_drain_or_close_conn_pools(RpThreadLocalClusterManagerImpl* self, RpHostSharedPtr host, RpDrainBehavior_e drain_behavior)
{
    LOGD("(%p, %p, %d)", self, host, drain_behavior);
    g_return_if_fail(RP_IS_THREAD_LOCAL_CLUSTER_MANAGER_IMPL(self));
    g_return_if_fail(RP_IS_HOST(host));
    RpConnPoolsContainer* container = rp_thread_local_cluster_manager_impl_get_http_conn_pools_container(self, host, false);
    if (container)
    {
        container->m_do_not_delete = true;
        rp_priority_conn_pool_map_drain_connections(container->m_pools, drain_behavior);
        container->m_do_not_delete = false;

        if (rp_priority_conn_pool_map_empty(container->m_pools))
        {
            g_hash_table_remove(self->m_host_http_conn_pool_map, host);
        }
    }
    {
        RpTcpConnPoolsContainer* container = g_hash_table_lookup(self->m_host_tcp_conn_map, host);
        if (container)
        {
            g_autoptr(GPtrArray) pools = g_ptr_array_new_full(g_hash_table_size(container->m_pools), NULL/*????*/);
            GHashTableIter itr;
            gpointer key;
            gpointer value;
            guint index = 0;
            g_hash_table_iter_init(&itr, container->m_pools);
            while (g_hash_table_iter_next(&itr, &key, &value))
            {
                pools->pdata[index++] = value;
            }

            for (index = 0; index < pools->len; ++index)
            {
                RpTcpConnPoolInstance* pool = g_ptr_array_index(pools, index);
                if (drain_behavior != RpDrainBehavior_Undefined)
                {
                    rp_connection_pool_instance_drain_connections(RP_CONNECTION_POOL_INSTANCE(pool), drain_behavior);
                }
                else
                {
                    rp_tcp_conn_pool_instance_close_connections(pool);
                }
            }
        }
    }
}

GHashTable*
rp_thread_local_cluster_manager_impl_host_tcp_conn_pool_map_(RpThreadLocalClusterManagerImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_THREAD_LOCAL_CLUSTER_MANAGER_IMPL(self), NULL);
    return self->m_host_tcp_conn_pool_map;
}
