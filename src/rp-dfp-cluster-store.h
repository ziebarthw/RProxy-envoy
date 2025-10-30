/*
 * rp-dfp-cluster-store.h
 * Copyright (C) 2025
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <glib-object.h>
#include <evhtp.h>

#include "rp-cluster-manager.h"
#include "rproxy.h"

G_BEGIN_DECLS

#define RP_TYPE_DFP_CLUSTER rp_dfp_cluster_get_type()
G_DECLARE_INTERFACE(RpDfpCluster, rp_dfp_cluster, RP, DFP_CLUSTER, GObject)

struct _RpDfpClusterInterface {
    GTypeInterface parent_iface;

    bool (*enable_sub_cluster)(RpDfpCluster*);
    RpClusterCfg (*create_sub_cluster_config)(RpDfpCluster*,
                                                const char*,
                                                const char*,
                                                int);
    bool (*touch)(RpDfpCluster*, const char*);
};

static inline bool
rp_dfp_cluster_enable_sub_cluster(RpDfpCluster* self)
{
    return RP_IS_DFP_CLUSTER(self) ?
        RP_DFP_CLUSTER_GET_IFACE(self)->enable_sub_cluster(self) : false;
}
static inline RpClusterCfg
rp_dfp_cluster_create_sub_cluster_config(RpDfpCluster* self, const char* cluster_name, const char* host, int port)
{
    return RP_DFP_CLUSTER_GET_IFACE(self)->create_sub_cluster_config(self, cluster_name, host, port);
}
static inline bool
rp_dfp_cluster_touch(RpDfpCluster* self, const char* cluster_name)
{
    return RP_IS_DFP_CLUSTER(self) ?
        RP_DFP_CLUSTER_GET_IFACE(self)->touch(self, cluster_name) : false;
}

typedef struct _RpDfpClusterStore RpDfpClusterStore;

RpDfpClusterStore* rp_dfp_cluster_store_new(RpClusterManager* cluster_manager);
void rp_dfp_cluster_store_free(RpDfpClusterStore* store);

// Thread-safe process-wide singleton accessor. The first non-NULL cluster_manager
// is captured for initialization.
RpDfpClusterStore* rp_dfp_cluster_store_get_instance(RpClusterManager* cluster_manager);
RpDfpCluster* rp_dfp_cluster_store_load(RpDfpClusterStore* self,
                                        const char* cluster_name);
void rp_dfp_cluster_store_save(RpDfpClusterStore* self,
                                const char* cluster_name,
                                RpDfpCluster* cluster);

#if 0
/**
 * Returns a cached downstream for hostname if present, otherwise creates one,
 * caches it, and returns it.
 */
downstream_t* rp_dfp_cluster_store_get_or_create_downstream(RpDfpClusterStore* store,
                                                            rproxy_t* rproxy,
                                                            const char* hostname);
#endif//0

G_END_DECLS
