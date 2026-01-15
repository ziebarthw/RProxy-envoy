/*
 * rp-cluster-manager-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"
#include "rp-cluster-manager.h"
#include "rp-factory-context.h"
#include "rp-server-instance.h"
#include "rp-thread-local.h"
#include "upstream/rp-cluster-discovery-manager.h"
#include "upstream/rp-priority-conn-pool-map-impl.h"
#include "upstream/rp-upstream-impl.h"

G_BEGIN_DECLS

// Parameters for calling postThreadLocalClusterUpdate()
typedef struct _RpThreadLocalClusterUpdateParams RpThreadLocalClusterUpdateParams;
struct _RpThreadLocalClusterUpdateParams {
    //TODO...this is highly simplified vs. c++ implementation.
    guint32 m_priority;
    RpHostListPtr m_hosts_added;
    RpHostListPtr m_hosts_removed;
};
static inline RpThreadLocalClusterUpdateParams
rp_thread_local_cluster_update_params_ctor(guint32 priority, RpHostListPtr hosts_added, RpHostListPtr hosts_removed)
{
    RpThreadLocalClusterUpdateParams self = {
        .m_priority = priority,
        .m_hosts_added = hosts_added,
        .m_hosts_removed = hosts_removed
    };
    return self;
}
static inline RpThreadLocalClusterUpdateParams*
rp_thread_local_cluster_update_params_new(guint32 priority, RpHostListPtr hosts_added, RpHostListPtr hosts_removed)
{
    RpThreadLocalClusterUpdateParams* self = g_new(RpThreadLocalClusterUpdateParams, 1);
    *self = rp_thread_local_cluster_update_params_ctor(priority, hosts_added, hosts_removed);
    return self;
}


/**
 * Production implementation of ClusterManagerFactory.
 */
#define RP_TYPE_PROD_CLUSTER_MANAGER_FACTORY rp_prod_cluster_manager_factory_get_type()
G_DECLARE_FINAL_TYPE(RpProdClusterManagerFactory, rp_prod_cluster_manager_factory, RP, PROD_CLUSTER_MANAGER_FACTORY, GObject)

RpProdClusterManagerFactory* rp_prod_cluster_manager_factory_new(RpServerFactoryContext* context,
                                                                    RpThreadLocalInstance* tls,
                                                                    RpServerInstance* server);


/**
 * Wrapper for a cluster owned by the cluster manager. Used by both the cluster manager and the
 * cluster manager init helper which needs to pass clusters back to the cluster manager.
 */
#define RP_TYPE_CLUSTER_MANAGER_CLUSTER rp_cluster_manager_cluster_get_type()
G_DECLARE_INTERFACE(RpClusterManagerCluster, rp_cluster_manager_cluster, RP, CLUSTER_MANAGER_CLUSTER, GObject)

struct _RpClusterManagerClusterInterface {
    GTypeInterface parent_iface;

    RpCluster* (*cluster)(RpClusterManagerCluster*);
    RpLoadBalancerFactorySharedPtr (*load_balancer_factory)(RpClusterManagerCluster*);
    bool (*added_or_updated)(RpClusterManagerCluster*);
    void (*set_added_or_updated)(RpClusterManagerCluster*);
    bool (*required_for_ads)(RpClusterManagerCluster*);
};

