/*
 * rp-upstream.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <evhtp.h>
#include "rp-cluster-configuration.h"
#include "rp-codec.h"
#include "rp-host-description.h"
#include "rp-filter-factory.h"
#include "rp-net-connection.h"

G_BEGIN_DECLS

typedef struct Http1Settings_s * Http1SettingsPtr;

typedef enum {
    RpInitializationPhase_Primary,
    RpInitializationPhase_Secondary
} RpInitializationPhase_e;


/**
 * A bundle struct for address and socket options.
 */
typedef struct _RpUpstreamLocalAddress RpUpstreamLocalAddress;
struct _RpUpstreamLocalAddress {
    struct sockaddr* m_address;
//TODO...Network::Connection::OptionsSharedPtr socket_options_;
};

static inline RpUpstreamLocalAddress
rp_upstream_local_address_ctor(struct sockaddr* address)
{
    RpUpstreamLocalAddress self = {
        .m_address = address
    };
    return self;
}


/**
 * Interface to select upstream local address based on the endpoint address.
 */
#define RP_TYPE_UPSTREAM_LOCAL_ADDRESS_SELECTOR rp_upstream_local_address_selector_get_type()
G_DECLARE_INTERFACE(RpUpstreamLocalAddressSelector, rp_upstream_local_address_selector, RP, UPSTREAM_LOCAL_ADDRESS_SELECTOR, GObject)

struct _RpUpstreamLocalAddressSelectorInterface {
    GTypeInterface parent_iface;

    RpUpstreamLocalAddress (*get_upstream_local_address)(RpUpstreamLocalAddressSelector*,
                                                            struct sockaddr*);
};

static inline RpUpstreamLocalAddress
rp_upstream_local_address_selector_get_upstream_local_address(RpUpstreamLocalAddressSelector* self, struct sockaddr* endpoint_address)
{
    return RP_IS_UPSTREAM_LOCAL_ADDRESS_SELECTOR(self) ?
        RP_UPSTREAM_LOCAL_ADDRESS_SELECTOR_GET_IFACE(self)->get_upstream_local_address(self, endpoint_address) :
        rp_upstream_local_address_ctor(NULL);
}


/**
 * Information about a given upstream cluster.
 * This includes the information and interfaces for building an upstream filter chain.
 */
#define RP_TYPE_CLUSTER_INFO rp_cluster_info_get_type()
G_DECLARE_INTERFACE(RpClusterInfo, rp_cluster_info, RP, CLUSTER_INFO, RpFilterChainFactory)

struct _RpClusterInfoInterface {
    RpFilterChainFactoryInterface parent_iface;

    bool (*added_via_api)(RpClusterInfo*);
    guint32 (*connect_timeout)(RpClusterInfo*);
    guint32 (*idle_timeout)(RpClusterInfo*);
    guint32 (*tcp_pool_idle_timeout)(RpClusterInfo*);
    guint32 (*max_connection_duration)(RpClusterInfo*);
    float (*per_upstream_preconnect_ratio)(RpClusterInfo*);
    float (*peek_ahead_ratio)(RpClusterInfo*);
    guint32 (*per_connection_buffer_limit_bytes)(RpClusterInfo*);
    guint64 (*features)(RpClusterInfo*);
    Http1SettingsPtr (*http1_settings)(RpClusterInfo*);
    //TODO...
    RpDiscoveryType_e (*type)(RpClusterInfo*);
    //TODO...
    bool (*maintenance_mode)(RpClusterInfo*);
    guint64 (*max_requests_per_connection)(RpClusterInfo*);
    guint32 (*max_response_headers_count)(RpClusterInfo*);
    guint16 (*max_response_headers_kb)(RpClusterInfo*);
    const char* (*name)(RpClusterInfo*);
    const char* (*observability_name)(RpClusterInfo*);
    RpResourceManager* (*resource_manager)(RpClusterInfo*, RpResourcePriority_e);
    //TODO...
    RpUpstreamLocalAddressSelector* (*get_upstream_local_address_selector)(RpClusterInfo*);
    //TODO...
    bool (*drain_connections_on_host_removal)(RpClusterInfo*);
    bool (*connection_pool_per_downstream_connection)(RpClusterInfo*);
    bool (*warm_hosts)(RpClusterInfo*);
    bool (*set_local_interface_name_on_upstream_connection)(RpClusterInfo*);
    const char* (*eds_service_name)(RpClusterInfo*);
    void (*create_network_filter_chain)(RpClusterInfo*, RpNetworkConnection*);
    //TODO...
};

