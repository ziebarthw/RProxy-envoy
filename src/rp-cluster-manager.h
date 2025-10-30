/*
 * rp-cluster-manager.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-dispatcher.h"
#include "rp-http-conn-pool.h"
#include "rp-thread-local-cluster.h"
#include "rp-cluster-configuration.h"

G_BEGIN_DECLS

typedef struct _RpClusterManagerFactory RpClusterManagerFactory;

/**
 * Manages connection pools and load balancing for upstream clusters. The cluster manager is
 * persistent and shared among multiple ongoing requests/connections.
 * Cluster manager is initialized in two phases. In the first phase which begins at the construction
 * all primary clusters (i.e. with endpoint assignments provisioned statically in bootstrap,
 * discovered through DNS or file based CDS) are initialized. This phase may complete synchronously
 * with cluster manager construction iff all clusters are STATIC and without health checks
 * configured. At the completion of the first phase cluster manager invokes callback set through the
 * `setPrimaryClustersInitializedCb` method.
 * After the first phase has completed the server instance initializes services (i.e. RTDS) needed
 * to successfully deploy the rest of dynamic configuration.
 * In the second phase all secondary clusters (with endpoint assignments provisioned by xDS servers)
 * are initialized and then the rest of the configuration provisioned through xDS.
 */
#define RP_TYPE_CLUSTER_MANAGER rp_cluster_manager_get_type()
G_DECLARE_INTERFACE(RpClusterManager, rp_cluster_manager, RP, CLUSTER_MANAGER, GObject)

typedef bool (*RpDrainConnectionsHostPredicate)(RpClusterManager*, RpHost*);
//TODO...typedef struct _RpClusterInfoMaps RpClusterInfoMaps;

struct _RpClusterManagerInterface {
    GTypeInterface parent_iface;

    RpStatusCode_e (*initialize)(RpClusterManager*/*, bootstrap*/);
    bool (*initialized)(RpClusterManager*);
    bool (*add_or_update_cluster)(RpClusterManager*, RpClusterCfg*, const char*);
    //TODO...RpClusterInfoMaps* (*clusters)(RpClusterManager*);
    //TODO...
    RpThreadLocalCluster* (*get_thread_local_cluster)(RpClusterManager*, const char*);
    bool (*remove_cluster)(RpClusterManager*, const char*);
    void (*shutdown)(RpClusterManager*);
    bool (*is_shutdown)(RpClusterManager*);
    //TODO...
    RpClusterManagerFactory* (*cluster_manager_factory)(RpClusterManager*);
    //TODO...
    void (*drain_connections)(RpClusterManager*, const char*, RpDrainConnectionsHostPredicate);
    //TODO...
};

/*

auto cluster = clusterManager().clusters().getCluster(cluster_name);
auto host = cluster->loadBalancer().chooseHost(...);

*/

static inline RpStatusCode_e
rp_cluster_manager_initialize(RpClusterManager* self/*, bootstrap*/)
{
    return RP_IS_CLUSTER_MANAGER(self) ?
        RP_CLUSTER_MANAGER_GET_IFACE(self)->initialize(self) : false;
}
static inline bool
rp_cluster_manager_initialized(RpClusterManager* self)
{
    return RP_IS_CLUSTER_MANAGER(self) ?
        RP_CLUSTER_MANAGER_GET_IFACE(self)->initialized(self) : false;
}
static inline bool
rp_cluster_manager_add_or_update_cluster(RpClusterManager* self, RpClusterCfg* cluster, const char* version_info)
{
    return RP_IS_CLUSTER_MANAGER(self) ?
        RP_CLUSTER_MANAGER_GET_IFACE(self)->add_or_update_cluster(self, cluster, version_info) :
        false;
}
static inline bool
rp_cluster_manager_remove_cluster(RpClusterManager* self, const char* cluster)
{
    return RP_IS_CLUSTER_MANAGER(self) ?
        RP_CLUSTER_MANAGER_GET_IFACE(self)->remove_cluster(self, cluster) :
        false;
}
static inline void
rp_cluster_manager_shutdown(RpClusterManager* self)
{
    if (RP_IS_CLUSTER_MANAGER(self)) \
        RP_CLUSTER_MANAGER_GET_IFACE(self)->shutdown(self);
}
static inline bool
rp_cluster_manager_is_shutdown(RpClusterManager* self)
{
    return RP_IS_CLUSTER_MANAGER(self) ?
        RP_CLUSTER_MANAGER_GET_IFACE(self)->is_shutdown(self) : false;
}
static inline RpThreadLocalCluster*
rp_cluster_manager_get_thread_local_cluster(RpClusterManager* self, const char* cluster)
{
    return RP_IS_CLUSTER_MANAGER(self) ?
        RP_CLUSTER_MANAGER_GET_IFACE(self)->get_thread_local_cluster(self, cluster) :
        NULL;
}
static inline RpClusterManagerFactory*
rp_cluster_manager_cluster_manager_factory(RpClusterManager* self)
{
    return RP_IS_CLUSTER_MANAGER(self) ?
        RP_CLUSTER_MANAGER_GET_IFACE(self)->cluster_manager_factory(self) : NULL;
}
static inline void
rp_cluster_manager_drain_connections(RpClusterManager* self, const char* cluster, RpDrainConnectionsHostPredicate predicate)
{
    if (RP_IS_CLUSTER_MANAGER(self))
    {
        RP_CLUSTER_MANAGER_GET_IFACE(self)->drain_connections(self, cluster, predicate);
    }
}


/**
 * Factory for objects needed during cluster manager operation.
 */
#define RP_TYPE_CLUSTER_MANAGER_FACTORY rp_cluster_manager_factory_get_type()
G_DECLARE_INTERFACE(RpClusterManagerFactory, rp_cluster_manager_factory, RP, CLUSTER_MANAGER_FACTORY, GObject)

struct _RpClusterManagerFactoryInterface {
    GTypeInterface parent_iface;

    RpHttpConnectionPoolInstancePtr (*allocate_conn_pool)(RpClusterManagerFactory*,
                                                            RpDispatcher*,
                                                            RpHost*,
                                                            RpResourcePriority_e,
                                                            evhtp_proto*);
    RpCluster* (*cluster_from_proto)(RpClusterManagerFactory*, RpClusterCfg*, RpClusterManager*, bool);
};

static inline RpHttpConnectionPoolInstancePtr
rp_cluster_manager_factory_allocate_conn_pool(RpClusterManagerFactory* self, RpDispatcher* dispatcher,
                                                RpHost* host, RpResourcePriority_e priority,
                                                evhtp_proto* protocols)
{
    return RP_IS_CLUSTER_MANAGER_FACTORY(self) ?
        RP_CLUSTER_MANAGER_FACTORY_GET_IFACE(self)->allocate_conn_pool(self, dispatcher, host, priority, protocols) :
        NULL;
}
static inline RpCluster*
rp_cluster_manager_factory_cluster_from_proto(RpClusterManagerFactory* self, RpClusterCfg* cluster, RpClusterManager* cm, bool added_via_api)
{
    return RP_IS_CLUSTER_MANAGER_FACTORY(self) ?
        RP_CLUSTER_MANAGER_FACTORY_GET_IFACE(self)->cluster_from_proto(self, cluster, cm, added_via_api) :
        NULL;
}

G_END_DECLS
