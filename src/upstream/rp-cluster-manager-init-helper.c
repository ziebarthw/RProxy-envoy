/*
 * rp-cluster-manager-init-helper.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_manager_init_helper_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_manager_init_helper_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-cluster-manager-impl.h"

struct _RpClusterManagerInitHelper {
    GObject parent_instance;

    RpClusterManager* m_cm;
    RpPerClusterInitCallback m_per_cluster_init_callback;

    RpPrimaryClustersReadyCallback m_primary_clusters_initialized_callback;
    RpInitializationCompleteCallback m_initalization_callback;

    gpointer m_primary_clusters_initialized_callback_arg;
    gpointer m_initalization_callback_arg;

    gpointer /*TODO...CdsApi**/ m_cds;

    UNIQUE_PTR(GHashTable) m_primary_init_clusters;
    UNIQUE_PTR(GHashTable) m_secondary_init_clusters;

    enum {
        State_Loading,
        State_WaitingForPrimaryInitializationToComplete,
        State_WaitingToStartSecondaryInitialization,
        State_WaitingToStartCdsInitialization,
        State_CdsInitialized,
        State_AllClustersInitialized
    } m_state;

    bool m_started_secondary_initialize;
};

G_DEFINE_FINAL_TYPE(RpClusterManagerInitHelper, rp_cluster_manager_init_helper, G_TYPE_OBJECT)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpClusterManagerInitHelper* self = RP_CLUSTER_MANAGER_INIT_HELPER(obj);
    g_clear_pointer(&self->m_primary_init_clusters, g_hash_table_unref);
    g_clear_pointer(&self->m_secondary_init_clusters, g_hash_table_unref);

    G_OBJECT_CLASS(rp_cluster_manager_init_helper_parent_class)->dispose(obj);
}

