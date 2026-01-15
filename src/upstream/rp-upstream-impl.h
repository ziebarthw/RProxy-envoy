/*
 * rp-upstream-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-factory-context.h"
#include "rp-local-info.h"
#include "rp-upstream.h"

G_BEGIN_DECLS

extern const guint32 kDefaultOverProvisioningFactor;


/**
 * Base implementation of most of Upstream::HostDescription, shared between
 * HostDescriptionImpl and LogicalHost, which is in
 * source/extensions/clusters/common/logical_host.h. These differ in threading.
 *
 * HostDescriptionImpl and HostImpl are intended to be initialized in the main
 * thread, and are thereafter read-only, and thus do not require locking.
 *
 * LogicalHostImpl is intended to be dynamically changed due to DNS resolution
 * and Happy Eyeballs from multiple threads, and thus requires an address_lock
 * and lock annotations to enforce this.
 *
 * The two level implementation inheritance allows most of the implementation
 * to be shared, but sinks the ones requiring different lock semantics into
 * the leaf subclasses.
 */
#define RP_TYPE_HOST_DESCRIPTION_IMPL_BASE rp_host_description_impl_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpHostDescriptionImplBase, rp_host_description_impl_base, RP, HOST_DESCRIPTION_IMPL_BASE, GObject)

struct _RpHostDescriptionImplBaseClass {
    GObjectClass parent_class;

};

RpNetworkAddressInstanceConstSharedPtr rp_host_description_impl_base_dest_address_(RpHostDescriptionImplBase* self);


/**
 * Final implementation of most of Upstream::HostDescription, providing const
 * of the address-related member variables.
 *
 * See also LogicalHostDescriptionImpl in
 * source/extensions/clusters/common/logical_host.h for a variant that allows
 * safe dynamic update to addresses.
 */
#define RP_TYPE_HOST_DESCRIPTION_IMPL rp_host_description_impl_get_type()
G_DECLARE_DERIVABLE_TYPE(RpHostDescriptionImpl, rp_host_description_impl, RP, HOST_DESCRIPTION_IMPL, RpHostDescriptionImplBase)

struct _RpHostDescriptionImplClass {
    RpHostDescriptionImplBaseClass parent_class;

};


/**
 * Implementation of Upstream::Host.
 */
#define RP_TYPE_HOST_IMPL rp_host_impl_get_type()
G_DECLARE_FINAL_TYPE(RpHostImpl, rp_host_impl, RP, HOST_IMPL, RpHostDescriptionImpl)

#if 0
RpHostImpl* rp_host_impl_new(RpClusterInfoConstSharedPtr cluster,
                                RpUpstreamTransportSocketFactory* socket_factory,
                                const char* hostname,
                                struct sockaddr* address,
                                guint32 initial_weight,
                                bool disable_health_check);
#endif//0
UNIQUE_PTR(RpHostImpl) rp_host_impl_create(RpClusterInfoConstSharedPtr cluster,
                                            const char* hostname,
                                            RpNetworkAddressInstanceConstSharedPtr address,
                                            RpMetadataConstSharedPtr endpoint_metadata,
                                            guint32 initial_weight,
                                            guint32 priority,
                                            RpTimeSource* time_source);


/**
 * A class for management of the set of hosts for a given priority level.
 */
#define RP_TYPE_HOST_SET_IMPL rp_host_set_impl_get_type()
G_DECLARE_DERIVABLE_TYPE(RpHostSetImpl, rp_host_set_impl, RP, HOST_SET_IMPL, GObject)

struct _RpHostSetImplClass {
    GObjectClass parent_class;

    RpStatusCode_e (*run_update_callbacks)(RpHostSetImpl*, RpHostVector*, RpHostVector*);
};

typedef UNIQUE_PTR(RpHostSetImpl) RpHostSetImplPtr;

static inline RpStatusCode_e
rp_host_set_impl_run_update_callbacks(RpHostSetImpl* self, RpHostVector* hosts_added, RpHostVector* hosts_removed)
{
    return RP_IS_HOST_SET_IMPL(self) ?
        RP_HOST_SET_IMPL_GET_CLASS(self)->run_update_callbacks(self, hosts_added, hosts_removed) :
        RpStatusCode_Ok;
}


/**
 * A class for management of the set of hosts in a given cluster.
 */
#define RP_TYPE_PRIORITY_SET_IMPL rp_priority_set_impl_get_type()
G_DECLARE_DERIVABLE_TYPE(RpPrioritySetImpl, rp_priority_set_impl, RP, PRIORITY_SET_IMPL, GObject)

struct _RpPrioritySetImplClass {
    GObjectClass parent_class;

    RpHostSetImplPtr (*create_host_set)(RpPrioritySetImpl*, guint32);
    RpStatusCode_e (*run_update_callbacks)(RpPrioritySetImpl*, RpHostVector*, RpHostVector*);
    RpStatusCode_e (*run_reference_update_callbacks)(RpPrioritySetImpl*, guint32, RpHostVector*, RpHostVector*);
};