static inline RpCluster*
rp_cluster_manager_cluster_cluster(RpClusterManagerCluster* self)
{
    return RP_IS_CLUSTER_MANAGER_CLUSTER(self) ?
        RP_CLUSTER_MANAGER_CLUSTER_GET_IFACE(self)->cluster(self) : NULL;
}
static inline RpLoadBalancerFactorySharedPtr
rp_cluster_manager_cluster_load_balancer_factory(RpClusterManagerCluster* self)
{
    return RP_IS_CLUSTER_MANAGER_CLUSTER(self) ?
        RP_CLUSTER_MANAGER_CLUSTER_GET_IFACE(self)->load_balancer_factory(self) :
        NULL;
}
static inline bool
rp_cluster_manager_cluster_added_or_updated(RpClusterManagerCluster* self)
{
    return RP_IS_CLUSTER_MANAGER_CLUSTER(self) ?
        RP_CLUSTER_MANAGER_CLUSTER_GET_IFACE(self)->added_or_updated(self) :
        false;
}
static inline void
rp_cluster_manager_cluster_set_added_or_updated(RpClusterManagerCluster* self)
{
    if (RP_IS_CLUSTER_MANAGER_CLUSTER(self)) \
        RP_CLUSTER_MANAGER_CLUSTER_GET_IFACE(self)->set_added_or_updated(self);
}
static inline bool
rp_cluster_manager_cluster_required_for_ads(RpClusterManagerCluster* self)
{
    return RP_IS_CLUSTER_MANAGER_CLUSTER(self) ?
        RP_CLUSTER_MANAGER_CLUSTER_GET_IFACE(self)->required_for_ads(self) :
        false;
}


typedef RpStatusCode_e (*RpPerClusterInitCallback)(RpClusterManager*,
                                                    RpClusterManagerCluster*);

/**
 * This is a helper class used during cluster management initialization. Dealing with primary
 * clusters, secondary clusters, and CDS, is quite complicated, so this makes it easier to test.
 */
#define RP_TYPE_CLUSTER_MANAGER_INIT_HELPER rp_cluster_manager_init_helper_get_type()
G_DECLARE_FINAL_TYPE(RpClusterManagerInitHelper, rp_cluster_manager_init_helper, RP, CLUSTER_MANAGER_INIT_HELPER, GObject)

RpClusterManagerInitHelper* rp_cluster_manager_init_helper_new(RpClusterManager* cm,
                                                                RpPerClusterInitCallback cb);
void rp_cluster_manager_init_helper_add_cluster(RpClusterManagerInitHelper* self,
                                                RpClusterManagerCluster* cm_cluster);
void rp_cluster_manager_init_helper_on_static_load_complete(RpClusterManagerInitHelper* self);
void rp_cluster_manager_init_helper_remove_cluster(RpClusterManagerInitHelper* self,
                                                    RpClusterManagerCluster* cluster);
void rp_cluster_manager_init_helper_set_primary_clusters_initialized_cb(RpClusterManagerInitHelper* self,
                                                                        RpPrimaryClustersReadyCallback cb,
                                                                        gpointer arg);
void rp_cluster_manager_init_helper_set_initialized_cb(RpClusterManagerInitHelper* self,
                                                        RpInitializationCompleteCallback cb,
                                                        gpointer arg);


/**
 * Implementation of ClusterManager that reads from a proto configuration, maintains a central
 * cluster list, as well as thread local caches of each cluster and associated connection pools.
 */
#define RP_TYPE_CLUSTER_MANAGER_IMPL rp_cluster_manager_impl_get_type()
G_DECLARE_FINAL_TYPE(RpClusterManagerImpl, rp_cluster_manager_impl, RP, CLUSTER_MANAGER_IMPL, GObject)

RpClusterManagerImpl* rp_cluster_manager_impl_new(rproxy_t* bootstrap,
                                                    RpClusterManagerFactory* factory,
                                                    RpCommonFactoryContext* context,
                                                    RpThreadLocalInstance* tls,
                                                    RpDispatcher* main_thread_dispatcher,
                                                    RpServerInstance* server,
                                                    RpStatusCode_e* creation_status);
RpClusterManagerFactory* rp_cluster_manager_impl_factory_(RpClusterManagerImpl* self);


/**
 * A cluster initialization object (CIO) encapsulates the relevant information
 * to create a cluster inline when there is traffic to it. We can thus use the
 * CIO to deferred instantiating clusters on workers until they are used.
 */