static void
rp_cluster_manager_init_helper_class_init(RpClusterManagerInitHelperClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_cluster_manager_init_helper_init(RpClusterManagerInitHelper* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_state = State_Loading;
    self->m_started_secondary_initialize = false;
    self->m_primary_init_clusters = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    self->m_secondary_init_clusters = NULL;//TODO...g_hash_table_new_full(...);
    self->m_cds = NULL;
}

RpClusterManagerInitHelper*
rp_cluster_manager_init_helper_new(RpClusterManager* cm, RpPerClusterInitCallback cb)
{
    LOGD("(%p, %p)", cm, cb);

    g_return_val_if_fail(RP_IS_CLUSTER_MANAGER(cm), NULL);
    g_return_val_if_fail(cb != NULL, NULL);

    RpClusterManagerInitHelper* self = g_object_new(RP_TYPE_CLUSTER_MANAGER_INIT_HELPER, NULL);
    self->m_cm = cm;
    self->m_per_cluster_init_callback = cb;
    return self;
}

static void
maybe_finish_initialize(RpClusterManagerInitHelper* self)
{
    NOISY_MSG_("(%p)", self);
    if (self->m_state == State_Loading || self->m_state == State_WaitingToStartCdsInitialization)
    {
        NOISY_MSG_("returning");
        return;
    }

    g_assert(self->m_state == State_WaitingForPrimaryInitializationToComplete ||
                self->m_state == State_CdsInitialized ||
                self->m_state == State_WaitingForPrimaryInitializationToComplete);
    if (g_hash_table_size(self->m_primary_init_clusters))
    {
        NOISY_MSG_("returning");
        return;
    }
    else if (self->m_state == State_WaitingForPrimaryInitializationToComplete)
    {
        self->m_state = State_WaitingToStartSecondaryInitialization;
        if (self->m_primary_clusters_initialized_callback)
        {
            self->m_primary_clusters_initialized_callback(self->m_primary_clusters_initialized_callback_arg);
        }
        return;
    }

    if (self->m_secondary_init_clusters && g_hash_table_size(self->m_secondary_init_clusters))
    {
        if (!self->m_started_secondary_initialize)
        {
            //TODO...
        }
        return;
    }

    self->m_started_secondary_initialize = false;
    if (self->m_state == State_WaitingToStartCdsInitialization && self->m_cds)
    {
        NOISY_MSG_("cm init: initializing cds");
        self->m_state = State_WaitingToStartCdsInitialization;
        //TODO...cds_->initialize();
    }
    else
    {
        NOISY_MSG_("cm init: all clusters initialized");
        self->m_state = State_AllClustersInitialized;
        if (self->m_initalization_callback)
        {
            self->m_initalization_callback(self->m_initalization_callback_arg);
        }
    }
}

void
rp_cluster_manager_init_helper_remove_cluster(RpClusterManagerInitHelper* self, RpClusterManagerCluster* cluster)
{
    LOGD("(%p, %p)", self, cluster);
    g_return_if_fail(RP_IS_CLUSTER_MANAGER_INIT_HELPER(self));
    g_return_if_fail(RP_IS_CLUSTER_MANAGER_CLUSTER(cluster));
    if (self->m_state == State_AllClustersInitialized)
    {
        NOISY_MSG_("already done");
        return;
    }
    UNIQUE_PTR(GHashTable) cluster_map = self->m_primary_init_clusters;
    //TODO...if (cluster.cluster().initializationPhase() == Cluster::InitializationPhase:;Primary) {}
    const char* name = rp_cluster_info_name(
                        rp_cluster_info(
                            rp_cluster_manager_cluster_cluster(cluster)));
    RpClusterManagerCluster* cluster_ = g_hash_table_lookup(cluster_map, name);
    if (cluster_ && cluster_ == cluster)
    {
        if (!g_hash_table_remove(cluster_map, name))
        {
            LOGD("\"%s\" not found", name);
        }
    }
    maybe_finish_initialize(self);
}

static RpStatusCode_e
on_cluster_init(RpClusterManagerInitHelper* self, RpClusterManagerCluster* cluster)
{
    NOISY_MSG_("(%p, %p)", self, cluster);
    g_assert(self->m_state != State_AllClustersInitialized);
    RpStatusCode_e status = self->m_per_cluster_init_callback(self->m_cm, cluster);
    if (status != RpStatusCode_Ok)
    {
        LOGD("failed with status code %d", status);
        return status;
    }
    rp_cluster_manager_init_helper_remove_cluster(self, cluster);
    return RpStatusCode_Ok;
}

typedef struct _RpInitializeCbCtx RpInitializeCbCtx;
struct _RpInitializeCbCtx {
    RpClusterManagerInitHelper* cm_init_helper;
    RpClusterManagerCluster* cm_cluster;
};
static inline RpInitializeCbCtx
rp_initialize_cb_ctx_ctor(RpClusterManagerInitHelper* cm_init_helper, RpClusterManagerCluster* cm_cluster)
{
    RpInitializeCbCtx self = {
        .cm_init_helper = cm_init_helper,
        .cm_cluster = cm_cluster
    };
    return self;
}
static inline RpInitializeCbCtx*
rp_initialize_cb_ctx_new(RpClusterManagerInitHelper* cm_init_helper, RpClusterManagerCluster* cm_cluster)
{
    RpInitializeCbCtx* self = g_new(RpInitializeCbCtx, 1);
    *self = rp_initialize_cb_ctx_ctor(cm_init_helper, cm_cluster);
    return self;
}

static RpStatusCode_e
initialize_cb(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    RpInitializeCbCtx* ctx = arg;
    RpClusterManagerInitHelper* self = ctx->cm_init_helper;
    RpClusterManagerCluster* cm_cluster = ctx->cm_cluster;
    g_free(arg);
NOISY_MSG_("cm_cluster %p(%p)", cm_cluster, rp_cluster_manager_cluster_cluster(cm_cluster));
    RpStatusCode_e status = on_cluster_init(self, cm_cluster);
    if (status != RpStatusCode_Ok)
    {
        NOISY_MSG_("failed status code %d", status);
        return status;
    }
    //TODO...cm_cluster.cluster().info()->configUpdateStats().warming_state_.set(0);
    return RpStatusCode_Ok;
}

void
rp_cluster_manager_init_helper_add_cluster(RpClusterManagerInitHelper* self, RpClusterManagerCluster* cm_cluster)
{
    LOGD("(%p, %p)", self, cm_cluster);

    g_return_if_fail(RP_IS_CLUSTER_MANAGER_INIT_HELPER(self));
    g_return_if_fail(RP_IS_CLUSTER_MANAGER_CLUSTER(cm_cluster));

    g_assert(self->m_state != State_AllClustersInitialized);
    RpCluster* cluster = rp_cluster_manager_cluster_cluster(cm_cluster);
    const char* name = rp_cluster_info_name(rp_cluster_info(cluster));
NOISY_MSG_("cluster %p(%s)", cluster, name);

    //TODO...cluster.info()->configUpdateStats().warming_state_.set(1);
    if (rp_cluster_initialize_phase(cluster) == RpInitializePhase_Primary)
    {
NOISY_MSG_("replacing");
        g_hash_table_replace(self->m_primary_init_clusters, g_strdup(name), cm_cluster);
        rp_cluster_initialize(cluster, initialize_cb, rp_initialize_cb_ctx_new(self, cm_cluster));
    }

    LOGD("cm init: adding: cluster=\"%s\" primary=%u", name, g_hash_table_size(self->m_primary_init_clusters));
}

void
rp_cluster_manager_init_helper_on_static_load_complete(RpClusterManagerInitHelper* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_CLUSTER_MANAGER_INIT_HELPER(self));
    self->m_state = State_WaitingForPrimaryInitializationToComplete;
    maybe_finish_initialize(self);
}