static inline RpHostSetImplPtr
rp_priority_set_impl_create_host_set(RpPrioritySetImpl* self, guint32 priority)
{
    return RP_IS_PRIORITY_SET_IMPL(self) ?
        RP_PRIORITY_SET_IMPL_GET_CLASS(self)->create_host_set(self, priority) :
        NULL;
}
static inline RpStatusCode_e
rp_priority_set_impl_run_update_callbacks(RpPrioritySetImpl* self, RpHostVector* hosts_added, RpHostVector* hosts_removed)
{
    return RP_IS_PRIORITY_SET_IMPL(self) ?
        RP_PRIORITY_SET_IMPL_GET_CLASS(self)->run_update_callbacks(self, hosts_added, hosts_removed) :
        RpStatusCode_Ok;
}
static inline RpStatusCode_e
rp_priority_set_impl_run_reference_update_callbacks(RpPrioritySetImpl* self, guint32 priority, RpHostVector* hosts_added, RpHostVector* hosts_removed)
{
    return RP_IS_PRIORITY_SET_IMPL(self) ?
        RP_PRIORITY_SET_IMPL_GET_CLASS(self)->run_reference_update_callbacks(self, priority, hosts_added, hosts_removed) :
        RpStatusCode_Ok;
}

RpPrioritySetImpl* rp_priority_set_impl_new(void);
const RpHostSet* rp_priority_set_impl_get_or_create_host_set(RpPrioritySetImpl* self,
                                                                guint32 priority);
RpHostMap** rp_priority_set_impl_const_cross_priority_host_map_(RpPrioritySetImpl* self);


/**
 * Specialized PrioritySetImpl designed for the main thread. It will update and maintain the read
 * only cross priority host map when the host set changes.
 */
#define RP_TYPE_MAIN_PRIORITY_SET_IMPL rp_main_priority_set_impl_get_type()
G_DECLARE_FINAL_TYPE(RpMainPrioritySetImpl, rp_main_priority_set_impl, RP, MAIN_PRIORITY_SET_IMPL, RpPrioritySetImpl)

RpMainPrioritySetImpl* rp_main_priority_set_impl_new(void);


/**
 * Implementation of ClusterInfo that reads from JSON.
 */
#define RP_TYPE_CLUSTER_INFO_IMPL rp_cluster_info_impl_get_type()
G_DECLARE_FINAL_TYPE(RpClusterInfoImpl, rp_cluster_info_impl, RP, CLUSTER_INFO_IMPL, GObject)

RpClusterInfoImpl* rp_cluster_info_impl_new(RpServerFactoryContext* server_context, RpClusterCfg* config, bool added_via_api);


/**
 * Base class all primary clusters.
 */
#define RP_TYPE_CLUSTER_IMPL_BASE rp_cluster_impl_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpClusterImplBase, rp_cluster_impl_base, RP, CLUSTER_IMPL_BASE, GObject)

struct _RpClusterImplBaseClass {
    GObjectClass parent_class;

    void (*start_pre_init)(RpClusterImplBase*);
};

typedef SHARED_PTR(RpClusterImplBase) RpClusterImplBaseSharedPtr;

void rp_cluster_impl_base_on_pre_init_complete(RpClusterImplBase* self);
const RpClusterCfg* rp_cluster_impl_base_config_(RpClusterImplBase* self);


/**
 * Manages PriorityState of a cluster. PriorityState is a per-priority binding of a set of hosts
 * with its corresponding locality weight map. This is useful to store priorities/hosts/localities
 * before updating the cluster priority set.
 */
#define RP_TYPE_PRIORITY_STATE_MANAGER rp_priority_state_manager_get_type()
G_DECLARE_FINAL_TYPE(RpPriorityStateManager, rp_priority_state_manager, RP, PRIORITY_STATE_MANAGER, GObject)

typedef UNIQUE_PTR(RpPriorityStateManager) RpPriorityStateManagerPtr;

RpPriorityStateManager* rp_priority_state_manager_new(RpClusterImplBase* cluster, const RpLocalInfo* local_info/*,TODO...HostUpdateCb update_cb*/);
void rp_priority_state_manager_initialize_priority_for(RpPriorityStateManager* self, const RpLocalityLbEndpointsCfg* locality_lb_endpoint);
void rp_priority_state_manager_register_host_for_priority(RpPriorityStateManager* self, const char* hostname, RpNetworkAddressInstanceConstSharedPtr address/*,std::vector<Network::Address::InstanceConstSharedPtr> address_list*/, const RpLocalityLbEndpointsCfg* locality_lb_endpoint, const RpLbEndpointCfg* lb_endpoint, RpTimeSource* time_source);
RpPriorityState rp_priority_state_manager_priority_state(RpPriorityStateManager* self);

G_END_DECLS