#define RP_TYPE_CLUSTER_INITIALIZATION_OBJECT rp_cluster_initialization_object_get_type()
G_DECLARE_FINAL_TYPE(RpClusterInitializationObject, rp_cluster_initialization_object, RP, CLUSTER_INITIALIZATION_OBJECT, GObject)

typedef SHARED_PTR(RpClusterInitializationObject) RpClusterInitializationObjectConstSharedPtr;
typedef SHARED_PTR(GHashTable) /*<std::string, ClusterInititalizationObjectConstSharedPtr>*/ RpClusterInitializationMap;

RpClusterInitializationObject* rp_cluster_initialization_object_new(const RpThreadLocalClusterUpdateParams* params, RpClusterInfoConstSharedPtr cluster_info, RpLoadBalancerFactorySharedPtr load_balancer_factory, RpHostMapConstSharedPtr map/*, TODO...*/);


typedef struct _RpThreadLocalClusterManagerImpl RpThreadLocalClusterManagerImpl;

/*
 * RpLocalClusterParams
*/
typedef struct _RpLocalClusterParams RpLocalClusterParams;
struct _RpLocalClusterParams {
    RpLoadBalancerFactorySharedPtr m_load_balancer_factory;
    RpClusterInfoConstSharedPtr m_info;
};


/*
 * RpConnPoolsContainer
*/
typedef struct _RpConnPoolsContainer RpConnPoolsContainer;
struct _RpConnPoolsContainer {
    RpHostConstSharedPtr m_host_handle;
//  using ConnPools = PriorityConnPoolMap<std::vector<uint8_t>, Http::ConnectionPool::Instance>;
//  std:;shared_ptr<ConnPools> pools_;
    SHARED_PTR(RpPriorityConnPoolMap) m_pools;
    bool m_do_not_delete;
};
static inline RpConnPoolsContainer
rp_conn_pools_container_ctor(RpHostConstSharedPtr host, SHARED_PTR(RpPriorityConnPoolMap) pools)
{
    RpConnPoolsContainer self = {
        .m_host_handle = g_object_ref(host),
        .m_pools = pools,
        .m_do_not_delete = false
    };
    return self;
}
static inline void
rp_conn_pools_container_dtor(RpConnPoolsContainer* self)
{
    g_return_if_fail(self != NULL);
    g_clear_object(&self->m_host_handle);
    g_clear_object(&self->m_pools);
}
static inline RpConnPoolsContainer*
rp_conn_pools_container_new(RpDispatcher* dispatcher, RpHostConstSharedPtr host)
{
    RpConnPoolsContainer* self = g_new(RpConnPoolsContainer, 1);
    RpPriorityConnPoolMapImpl* conn_pool_map = rp_priority_conn_pool_map_impl_new(dispatcher, host);
    *self = rp_conn_pools_container_ctor(host, RP_PRIORITY_CONN_POOL_MAP(conn_pool_map));
    return self;
}
static inline void
rp_conn_pools_container_free(RpConnPoolsContainer* self)
{
    g_return_if_fail(self != NULL);
    rp_conn_pools_container_dtor(self);
    g_free(self);
}


/*
 * RpTcpConnPoolsContainer
*/
typedef struct _RpTcpConnPoolsContainer RpTcpConnPoolsContainer;
struct _RpTcpConnPoolsContainer {
    RpHostConstSharedPtr m_host_handle;
//  using ConnPools = std::map<std::vector<uint8_t>, Tcp::ConnectionPool::InstancePtr>;
//  ConnPools pools_;
    GHashTable* /* <int, RpTcpConnPoolInstancePtr> */ m_pools;
};
static inline RpTcpConnPoolsContainer
rp_tcp_conn_pools_container_ctor(RpHostConstSharedPtr host, GHashTable* pools)
{
    RpTcpConnPoolsContainer self = {
        .m_host_handle = g_object_ref(host),
        .m_pools = pools
    };
    return self;
}
static inline void
rp_tcp_conn_pools_container_dtor(RpTcpConnPoolsContainer* self)
{
    g_return_if_fail(self != NULL);
    g_clear_object(&self->m_host_handle);
    g_clear_pointer(&self->m_pools, g_hash_table_unref);
}
static inline RpTcpConnPoolsContainer*
rp_tcp_conn_pools_container_new(RpHostConstSharedPtr host)
{
    RpTcpConnPoolsContainer* self = g_new(RpTcpConnPoolsContainer, 1);
    *self = rp_tcp_conn_pools_container_ctor(host, g_hash_table_new(g_int_hash, g_int_equal));
    return self;
}
static inline void
rp_tcp_conn_pools_container_free(RpTcpConnPoolsContainer* self)
{
    g_return_if_fail(self != NULL);
    rp_tcp_conn_pools_container_dtor(self);
    g_free(self);
}