static inline float
rp_cluster_info_per_upstream_preconnect_ratio(RpClusterInfo* self)
{
    return RP_IS_CLUSTER_INFO(self) ?
        RP_CLUSTER_INFO_GET_IFACE(self)->per_upstream_preconnect_ratio(self) :
        1.0;
}
static inline RpDiscoveryType_e
rp_cluster_info_type(RpClusterInfo* self)
{
    return RP_IS_CLUSTER_INFO(self) ?
        RP_CLUSTER_INFO_GET_IFACE(self)->type(self) : RpDiscoveryType_STATIC;
}
static inline RpResourceManager*
rp_cluster_info_resource_manager(RpClusterInfo* self, RpResourcePriority_e priority)
{
    return RP_IS_CLUSTER_INFO(self) ?
        RP_CLUSTER_INFO_GET_IFACE(self)->resource_manager(self, priority) : NULL;
}
static inline RpUpstreamLocalAddressSelector*
rp_cluster_info_get_upstream_local_address_selector(RpClusterInfo* self)
{
    return RP_IS_CLUSTER_INFO(self) ?
        RP_CLUSTER_INFO_GET_IFACE(self)->get_upstream_local_address_selector(self) :
        NULL;
}
static inline void
rp_cluster_info_create_network_filter_chain(RpClusterInfo* self, RpNetworkConnection* connection)
{
    if (RP_IS_CLUSTER_INFO(self))
    {
        RP_CLUSTER_INFO_GET_IFACE(self)->create_network_filter_chain(self, connection);
    }
}
static inline const char*
rp_cluster_info_name(RpClusterInfo* self)
{
    return RP_IS_CLUSTER_INFO(self) ?
        RP_CLUSTER_INFO_GET_IFACE(self)->name(self) : NULL;
}
static inline guint64
rp_cluster_info_max_requests_per_connection(RpClusterInfo* self)
{
    return RP_IS_CLUSTER_INFO(self) ?
        RP_CLUSTER_INFO_GET_IFACE(self)->max_requests_per_connection(self) : 0;
}


typedef struct _RpCreateConnectionData RpCreateConnectionData;
typedef RpCreateConnectionData * RpCreateConnectionDataPtr;
struct _RpCreateConnectionData {
    RpNetworkClientConnection* m_connection;
    RpHostDescription* m_host_description;
};

static inline RpCreateConnectionData
rp_create_connection_data_ctor(RpNetworkClientConnection* connection, RpHostDescription* host_description)
{
    RpCreateConnectionData self = {
        .m_connection = connection,
        .m_host_description = host_description
    };
    return self;
}

/**
 * An upstream host.
 */
#define RP_TYPE_HOST rp_host_get_type()
G_DECLARE_INTERFACE(RpHost, rp_host, RP, HOST, GObject)

struct _RpHostInterface {
    GTypeInterface/*virtual RpHostDescription*/ parent_iface;

    //TODO...
    RpCreateConnectionData (*create_connection)(RpHost*,
                                                RpDispatcher*
                                                /*Network::TransportSocketOptions,*/
                                                /*Metadata*/);
    //TODO...
    guint32 (*weight)(RpHost*);
    void (*set_wieght)(RpHost*, guint32);
    bool (*used)(RpHost*);
    //TODO...
};

static inline RpCreateConnectionData
rp_host_create_connection(RpHost* self, RpDispatcher* dispatcher)
{
    return RP_IS_HOST(self) ?
        RP_HOST_GET_IFACE(self)->create_connection(self, dispatcher) :
        rp_create_connection_data_ctor(NULL, NULL);
}
static inline guint32
rp_host_weight(RpHost* self)
{
    return RP_IS_HOST(self) ? RP_HOST_GET_IFACE(self)->weight(self) : 1;
}
static inline void
rp_host_set_wieght(RpHost* self, guint32 new_weight)
{
    if (RP_IS_HOST(self))
    {
        RP_HOST_GET_IFACE(self)->set_wieght(self, new_weight);
    }
}
static inline bool
rp_host_used(RpHost* self)
{
    return RP_IS_HOST(self) ? RP_HOST_GET_IFACE(self)->used(self) : false;
}


typedef GArray* RpHostVector;
typedef GHashTable* RpHostMap;

