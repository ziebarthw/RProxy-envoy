/*
 * rp-cluster-store.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-cluster-configuration.h"
#include "rp-singleton-manager.h"
#include "rp-upstream.h"

G_BEGIN_DECLS

#define RP_TYPE_DFP_LB rp_dfp_lb_get_type()
G_DECLARE_INTERFACE(RpDfpLb, rp_dfp_lb, RP, DFP_LB, GObject)

struct _RpDfpLbInterface {
    GTypeInterface parent_iface;

    RpHostConstSharedPtr (*find_host_by_name)(RpDfpLb*, const char*);
};
static inline RpHostConstSharedPtr
rp_dfp_lb_find_host_by_name(RpDfpLb* self, const char* host)
{
    return RP_IS_DFP_LB(self) ?
        RP_DFP_LB_GET_IFACE(self)->find_host_by_name(self, host) : NULL;
}



#define RP_TYPE_DFP_CLUSTER rp_dfp_cluster_get_type()
G_DECLARE_INTERFACE(RpDfpCluster, rp_dfp_cluster, RP, DFP_CLUSTER, GObject)

typedef struct _RpDfpCreateSubClusterConfigRval RpDfpCreateSubClusterConfigRval;
struct _RpDfpCreateSubClusterConfigRval {
    RpClusterCfg config_;
    RpClusterCfg* config;
    bool success;
};
static inline RpDfpCreateSubClusterConfigRval
rp_dfp_create_sub_cluster_config_rval_ctor(const RpClusterCfg* config, bool success)
{
    RpClusterCfg empty = {0};
    RpDfpCreateSubClusterConfigRval self = {
        .config_ = config ? *config : empty,
        .success = success
    };
    self.config = config ? &self.config_ : NULL;
    return self;
}

struct _RpDfpClusterInterface {
    GTypeInterface parent_iface;

    bool (*enable_sub_cluster)(RpDfpCluster*);
    RpDfpCreateSubClusterConfigRval (*create_sub_cluster_config)(RpDfpCluster*, const char*, const char*, int);
    bool (*touch)(RpDfpCluster*, const char*);
};

typedef SHARED_PTR(RpDfpCluster) RpDfpClusterSharedPtr;
typedef WEAK_PTR(RpDfpCluster) RpDfpClusterWeakPtr;

static inline bool
rp_dfp_cluster_enable_sub_cluster(RpDfpCluster* self)
{
    return RP_IS_DFP_CLUSTER(self) ?
        RP_DFP_CLUSTER_GET_IFACE(self)->enable_sub_cluster(self) : false;
}
static inline RpDfpCreateSubClusterConfigRval
rp_dfp_cluster_create_sub_cluster_config(RpDfpCluster* self, const char* cluster_name, const char* host_name, int port)
{
    return RP_IS_DFP_CLUSTER(self) ?
        RP_DFP_CLUSTER_GET_IFACE(self)->create_sub_cluster_config(self, cluster_name, host_name, port) :
        rp_dfp_create_sub_cluster_config_rval_ctor(rp_cluster_cfg_empty(), false);
}
static inline bool
rp_dfp_cluster_touch(RpDfpCluster* self, const char* cluster_name)
{
    return RP_IS_DFP_CLUSTER(self) ?
        RP_DFP_CLUSTER_GET_IFACE(self)->touch(self, cluster_name) : false;
}


#define RP_TYPE_DFP_CLUSTER_STORE rp_dfp_cluster_store_get_type()
G_DECLARE_FINAL_TYPE(RpDfpClusterStore, rp_dfp_cluster_store, RP, DFP_CLUSTER_STORE, GObject)

typedef SHARED_PTR(RpDfpClusterStore) RpDfpClusterStoreSharedPtr;

RpDfpClusterStore* rp_dfp_cluster_store_new(void);
RpDfpClusterSharedPtr rp_dfp_cluster_store_load(RpDfpClusterStore* self, const char* cluster_name);
void rp_dfp_cluster_store_save(RpDfpClusterStore* self, const char* cluster_name, RpDfpClusterSharedPtr cluster);
void rp_dfp_cluster_store_remove(RpDfpClusterStore* self, const char* cluster_name);


#define RP_TYPE_DFP_CLUSTER_STORE_FACTORY rp_dfp_cluster_store_factory_get_type()
G_DECLARE_FINAL_TYPE(RpDfpClusterStoreFactory, rp_dfp_cluster_store_factory, RP, DFP_CLUSTER_STORE_FACTORY, GObject)

RpDfpClusterStoreFactory* rp_dfp_cluster_store_factory_new(RpSingletonManager* singleton_manager);
RpDfpClusterStoreSharedPtr rp_dfp_cluster_store_factory_get(RpDfpClusterStoreFactory* self);

G_END_DECLS