/**
 * RpTcpConnContainer
 * Holds an unowned reference to a connection, and watches for Closed events. If the connection
 * is closed, this container removes itself from the container that owns it.
*/
#define RP_TYPE_TCP_CONN_CONTAINER rp_tcp_conn_container_get_type()
G_DECLARE_FINAL_TYPE(RpTcpConnContainer, rp_tcp_conn_container, RP, TCP_CONN_CONTAINER, GObject)

RpTcpConnContainer* rp_tcp_conn_container_new(RpThreadLocalClusterManagerImpl* parent,
                                                SHARED_PTR(RpHost) host,
                                                RpNetworkClientConnection* connection);


typedef struct _RpTcpConnectionsMap RpTcpConnectionsMap;
struct _RpTcpConnectionsMap {
//  TcpConnectionsMap(HostHandlePtr&& host_handle) : host_handle_(std::move(host_handle)) {}

    // Destroyed after pools.
    RpHostConstSharedPtr m_host_handle;
//  absl::node_hash_map<Network::ClientConnection*, std::unique_ptr<TcpConnContainer>> connections_;
    GHashTable* /* <RpNetworkClientConnection, UNIQUE_PTR(RpTcpConnContainer)> */ m_connections;
};
static inline bool
rp_tcp_connections_map_empty(RpTcpConnectionsMap* self)
{
    return g_hash_table_size(self->m_connections) == 0;
}
static inline RpTcpConnectionsMap
rp_tcp_connections_map_ctor(RpHostConstSharedPtr host_handle, GHashTable* connections)
{
    RpTcpConnectionsMap self = {
        .m_host_handle = g_object_ref(host_handle),
        .m_connections = connections
    };
    return self;
}
static inline void
rp_tcp_connections_map_dtor(RpTcpConnectionsMap* self)
{
    g_hash_table_destroy(g_steal_pointer(&self->m_connections));
    g_clear_object(&self->m_host_handle);
}
static inline RpTcpConnectionsMap*
rp_tcp_connections_map_new(RpHostConstSharedPtr host_handle)
{
    RpTcpConnectionsMap* self = g_new(RpTcpConnectionsMap, 1);
    *self = rp_tcp_connections_map_ctor(host_handle, g_hash_table_new_full(NULL, NULL, NULL, g_object_unref));
    return self;
}
static inline void
rp_tcp_connections_map_free(RpTcpConnectionsMap* self)
{
    g_return_if_fail(self != NULL);
    rp_tcp_connections_map_dtor(self);
    g_free(self);
}


/*
 * RpClusterEntry
*/
#define RP_TYPE_CLUSTER_ENTRY rp_cluster_entry_get_type()
G_DECLARE_FINAL_TYPE(RpClusterEntry, rp_cluster_entry, RP, CLUSTER_ENTRY, GObject)

RpClusterEntry* rp_cluster_entry_new(RpThreadLocalClusterManagerImpl* parent,
                                        RpClusterInfoConstSharedPtr cluster,
                                        RpLoadBalancerFactorySharedPtr lb_factory);
