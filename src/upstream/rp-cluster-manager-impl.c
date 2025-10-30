/*
 * rp-cluster-manager-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_cluster_manager_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_manager_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "http1/rp-http1-conn-pool.h"
#include "upstream/rp-cluster-factory-context-impl.h"
#include "rp-cluster-factory.h"
#include "rp-load-balancer.h"
#include "rp-thread-local-cluster.h"
#include "rp-cluster-manager-impl.h"

struct _RpProdClusterManagerFactory {
    GObject parent_instance;

    RpServerFactoryContext* m_context;
    RpInstance* m_server;
    RpClusterInfo* m_cluster_info;
    RpDispatcher* m_dispatcher;

    UNIQUE_PTR(GHashTable) m_active_clusters;

    bool m_initialized;
};

static void cluster_manager_iface_init(RpClusterManagerInterface* iface);
static void cluster_manager_factory_iface_init(RpClusterManagerFactoryInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpProdClusterManagerFactory, rp_prod_cluster_manager_factory, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_CLUSTER_MANAGER, cluster_manager_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_CLUSTER_MANAGER_FACTORY, cluster_manager_factory_iface_init)
)

static RpClusterManagerFactory*
cluster_manager_factory_i(RpClusterManager* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_MANAGER_FACTORY(self);
}

static void
drain_connections_i(RpClusterManager* self, const char* cluster, RpDrainConnectionsHostPredicate predicate)
{
    NOISY_MSG_("(%p, %p(%s), %p)", self, cluster, cluster, predicate);
//TODO...
//cluster == NULL == "drain all clusters"
}

static RpThreadLocalCluster*
get_thread_local_cluster_i(RpClusterManager* self, const char* cluster)
{
    NOISY_MSG_("(%p, %p(%s))", self, cluster, cluster);
    RpProdClusterManagerFactory* me = RP_PROD_CLUSTER_MANAGER_FACTORY(self);
    return RP_THREAD_LOCAL_CLUSTER(g_hash_table_lookup(me->m_active_clusters, cluster));
}

static RpStatusCode_e
initialize_i(RpClusterManager* self)
{
    NOISY_MSG_("(%p)", self);
//TODO...
    RpProdClusterManagerFactory* me = RP_PROD_CLUSTER_MANAGER_FACTORY(self);
    me->m_initialized = true;

return RpStatusCode_Ok;
}

static bool
initialized_i(RpClusterManager* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_PROD_CLUSTER_MANAGER_FACTORY(self)->m_initialized;
}

static RpCluster*
load_cluster(RpProdClusterManagerFactory* self, RpClusterCfg* cluster, bool added_via_api, GHashTable* cluster_map)
{
    NOISY_MSG_("(%p, %p, %u, %p)", self, cluster, added_via_api, cluster_map);
    g_autoptr(RpClusterFactoryContextImpl) context = rp_cluster_factory_context_impl_new(self->m_context, RP_CLUSTER_MANAGER(self), added_via_api);
    RpCluster* new_cluster = rp_cluster_factory_create(cluster->factory, cluster, RP_CLUSTER_FACTORY_CONTEXT(context));
//TODO...lb = ...
    RpClusterInfo* cluster_info = rp_cluster_info(new_cluster);
    const char* name = rp_cluster_info_name(cluster_info);
NOISY_MSG_("name %p(%s)", name, name);
NOISY_MSG_("%p adding %p(%s) to cluster map %p", self, name, name, cluster_map);
    g_hash_table_insert(cluster_map, (gpointer)name, new_cluster);
    return new_cluster;
}

static RpStatusCode_e
init_callback(RpCluster* self)
{
    NOISY_MSG_("(%p)", self);
    return RpStatusCode_Ok;
}

static bool
add_or_update_cluster_i(RpClusterManager* self, RpClusterCfg* cluster, const char* version_info)
{
    NOISY_MSG_("(%p, %p, %p(%s))", self, cluster, version_info, version_info);

    const char* name = cluster->name;
    NOISY_MSG_("name \"%s\"", name);
    RpProdClusterManagerFactory* me = RP_PROD_CLUSTER_MANAGER_FACTORY(self);
    RpCluster* existing_active_cluster = g_hash_table_lookup(me->m_active_clusters, name);
    //TODO...warming_clusters
    if (existing_active_cluster)
    {
        g_hash_table_remove(me->m_active_clusters, name);
    }
    RpCluster* new_cluster = load_cluster(me, cluster, true, me->m_active_clusters/*TODO...warming custers?*/);
    rp_cluster_initialize(new_cluster, init_callback);
    return true;
}

static void
cluster_manager_iface_init(RpClusterManagerInterface* iface)
{
    LOGD("(%p)", iface);
    iface->cluster_manager_factory = cluster_manager_factory_i;
    iface->drain_connections = drain_connections_i;
    iface->get_thread_local_cluster = get_thread_local_cluster_i;
    iface->initialize = initialize_i;
    iface->initialized = initialized_i;
    iface->add_or_update_cluster = add_or_update_cluster_i;
}

static RpHttpConnectionPoolInstancePtr
allocate_conn_pool_i(RpClusterManagerFactory* self, RpDispatcher* dispatcher, RpHost* host, RpResourcePriority_e priority, evhtp_proto* protocols)
{
    NOISY_MSG_("(%p, %p, %p, %d, %p)", self, dispatcher, host, priority, protocols);
//TODO...more complex selection logic; just http1 for now...
    if (protocols[0] != EVHTP_PROTO_11)
    {
        LOGE("protocol %d not supported!", protocols[0]);
        return NULL;
    }
    return http1_allocate_conn_pool(dispatcher, host, priority);
}

static RpCluster*
cluster_from_proto_i(RpClusterManagerFactory* self, RpClusterCfg* cluster, RpClusterManager* cm, bool added_via_api)
{
    NOISY_MSG_("(%p, %p, %p, %u)", self, cluster, cm, added_via_api);
//    rp_cluster_factory_create()
return NULL;
}

static void
cluster_manager_factory_iface_init(RpClusterManagerFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->allocate_conn_pool = allocate_conn_pool_i;
    iface->cluster_from_proto = cluster_from_proto_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpProdClusterManagerFactory* self = RP_PROD_CLUSTER_MANAGER_FACTORY(obj);
    g_clear_pointer(&self->m_active_clusters, g_hash_table_unref);

    G_OBJECT_CLASS(rp_prod_cluster_manager_factory_parent_class)->dispose(obj);
}

static void
rp_prod_cluster_manager_factory_class_init(RpProdClusterManagerFactoryClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_prod_cluster_manager_factory_init(RpProdClusterManagerFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpProdClusterManagerFactory*
constructed(RpProdClusterManagerFactory* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_active_clusters = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
    return self;
}

RpProdClusterManagerFactory*
rp_prod_cluster_manager_factory_new(RpServerFactoryContext* context, RpInstance* server)
{
    LOGD("(%p, %p)", context, server);
    RpProdClusterManagerFactory* self = g_object_new(RP_TYPE_PROD_CLUSTER_MANAGER_FACTORY, NULL);
    self->m_context = context;
    self->m_server = server;
    return constructed(self);
}
