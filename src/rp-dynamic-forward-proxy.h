/*
 * rp-dynamic-forward-proxy.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-cluster-store.h"
#include "rp-factory-context.h"
#include "rp-pass-through-filter.h"
#include "rp-server-filter-config.h"
#include "rp-thread-local.h"

G_BEGIN_DECLS

/**
 * Handle returned from addDynamicCluster(). Destruction of the handle will cancel any future
 * callback.
 */
#define RP_TYPE_LOAD_CLUSTER_ENTRY_HANDLE rp_load_cluster_entry_handle_get_type()
G_DECLARE_INTERFACE(RpLoadClusterEntryHandle, rp_load_cluster_entry_handle, RP, LOAD_CLUSTER_ENTRY_HANDLE, GObject)

typedef UNIQUE_PTR(RpLoadClusterEntryHandle) RpLoadClusterEntryHandlePtr;

struct _RpLoadClusterEntryHandleInterface {
    GTypeInterface parent_iface;

};


/**
 * RpLoadClusterEtryCallbacks
 */
#define RP_TYPE_LOAD_CLUSTER_ENTRY_CALLBACKS rp_load_cluster_entry_callbacks_get_type()
G_DECLARE_INTERFACE(RpLoadClusterEntryCallbacks, rp_load_cluster_entry_callbacks, RP, LOAD_CLUSTER_ENTRY_CALLBACKS, GObject)

struct _RpLoadClusterEntryCallbacksInterface {
    GTypeInterface parent_iface;

    void (*on_load_cluster_complete)(RpLoadClusterEntryCallbacks*);
};

static inline void
rp_load_cluster_entry_callbacks_on_load_cluster_complete(RpLoadClusterEntryCallbacks* self)
{
    if (RP_IS_LOAD_CLUSTER_ENTRY_CALLBACKS(self)) \
        RP_LOAD_CLUSTER_ENTRY_CALLBACKS_GET_IFACE(self)->on_load_cluster_complete(self);
}


/**
 * RpDynamicForwardProxy
 */
#define RP_TYPE_DYNAMIC_FORWARD_PROXY rp_dynamic_forward_proxy_get_type()
G_DECLARE_FINAL_TYPE(RpDynamicForwardProxy, rp_dynamic_forward_proxy, RP, DYNAMIC_FORWARD_PROXY, RpPassThroughFilter)

RpFilterFactoryCb* rp_dynamic_forward_proxy_create_filter_factory(RpFactoryContext* context);


/**
 * RpProxyFilterConfig
 */
#define RP_TYPE_PROXY_FILTER_CONFIG rp_proxy_filter_config_get_type()
G_DECLARE_FINAL_TYPE(RpProxyFilterConfig, rp_proxy_filter_config, RP, PROXY_FILTER_CONFIG, GObject)

typedef SHARED_PTR(RpProxyFilterConfig) RpProxyFilterConfigSharedPtr;

RpProxyFilterConfigSharedPtr rp_proxy_filter_config_new(RpDfpClusterStoreFactory* cluster_store_factory,
                                                        RpFactoryContext* context);
RpDfpClusterStoreSharedPtr rp_proxy_filter_config_cluster_store(RpProxyFilterConfig* self);
RpClusterManager* rp_proxy_filter_config_cluster_manager(RpProxyFilterConfig* self);
bool rp_proxy_filter_config_save_upstream_address(RpProxyFilterConfig* self);
RpLoadClusterEntryHandlePtr rp_proxy_filter_config_add_dynamic_cluster(RpProxyFilterConfig* self,
                                                                        RpDfpCluster* cluster,
                                                                        const char* cluster_name,
                                                                        const char* host,
                                                                        int port,
                                                                        RpLoadClusterEntryCallbacks* callback);


/**
 * RpLoadClusterEntryHandleImpl
 */
#define RP_TYPE_LOAD_CLUSTER_ENTRY_HANDLE_IMPL rp_load_cluster_entry_handle_impl_get_type()
G_DECLARE_FINAL_TYPE(RpLoadClusterEntryHandleImpl, rp_load_cluster_entry_handle_impl, RP, LOAD_CLUSTER_ENTRY_HANDLE_IMPL, GObject)

RpLoadClusterEntryHandleImpl* rp_load_cluster_entry_handle_impl_new(GHashTable* parent, const char* host, RpLoadClusterEntryCallbacks* callbacks);
RpLoadClusterEntryCallbacks* rp_load_cluster_entry_handle_impl_callbacks_(RpLoadClusterEntryHandleImpl* self);

/**
 * RpThreadLocalClusterInfo
 */
#define RP_TYPE_THREAD_LOCAL_CLUSTER_INFO rp_thread_local_cluster_info_get_type()
G_DECLARE_FINAL_TYPE(RpThreadLocalClusterInfo, rp_thread_local_cluster_info, RP, THREAD_LOCAL_CLUSTER_INFO, GObject)

RpThreadLocalClusterInfo* rp_thread_local_cluster_info_new(RpProxyFilterConfig* parent);
GHashTable* rp_thread_local_cluster_info_pending_clusters_(RpThreadLocalClusterInfo* self);

G_END_DECLS