void rp_cluster_entry_drain_conn_pools(RpClusterEntry* self);
void rp_cluster_entry_drain_conn_pools_hosts_removed(RpClusterEntry* self,
                                                        RpHostVector* hosts_removed);


/**
 * Thread local cached cluster data. Each thread local cluster gets updates from the parent
 * central dynamic cluster (if applicable). It maintains load balancer state and any created
 * connection pools.
 */
#define RP_TYPE_THREAD_LOCAL_CLUSTER_MANAGER_IMPL rp_thread_local_cluster_manager_impl_get_type()
G_DECLARE_FINAL_TYPE(RpThreadLocalClusterManagerImpl, rp_thread_local_cluster_manager_impl, RP, THREAD_LOCAL_CLUSTER_MANAGER_IMPL, GObject)

RpThreadLocalClusterManagerImpl* rp_thread_local_cluster_manager_impl_new(RpClusterManagerImpl* parent,
                                                                            RpDispatcher* dispatcher,
                                                                            RpLocalClusterParams* local_cluster_params);
void rp_thread_local_cluster_manager_impl_remove_tcp_conn(RpThreadLocalClusterManagerImpl* self,
                                                            RpHostConstSharedPtr host,
                                                            RpNetworkClientConnection* connection);
RpConnPoolsContainer* rp_thread_local_cluster_manager_impl_get_http_conn_pools_container(RpThreadLocalClusterManagerImpl* self,
                                                                                            RpHostConstSharedPtr host,
                                                                                            bool allocate);
RpClusterEntry* rp_thread_local_cluster_manager_impl_initialize_cluster_inline_if_exists(RpThreadLocalClusterManagerImpl* self,
                                                                                            const char* cluster);
void rp_thread_local_cluster_manager_impl_drain_or_close_conn_pools(RpThreadLocalClusterManagerImpl* self, RpHostSharedPtr host, RpDrainBehavior_e drain_bahavior);
RpPrioritySet* rp_thread_local_cluster_manager_impl_local_priority_set_(RpThreadLocalClusterManagerImpl* self);
RpClusterManagerImpl* rp_thread_local_cluster_manager_impl_parent_(RpThreadLocalClusterManagerImpl* self);
RpDispatcher* rp_thread_local_cluster_manager_impl_dispatcher_(RpThreadLocalClusterManagerImpl* self);
GHashTable* rp_thread_local_cluster_manager_impl_thread_local_clusters_(RpThreadLocalClusterManagerImpl* self);
GHashTable* rp_thread_local_cluster_manager_impl_host_tcp_conn_pool_map_(RpThreadLocalClusterManagerImpl* self);


typedef struct UNIQUE_PTR(_RpClusterData) RpClusterDataPtr;

#define RP_TYPE_CLUSTER_DATA rp_cluster_data_get_type()
G_DECLARE_FINAL_TYPE(RpClusterData, rp_cluster_data, RP, CLUSTER_DATA, GObject)

RpClusterData* rp_cluster_data_new(RpClusterCfg* cluster_config, bool added_via_api, SHARED_PTR(RpCluster) cluster, RpTimeSource* time_source);
RpThreadAwareLoadBalancerPtr* rp_cluster_data_thread_aware_lb_(RpClusterData* self);


/**
 * RpClusterUpdateCallbacksHandlerImpl
 */
#define RP_TYPE_CLUSTER_UPDATE_CALLBACKS_HANDLER_IMPL rp_cluster_update_callbacks_handler_impl_get_type()
G_DECLARE_FINAL_TYPE(RpClusterUpdateCallbacksHandlerImpl, rp_cluster_update_callbacks_handler_impl, RP, CLUSTER_UPDATE_CALLBACKS_HANDLER_IMPL, GObject)

RpClusterUpdateCallbacksHandlerImpl* rp_cluster_update_callbacks_handler_impl_new(RpClusterUpdateCallbacks* cb, GList** parent);

G_END_DECLS
