/*
 * rp-cluster-store-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_store_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_clusterstore_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-cluster-store.h"

typedef UNIQUE_PTR(GHashTable) ClusterMapType;
typedef struct _ClusterStoreType ClusterStoreType;
struct _ClusterStoreType {
    ClusterMapType m_map;
    GRWLock m_mutex;
};
static inline ClusterStoreType*
cluster_store_type_new(void)
{
    NOISY_MSG_("()");
    ClusterStoreType* self = g_new0(ClusterStoreType, 1);
    self->m_map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    g_rw_lock_init(&self->m_mutex);
    return self;
}

static ClusterStoreType* singleton_cluster_store = NULL;

static inline gpointer
cluster_store_type_init_singleton(gpointer user_data G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", user_data);
    singleton_cluster_store = cluster_store_type_new();
    return singleton_cluster_store;
}

static ClusterStoreType*
get_cluster_store(void)
{
    static GOnce init_once = G_ONCE_INIT;
    NOISY_MSG_("()");
    g_once(&init_once, cluster_store_type_init_singleton, NULL);
    return singleton_cluster_store;
}

struct _RpDfpClusterStore {
    GObject parent_instance;

};

G_DEFINE_TYPE_WITH_CODE(RpDfpClusterStore, rp_dfp_cluster_store, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_SINGLETON_INSTANCE, NULL)
)

static void
rp_dfp_cluster_store_class_init(RpDfpClusterStoreClass* klass)
{
    LOGD("(%p)", klass);
}

static void
rp_dfp_cluster_store_init(RpDfpClusterStore* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpDfpClusterStore*
rp_dfp_cluster_store_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_DFP_CLUSTER_STORE, NULL);
}

RpDfpClusterSharedPtr
rp_dfp_cluster_store_load(RpDfpClusterStore* self, const char* cluster_name)
{
    LOGD("(%p, %p(%s))", self, cluster_name, cluster_name);
    g_return_val_if_fail(RP_IS_DFP_CLUSTER_STORE(self), NULL);
    g_return_val_if_fail(cluster_name != NULL, NULL);
    g_return_val_if_fail(cluster_name[0], NULL);
    ClusterStoreType* cluster_store = get_cluster_store();
    G_RW_LOCK_READER_AUTO_LOCK(&cluster_store->m_mutex, locker);
    return g_hash_table_lookup(cluster_store->m_map, cluster_name);
}

void
rp_dfp_cluster_store_save(RpDfpClusterStore* self, const char* cluster_name, RpDfpClusterSharedPtr cluster)
{
    LOGD("(%p, %p(%s), %p)", self, cluster_name, cluster_name, cluster);
    g_return_if_fail(RP_IS_DFP_CLUSTER_STORE(self));
    g_return_if_fail(cluster_name != NULL);
    g_return_if_fail(cluster_name[0]);
    g_return_if_fail(RP_IS_DFP_CLUSTER(cluster));
    ClusterStoreType* cluster_store = get_cluster_store();
    G_RW_LOCK_WRITER_AUTO_LOCK(&cluster_store->m_mutex, locker);
    g_hash_table_insert(cluster_store->m_map, g_strdup(cluster_name), g_steal_pointer(&cluster));
}

void
rp_dfp_cluster_store_remove(RpDfpClusterStore* self, const char* cluster_name)
{
    LOGD("(%p, %p(%s))", self, cluster_name, cluster_name);
    g_return_if_fail(RP_IS_DFP_CLUSTER_STORE(self));
    g_return_if_fail(cluster_name != NULL);
    g_return_if_fail(cluster_name[0]);
    ClusterStoreType* cluster_store = get_cluster_store();
    G_RW_LOCK_WRITER_AUTO_LOCK(&cluster_store->m_mutex, locker);
    g_hash_table_remove(cluster_store->m_map, cluster_name);
}