typedef GArray* RpLocalityWeights;


/**
 * Bucket hosts by locality.
 */
#define RP_TYPE_HOSTS_PER_LOCALITY rp_hosts_per_locality_get_type()
G_DECLARE_INTERFACE(RpHostsPerLocality, rp_hosts_per_locality, RP, HOSTS_PER_LOCALITY, GObject)

struct _RpHostsPerLocalityInterface {
    GTypeInterface parent_iface;

    bool (*has_local_locality)(RpHostsPerLocality*);
    RpHostVector (*get)(RpHostsPerLocality*);
    //TODO...
};


/**
 * Base host set interface. This contains all of the endpoints for a given LocalityLbEndpoints
 * priority level.
 */
#define RP_TYPE_HOST_SET rp_host_set_get_type()
G_DECLARE_INTERFACE(RpHostSet, rp_host_set, RP, HOST_SET, GObject)

struct _RpHostSetInterface {
    GTypeInterface parent_iface;

    RpHostVector (*hosts)(RpHostSet*);
    RpHostVector (*healthy_hosts)(RpHostSet*);
    RpHostVector (*degraded_hosts)(RpHostSet*);
    RpHostVector (*excluded_hosts)(RpHostSet*);
    RpHostsPerLocality* (*hosts_per_locality)(RpHostSet*);
    RpHostsPerLocality* (*healthy_hosts_per_locality)(RpHostSet*);
    RpHostsPerLocality* (*degraded_hosts_per_locality)(RpHostSet*);
    RpHostsPerLocality* (*excluded_hosts_per_locality)(RpHostSet*);
    RpLocalityWeights (*locality_weights)(RpHostSet*);
    guint32 (*choose_healthy_locality)(RpHostSet*);
    guint32 (*choose_degraded_locality)(RpHostSet*);
    guint32 (*priority)(RpHostSet*);
    guint32 (*overprovisioning_factor)(RpHostSet*);
    bool (*weighted_priority_health)(RpHostSet*);
};


/**
 * This class contains all of the HostSets for a given cluster grouped by priority, for
 * ease of load balancing.
 */
#define RP_TYPE_PRIORITY_SET rp_priority_set_get_type()
G_DECLARE_INTERFACE(RpPrioritySet, rp_priority_set, RP, PRIORITY_SET, GObject)

struct _RpPrioritySetInterface {
    GTypeInterface parent_iface;

    //TODO...
    GArray* (*host_sets_per_priority)(RpPrioritySet*);
    RpHostMap (*cross_priority_host_map)(RpPrioritySet*);
    //TODO...
};


/**
 * An upstream cluster (group of hosts). This class is the "primary" singleton cluster used amongst
 * all forwarding threads/workers. Individual HostSets are used on the workers themselves.
 */
#define RP_TYPE_CLUSTER rp_cluster_get_type()
G_DECLARE_INTERFACE(RpCluster, rp_cluster, RP, CLUSTER, GObject)

struct _RpClusterInterface {
    GTypeInterface parent_iface;

    //TODO...
    RpClusterInfo* (*info)(RpCluster*);
    //TODO...
    void (*initialize)(RpCluster*, RpStatusCode_e (*)(RpCluster*));
    RpInitializationPhase_e (*initialize_phase)(RpCluster*);
    RpPrioritySet* (*priority_set)(RpCluster*);
};

static inline RpClusterInfo*
rp_cluster_info(RpCluster* self)
{
    return RP_IS_CLUSTER(self) ? RP_CLUSTER_GET_IFACE(self)->info(self) : NULL;
}
static inline void
rp_cluster_initialize(RpCluster* self, RpStatusCode_e (*callback)(RpCluster*))
{
    if (RP_IS_CLUSTER(self)) \
        RP_CLUSTER_GET_IFACE(self)->initialize(self, callback);
}
static inline RpInitializationPhase_e
rp_cluster_initialize_phase(RpCluster* self)
{
    return RP_IS_CLUSTER(self) ?
        RP_CLUSTER_GET_IFACE(self)->initialize_phase(self) : RpInitializationPhase_Primary;
}
static inline RpPrioritySet*
rp_cluster_priority_set(RpCluster* self)
{
    return RP_IS_CLUSTER(self) ?
        RP_CLUSTER_GET_IFACE(self)->priority_set(self) : NULL;
}

G_END_DECLS
