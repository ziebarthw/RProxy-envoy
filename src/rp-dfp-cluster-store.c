/*
 * rp-dfp-cluster-store.c
 *
 * A minimal C analogue of Envoy's Extensions::Common::DynamicForwardProxy::DFPClusterStore.
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_dfp_cluster_store_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_dfp_cluster_store_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <string.h>
#include <glib.h>

#include "rp-dfp-cluster-store.h"

static __thread RpDfpClusterStore* thread_instance = NULL;

G_DEFINE_INTERFACE(RpDfpCluster, rp_dfp_cluster, G_TYPE_OBJECT)

static void
rp_dfp_cluster_default_init(RpDfpClusterInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

struct _RpDfpClusterStore {
    RpClusterManager* m_cluster_manager;
    GHashTable* m_map; // char* -> downstream_t*
};

static void
free_cluster_name_key(gpointer data)
{
    g_free(data);
}

RpDfpClusterStore*
rp_dfp_cluster_store_new(RpClusterManager* cluster_manager)
{
    RpDfpClusterStore* store = g_new0(RpDfpClusterStore, 1);
    store->m_cluster_manager = cluster_manager;
    store->m_map = g_hash_table_new_full(g_str_hash,
                                            g_str_equal,
                                            free_cluster_name_key,
                                            /*free_downstream_value*/NULL);
    return store;
}

void
rp_dfp_cluster_store_free(RpDfpClusterStore* self)
{
    LOGD("(%p)", self);
    if (!self) return;
    g_clear_pointer(&self->m_map, g_hash_table_destroy);
    g_free(self);
    thread_instance = NULL;
}

RpDfpClusterStore*
rp_dfp_cluster_store_get_instance(RpClusterManager* cluster_manager)
{
    LOGD("(%p)", cluster_manager);
    if (G_UNLIKELY(thread_instance == NULL))
    {
        NOISY_MSG_("creating cluster store");
        thread_instance = rp_dfp_cluster_store_new(cluster_manager);
    }
    return thread_instance;
}

RpDfpCluster*
rp_dfp_cluster_store_load(RpDfpClusterStore* self, const char* cluster_name)
{
    LOGD("(%p, %p(%s))", self, cluster_name, cluster_name);
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(cluster_name != NULL, NULL);
    g_return_val_if_fail(cluster_name[0], NULL);
    return g_hash_table_lookup(self->m_map, cluster_name);
}

void
rp_dfp_cluster_store_save(RpDfpClusterStore* self, const char* cluster_name, RpDfpCluster* cluster)
{
    LOGD("(%p, %p(%s), %p)", self, cluster_name, cluster_name, cluster);
    g_return_if_fail(self);
    g_return_if_fail(cluster_name);
    g_return_if_fail(cluster_name[0]);
    g_return_if_fail(RP_IS_DFP_CLUSTER(cluster));
    if (!g_hash_table_insert(self->m_map, (gpointer)cluster_name, cluster))
    {
        LOGE("hash table insert failed!");
    }
}
