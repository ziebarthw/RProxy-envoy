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
#include "rp-callback.h"
#include "rp-cluster-configuration.h"
#include "rp-codec.h"
#include "rp-host-description.h"
#include "rp-filter-factory.h"
#include "rp-net-connection.h"

G_BEGIN_DECLS

typedef struct _RpLoadBalancerConfig RpLoadBalancerConfig;
typedef struct _RpTypedLoadBalancerFactory RpTypedLoadBalancerFactory;
typedef struct _RpTransportSocketFactoryContext RpTransportSocketFactoryContext;
typedef SHARED_PTR(RpTransportSocketFactoryContext) RpTransportSocketFactoryContextPtr;

typedef struct Http1Settings_s * Http1SettingsPtr;

typedef enum {
    RpInitializePhase_Primary,
    RpInitializePhase_Secondary
} RpInitializePhase_e;


typedef enum {
    RpHostHealth_UNHEALTHY,
    RpHostHealth_DEGRADED,
    RpHostHealth_HEALTHY
} RpHostHealth_e;


/**
 * A bundle struct for address and socket options.
 */
typedef struct _RpUpstreamLocalAddress RpUpstreamLocalAddress;
struct _RpUpstreamLocalAddress {
    RpNetworkAddressInstanceConstSharedPtr m_address;
//TODO...Network::Connection::OptionsSharedPtr socket_options_;
};

static inline RpUpstreamLocalAddress
rp_upstream_local_address_ctor(RpNetworkAddressInstanceConstSharedPtr address)
{
    RpUpstreamLocalAddress self = {
        .m_address = address
    };
    return self;
}


typedef struct _RpCreateConnectionData RpCreateConnectionData;
typedef RpCreateConnectionData * RpCreateConnectionDataPtr;
struct _RpCreateConnectionData {
    RpNetworkClientConnection* m_connection;
    RpHostDescriptionConstSharedPtr m_host_description;
};

static inline RpCreateConnectionData
rp_create_connection_data_ctor(RpNetworkClientConnection* connection, RpHostDescriptionConstSharedPtr host_description)
{
    RpCreateConnectionData self = {
        .m_connection = connection,
        .m_host_description = host_description
    };
    return self;
}


  // We use an X-macro here to make it easier to verify that all the enum values are accounted for.
  // clang-format off
#define HEALTH_FLAG_ENUM_VALUES(m)                                               \
  /* The host is currently failing active health checks. */                      \
  m(FAILED_ACTIVE_HC, 0x1)                                                       \
  /* The host is currently considered an outlier and has been ejected. */        \
  m(FAILED_OUTLIER_CHECK, 0x02)                                                  \
  /* The host is currently marked as unhealthy by EDS. */                        \
  m(FAILED_EDS_HEALTH, 0x04)                                                     \
  /* The host is currently marked as degraded through active health checking. */ \
  m(DEGRADED_ACTIVE_HC, 0x08)                                                    \
  /* The host is currently marked as degraded by EDS. */                         \
  m(DEGRADED_EDS_HEALTH, 0x10)                                                   \
  /* The host is pending removal from discovery but is stabilized due to */      \
  /* active HC. */                                                               \
  m(PENDING_DYNAMIC_REMOVAL, 0x20)                                               \
  /* The host is pending its initial active health check. */                     \
  m(PENDING_ACTIVE_HC, 0x40)                                                     \
  /* The host should be excluded from panic, spillover, etc. calculations */     \
  /* because it was explicitly taken out of rotation via protocol signal and */  \
  /* is not meant to be routed to. */                                            \
  m(EXCLUDED_VIA_IMMEDIATE_HC_FAIL, 0x80)                                        \
  /* The host failed active HC due to timeout. */                                \
  m(ACTIVE_HC_TIMEOUT, 0x100)                                                    \
  /* The host is currently marked as draining by EDS */                          \
  m(EDS_STATUS_DRAINING, 0x200)
  // clang-format on

#define DECLARE_ENUM(name, value) name = value,

  typedef enum { HEALTH_FLAG_ENUM_VALUES(DECLARE_ENUM) } RpHostHealthFlag_e;

#undef DECLARE_ENUM

/**
 * An upstream host.
 */
#define RP_TYPE_HOST rp_host_get_type()
G_DECLARE_INTERFACE(RpHost, rp_host, RP, HOST, GObject)

struct _RpHostInterface {
    GTypeInterface/*virtual RpHostDescription*/ parent_iface;

    //TODO...
    RpCreateConnectionData (*create_connection)(const RpHost*,
                                                RpDispatcher*
                                                /*Network::TransportSocketOptions,*/
                                                /*Metadata*/);
    //TODO...
    void (*health_flag_clear)(RpHost*, RpHostHealthFlag_e);
    bool (*health_flag_get)(const RpHost*, RpHostHealthFlag_e);
    void (*health_flag_set)(RpHost*, RpHostHealth_e);
    guint32 (*health_flags_get_all)(const RpHost*);
    guint32 (*health_flags_set_all)(RpHost*, guint32);
    RpHostHealth_e (*coarse_health)(const RpHost*);
    guint32 (*weight)(const RpHost*);
    void (*set_wieght)(RpHost*, guint32);
    bool (*used)(const RpHost*);
    //TODO...
};

typedef RpHost RpHost;     // GObject type.
typedef RpHost* RpHostPtr; // Plain pointer (borrowed unless documented otherwise).

static inline gboolean
rp_host_is_a(const RpHost* self)
{
    g_return_val_if_fail(self != NULL, FALSE);
    return RP_IS_HOST((GObject*)self);
}
static inline RpHostInterface*
rp_host_iface(const RpHost* self)
{
    g_return_val_if_fail(rp_host_is_a(self), NULL);
    return RP_HOST_GET_IFACE((GObject*)self);
}
static inline void
rp_host_set_object(RpHost** dst, const RpHost* src)
{
    g_return_if_fail(dst != NULL);
    g_set_object((GObject**)dst, (GObject*)src);
}
static inline RpCreateConnectionData
rp_host_create_connection(const RpHost* self, RpDispatcher* dispatcher)
{
    g_return_val_if_fail(rp_host_is_a(self), rp_create_connection_data_ctor(NULL, NULL));
    RpHostInterface* iface = rp_host_iface(self);
    return iface->create_connection(self, dispatcher);
}
static inline void
rp_host_health_flag_clear(RpHost* self, RpHostHealthFlag_e flag)
{
    g_return_if_fail(rp_host_is_a(self));
    RpHostInterface* iface = rp_host_iface(self);
    iface->health_flag_clear(self, flag);
}
static inline bool
rp_host_health_flag_get(const RpHost* self, RpHostHealthFlag_e flag)
{
    g_return_val_if_fail(rp_host_is_a(self), false);
    RpHostInterface* iface = rp_host_iface(self);
    return iface->health_flag_get(self, flag);
}
static inline void
rp_host_health_flag_set(RpHost* self, RpHostHealth_e flag)
{
    g_return_if_fail(rp_host_is_a(self));
    RpHostInterface* iface = rp_host_iface(self);
    iface->health_flag_set(self, flag);
}
static inline guint32
rp_host_health_flags_get_all(const RpHost* self)
{
    g_return_val_if_fail(rp_host_is_a(self), 0);
    RpHostInterface* iface = rp_host_iface(self);
    return iface->health_flags_get_all(self);
}
static inline guint32
rp_host_health_flags_set_all(RpHost* self, guint32 bits)
{
    g_return_val_if_fail(rp_host_is_a(self), 0);
    RpHostInterface* iface = rp_host_iface(self);
    return iface->health_flags_set_all(self, bits);
}
static inline RpHostHealth_e
rp_host_coarse_health(const RpHost* self)
{
    g_return_val_if_fail(rp_host_is_a(self), RpHostHealth_UNHEALTHY);
    RpHostInterface* iface = rp_host_iface(self);
    return iface->coarse_health(self);
}
static inline guint32
rp_host_weight(const RpHost* self)
{
    g_return_val_if_fail(rp_host_is_a(self), 1);
    RpHostInterface* iface = rp_host_iface(self);
    return iface->weight(self);
}
static inline void
rp_host_set_wieght(RpHost* self, guint32 new_weight)
{
    g_return_if_fail(rp_host_is_a(self));
    RpHostInterface* iface = rp_host_iface(self);
    iface->set_wieght(self, new_weight);
}
static inline bool
rp_host_used(const RpHost* self)
{
    g_return_val_if_fail(rp_host_is_a(self), false);
    RpHostInterface* iface = rp_host_iface(self);
    return iface->used(self);
}


/**
 * RpHostVector
 */
typedef struct _RpHostVector RpHostVector;

RpHostVector* rp_host_vector_new(void);
RpHostVector* rp_host_vector_copy(const RpHostVector* src);
void rp_host_vector_add(RpHostVector* self, RpHost* host);         /* refs host */
void rp_host_vector_add_take(RpHostVector* self, RpHost* host);    /* takes ownership */
RpHostVector* rp_host_vector_ref(const RpHostVector* self);
void rp_host_vector_unref(const RpHostVector* self);
gint rp_host_vector_ref_count(const RpHostVector* self);
bool rp_host_vector_is_empty(const RpHostVector* self);
RpHost* rp_host_vector_get(const RpHostVector* self, guint index); /* transfer none */
RpHost* rp_host_vector_dup(const RpHostVector* self, guint index); /* transfer full */
guint rp_host_vector_len(const RpHostVector* self);


typedef UNIQUE_PTR(RpHostVector) RpHostListPtr;      // std::unique_ptr<HostVector>
typedef UNIQUE_PTR(GHashTable) RpLocalityWeightsMap; // absl::node_hash_map<Locality, uint32_t, LocalityHash, LocalityEqualTo>
// std::pair<HostListsPtr, LocalityWeightsMap>
//RP_DEFINE_PAIR_STRICT(PairHostListsPtrLocalityWeightsMap, RpHostListPtr, RpLocalityWeightsMap);
//RP_DECLARE_PAIR_CTOR(PairHostListsPtrLocalityWeightsMap, RpHostListPtr, RpLocalityWeightsMap);

typedef struct _RpPriorityState RpPriorityState;       // std::vector<std::pair<HostListPtr, LocalityWeightsMap>>
typedef UNIQUE_PTR(RpPriorityState) RpPriorityStatePtr;

RpPriorityStatePtr rp_priority_state_new(void);
guint rp_priority_state_size(const RpPriorityStatePtr self);
void rp_priority_state_ensure(RpPriorityStatePtr self, guint32 priority);
void rp_priority_state_set_host_list(RpPriorityStatePtr self,
                                        guint32 priority,
                                        RpHostListPtr host_list/*consumes*/);
RpHostListPtr rp_priority_state_steal_host_list(RpPriorityStatePtr self,
                                                guint32 priority);
void rp_priority_state_unref(RpPriorityStatePtr self);
RpHostListPtr rp_priority_state_peek_host_list(RpPriorityStatePtr self,
                                                guint32 priority);


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

    bool (*added_via_api)(const RpClusterInfo*);
    guint32 (*connect_timeout)(const RpClusterInfo*);
    guint32 (*idle_timeout)(const RpClusterInfo*);
    guint32 (*tcp_pool_idle_timeout)(const RpClusterInfo*);
    guint32 (*max_connection_duration)(const RpClusterInfo*);
    float (*per_upstream_preconnect_ratio)(const RpClusterInfo*);
    float (*peek_ahead_ratio)(const RpClusterInfo*);
    guint32 (*per_connection_buffer_limit_bytes)(const RpClusterInfo*);
    guint64 (*features)(const RpClusterInfo*);
    Http1SettingsPtr (*http1_settings)(const RpClusterInfo*);
    //TODO...
    RpLoadBalancerConfig* (*load_balancer_config)(const RpClusterInfo*);
    RpTypedLoadBalancerFactory* (*load_balancer_factory)(const RpClusterInfo*);
    RpDiscoveryType_e (*type)(const RpClusterInfo*);
    const RpCustomClusterTypeCfg* (*cluster_type)(const RpClusterInfo*);
    //TODO...
    bool (*maintenance_mode)(const RpClusterInfo*);
    guint64 (*max_requests_per_connection)(const RpClusterInfo*);
    guint32 (*max_response_headers_count)(const RpClusterInfo*);
    guint16 (*max_response_headers_kb)(const RpClusterInfo*);
    const char* (*name)(const RpClusterInfo*);
    const char* (*observability_name)(const RpClusterInfo*);
    RpResourceManager* (*resource_manager)(const RpClusterInfo*, RpResourcePriority_e);
    RpTransportSocketFactoryContextPtr (*transport_socket_context)(const RpClusterInfo*);
    //TODO...
    RpUpstreamLocalAddressSelector* (*get_upstream_local_address_selector)(const RpClusterInfo*);
    //TODO...
    bool (*drain_connections_on_host_removal)(const RpClusterInfo*);
    bool (*connection_pool_per_downstream_connection)(const RpClusterInfo*);
    bool (*warm_hosts)(const RpClusterInfo*);
    bool (*set_local_interface_name_on_upstream_connection)(const RpClusterInfo*);
    const char* (*eds_service_name)(const RpClusterInfo*);
    void (*create_network_filter_chain)(const RpClusterInfo*, RpNetworkConnection*);
    evhtp_proto* (*upstream_http_protocol)(const RpClusterInfo*, evhtp_proto);
    //TODO...
};

typedef const SHARED_PTR(RpClusterInfo) RpClusterInfoConstSharedPtr;
typedef SHARED_PTR(RpClusterInfo) RpClusterInfoSharedPtr;

static inline void
rp_cluster_info_set_object(RpClusterInfoSharedPtr* dst, RpClusterInfoConstSharedPtr src)
{
    g_set_object((GObject**)dst, (GObject*)src);
}
static inline RpClusterInfoInterface*
rp_cluster_info_iface(RpClusterInfoConstSharedPtr self)
{
    return RP_CLUSTER_INFO_GET_IFACE((RpClusterInfo*)self);
}
static inline gboolean
rp_cluster_info_is_cluster_info(RpClusterInfoConstSharedPtr self)
{
    return self && RP_IS_CLUSTER_INFO((RpClusterInfo*)self);
}
static inline float
rp_cluster_info_per_upstream_preconnect_ratio(RpClusterInfoConstSharedPtr self)
{
    g_return_val_if_fail(rp_cluster_info_is_cluster_info(self), 1.0);
    RpClusterInfoInterface* iface = rp_cluster_info_iface(self);
    g_return_val_if_fail(iface->per_upstream_preconnect_ratio != NULL, 1.0);
    return iface->per_upstream_preconnect_ratio(self);
}
static inline RpLoadBalancerConfig*
rp_cluster_info_load_balancer_config(RpClusterInfoConstSharedPtr self)
{
    g_return_val_if_fail(rp_cluster_info_is_cluster_info(self), NULL);
    RpClusterInfoInterface* iface = rp_cluster_info_iface(self);
    g_return_val_if_fail(iface->load_balancer_config != NULL, NULL);
    return iface->load_balancer_config(self);
}
static inline RpTypedLoadBalancerFactory*
rp_cluster_info_load_balancer_factory(RpClusterInfoConstSharedPtr self)
{
    g_return_val_if_fail(rp_cluster_info_is_cluster_info(self), NULL);
    RpClusterInfoInterface* iface = rp_cluster_info_iface(self);
    g_return_val_if_fail(iface->load_balancer_factory != NULL, NULL);
    return iface->load_balancer_factory(self);
}
static inline RpDiscoveryType_e
rp_cluster_info_type(RpClusterInfoConstSharedPtr self)
{
    g_return_val_if_fail(rp_cluster_info_is_cluster_info(self), RpDiscoveryType_STATIC);
    RpClusterInfoInterface* iface = rp_cluster_info_iface(self);
    g_return_val_if_fail(iface->type != NULL, RpDiscoveryType_STATIC);
    return iface->type(self);
}
static inline const RpCustomClusterTypeCfg*
rp_cluster_info_cluster_type(RpClusterInfoConstSharedPtr self)
{
    g_return_val_if_fail(rp_cluster_info_is_cluster_info(self), NULL);
    RpClusterInfoInterface* iface = rp_cluster_info_iface(self);
    g_return_val_if_fail(iface->cluster_type != NULL, NULL);
    return iface->cluster_type(self);
}
static inline RpResourceManager*
rp_cluster_info_resource_manager(RpClusterInfoConstSharedPtr self, RpResourcePriority_e priority)
{
    g_return_val_if_fail(rp_cluster_info_is_cluster_info(self), NULL);
    RpClusterInfoInterface* iface = rp_cluster_info_iface(self);
    g_return_val_if_fail(iface->resource_manager != NULL, NULL);
    return iface->resource_manager(self, priority);
}
static inline RpTransportSocketFactoryContextPtr
rp_cluster_info_transport_socket_context(RpClusterInfoConstSharedPtr self)
{
    g_return_val_if_fail(rp_cluster_info_is_cluster_info(self), NULL);
    RpClusterInfoInterface* iface = rp_cluster_info_iface(self);
    g_return_val_if_fail(iface->transport_socket_context != NULL, NULL);
    return iface->transport_socket_context(self);
}
static inline RpUpstreamLocalAddressSelector*
rp_cluster_info_get_upstream_local_address_selector(RpClusterInfoConstSharedPtr self)
{
    g_return_val_if_fail(rp_cluster_info_is_cluster_info(self), NULL);
    RpClusterInfoInterface* iface = rp_cluster_info_iface(self);
    g_return_val_if_fail(iface->get_upstream_local_address_selector != NULL, NULL);
    return iface->get_upstream_local_address_selector(self);
}
static inline void
rp_cluster_info_create_network_filter_chain(RpClusterInfoConstSharedPtr self, RpNetworkConnection* connection)
{
    g_return_if_fail(rp_cluster_info_is_cluster_info(self));
    RpClusterInfoInterface* iface = rp_cluster_info_iface(self);
    g_return_if_fail(iface->create_network_filter_chain != NULL);
    iface->create_network_filter_chain(self, connection);
}
static inline const char*
rp_cluster_info_name(RpClusterInfoConstSharedPtr self)
{
    g_return_val_if_fail(rp_cluster_info_is_cluster_info(self), NULL);
    RpClusterInfoInterface* iface = rp_cluster_info_iface(self);
    g_return_val_if_fail(iface->name != NULL, NULL);
    return iface->name(self);
}
static inline guint64
rp_cluster_info_max_requests_per_connection(RpClusterInfoConstSharedPtr self)
{
    g_return_val_if_fail(rp_cluster_info_is_cluster_info(self), 1024);
    RpClusterInfoInterface* iface = rp_cluster_info_iface(self);
    g_return_val_if_fail(iface->max_requests_per_connection != NULL, 1024);
    return iface->max_requests_per_connection(self);
}
static inline evhtp_proto*
rp_cluster_info_upstream_http_protocol(RpClusterInfoConstSharedPtr self, evhtp_proto downstream_protocol)
{
    g_return_val_if_fail(rp_cluster_info_is_cluster_info(self), NULL);
    RpClusterInfoInterface* iface = rp_cluster_info_iface(self);
    g_return_val_if_fail(iface->upstream_http_protocol != NULL, NULL);
    return iface->upstream_http_protocol(self, downstream_protocol);
}


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
 * Base host set interface. This contains all of the endpoints for a given
 * LocalityLbEndpoints priority level.
 */
#define RP_TYPE_HOST_SET rp_host_set_get_type()
G_DECLARE_INTERFACE(RpHostSet, rp_host_set, RP, HOST_SET, GObject)

/*
 * NOTE: Ownership conventions (GLib-style):
 *  - get_*(): returns a borrowed pointer; do NOT unref/free; lifetime tied to self.
 *  - ref_*(): returns a new reference; caller must rp_host_vector_unref().
 *
 * Envoy mapping:
 *  - HostSet::hosts()                -> get_hosts()
 *  - HostSet::hostsPtr()             -> ref_hosts()
 *  - ... similarly for healthy/degraded/excluded.
 */

typedef GPtrArray RpLocalityWeights;

/* If locality weights ever need ownership semantics, consider wrapping in a
 * refcounted type similar to RpHostVector. For now, treat as borrowed. */
typedef const RpLocalityWeights* RpLocalityWeightsPtr; /* borrowed */

struct _RpHostSetInterface {
    GTypeInterface parent_iface;

    /* Borrowed vectors (Envoy: hosts(), healthyHosts(), ...) */
    const RpHostVector* (*get_hosts)(RpHostSet* self);
    const RpHostVector* (*get_healthy_hosts)(RpHostSet* self);
    const RpHostVector* (*get_degraded_hosts)(RpHostSet* self);
    const RpHostVector* (*get_excluded_hosts)(RpHostSet* self);

    /* Owned (new ref) vectors (Envoy: hostsPtr(), healthyHostsPtr(), ...) */
    RpHostVector* (*ref_hosts)(RpHostSet* self);
    RpHostVector* (*ref_healthy_hosts)(RpHostSet* self);
    RpHostVector* (*ref_degraded_hosts)(RpHostSet* self);
    RpHostVector* (*ref_excluded_hosts)(RpHostSet* self);

    /* Per-locality containers (pick borrowed vs ref semantics consistently) */
    RpHostsPerLocality* (*get_hosts_per_locality)(RpHostSet* self);
    RpHostsPerLocality* (*get_healthy_hosts_per_locality)(RpHostSet* self);
    RpHostsPerLocality* (*get_degraded_hosts_per_locality)(RpHostSet* self);
    RpHostsPerLocality* (*get_excluded_hosts_per_locality)(RpHostSet* self);

    /* Locality weights (currently borrowed) */
    RpLocalityWeightsPtr (*get_locality_weights)(RpHostSet* self);

    /* Selection helpers */
    guint32 (*choose_healthy_locality)(RpHostSet* self);
    guint32 (*choose_degraded_locality)(RpHostSet* self);

    /* Scalar properties */
    guint32 (*get_priority)(RpHostSet* self);
    guint32 (*get_overprovisioning_factor)(RpHostSet* self);
    bool    (*get_weighted_priority_health)(RpHostSet* self);
};

/* ---- Borrowed accessors ---- */

static inline const RpHostVector*
rp_host_set_get_hosts(RpHostSet* self)
{
    return RP_IS_HOST_SET(self) ? RP_HOST_SET_GET_IFACE(self)->get_hosts(self) : NULL;
}

static inline const RpHostVector*
rp_host_set_get_healthy_hosts(RpHostSet* self)
{
    return RP_IS_HOST_SET(self) ? RP_HOST_SET_GET_IFACE(self)->get_healthy_hosts(self) : NULL;
}

static inline const RpHostVector*
rp_host_set_get_degraded_hosts(RpHostSet* self)
{
    return RP_IS_HOST_SET(self) ? RP_HOST_SET_GET_IFACE(self)->get_degraded_hosts(self) : NULL;
}

static inline const RpHostVector*
rp_host_set_get_excluded_hosts(RpHostSet* self)
{
    return RP_IS_HOST_SET(self) ? RP_HOST_SET_GET_IFACE(self)->get_excluded_hosts(self) : NULL;
}

/* ---- Owned (new ref) accessors ---- */

static inline RpHostVector*
rp_host_set_ref_hosts(RpHostSet* self)
{
    return RP_IS_HOST_SET(self) ? RP_HOST_SET_GET_IFACE(self)->ref_hosts(self) : NULL;
}

static inline RpHostVector*
rp_host_set_ref_healthy_hosts(RpHostSet* self)
{
    return RP_IS_HOST_SET(self) ? RP_HOST_SET_GET_IFACE(self)->ref_healthy_hosts(self) : NULL;
}

static inline RpHostVector*
rp_host_set_ref_degraded_hosts(RpHostSet* self)
{
    return RP_IS_HOST_SET(self) ? RP_HOST_SET_GET_IFACE(self)->ref_degraded_hosts(self) : NULL;
}

static inline RpHostVector*
rp_host_set_ref_excluded_hosts(RpHostSet* self)
{
    return RP_IS_HOST_SET(self) ? RP_HOST_SET_GET_IFACE(self)->ref_excluded_hosts(self) : NULL;
}

/* ---- Locality/per-locality helpers ---- */

static inline RpHostsPerLocality*
rp_host_set_get_hosts_per_locality(RpHostSet* self)
{
    return RP_IS_HOST_SET(self) ? RP_HOST_SET_GET_IFACE(self)->get_hosts_per_locality(self) : NULL;
}

static inline RpHostsPerLocality*
rp_host_set_get_healthy_hosts_per_locality(RpHostSet* self)
{
    return RP_IS_HOST_SET(self) ? RP_HOST_SET_GET_IFACE(self)->get_healthy_hosts_per_locality(self) : NULL;
}

static inline RpHostsPerLocality*
rp_host_set_get_degraded_hosts_per_locality(RpHostSet* self)
{
    return RP_IS_HOST_SET(self) ? RP_HOST_SET_GET_IFACE(self)->get_degraded_hosts_per_locality(self) : NULL;
}

static inline RpHostsPerLocality*
rp_host_set_get_excluded_hosts_per_locality(RpHostSet* self)
{
    return RP_IS_HOST_SET(self) ? RP_HOST_SET_GET_IFACE(self)->get_excluded_hosts_per_locality(self) : NULL;
}

static inline RpLocalityWeightsPtr
rp_host_set_get_locality_weights(RpHostSet* self)
{
    return RP_IS_HOST_SET(self) ? RP_HOST_SET_GET_IFACE(self)->get_locality_weights(self) : NULL;
}

/* ---- Scalar properties ---- */

static inline guint32
rp_host_set_get_priority(RpHostSet* self)
{
    return RP_IS_HOST_SET(self) ? RP_HOST_SET_GET_IFACE(self)->get_priority(self)
                                : RpResourcePriority_Default;
}

static inline guint32
rp_host_set_get_overprovisioning_factor(RpHostSet* self)
{
    return RP_IS_HOST_SET(self) ? RP_HOST_SET_GET_IFACE(self)->get_overprovisioning_factor(self)
                                : 0;
}

static inline bool
rp_host_set_get_weighted_priority_health(RpHostSet* self)
{
    return RP_IS_HOST_SET(self) ? RP_HOST_SET_GET_IFACE(self)->get_weighted_priority_health(self)
                                : false;
}


/**
 * This class contains all of the HostSets for a given cluster grouped by priority, for
 * ease of load balancing.
 */
#define RP_TYPE_PRIORITY_SET rp_priority_set_get_type()
G_DECLARE_INTERFACE(RpPrioritySet, rp_priority_set, RP, PRIORITY_SET, GObject)

typedef RpStatusCode_e (*RpPrioritySetMemberUpdateCb)(const RpHostVector*, const RpHostVector*, gpointer);
typedef RpStatusCode_e (*RpPrioritySetPriorityUpdateCb)(guint32, const RpHostVector*, const RpHostVector*, gpointer);

typedef UNIQUE_PTR(RpHostSet) RpHostSetPtr;
typedef struct _RpHostSetPtrVector RpHostSetPtrVector;

typedef struct _RpHostMapSnap RpHostMapSnap;
typedef UNIQUE_PTR(RpHostMapSnap) RpHostMapSnapPtr;
typedef SHARED_PTR(RpHostMapSnap) RpHostMapSnapSharedPtr;


/**
 * RpHostMap
 */
typedef struct _RpHostMap RpHostMap;

RpHostMap* rp_host_map_clone_from_snapshot(const RpHostMapSnap* src);
RpHostMap* rp_host_map_new(void);
RpHostMap* rp_host_map_ref(RpHostMap* self);
void rp_host_map_unref(const RpHostMap* self);
guint rp_host_map_ref_count(const RpHostMap* self);
void rp_host_map_add(RpHostMap* self, RpHost* host);           /* refs host */
void rp_host_map_add_take(RpHostMap* self, RpHost* host);      /* takes ownership */
RpHost* rp_host_map_find(const RpHostMap* self, RpHost* host); /* transfer none */
gboolean rp_host_map_remove(RpHostMap* self, RpHost* host);
bool rp_host_map_is_empty(const RpHostMap* self);
const GHashTable* rp_host_map_hash_table(const RpHostMap* self);
guint rp_host_map_size(const RpHostMap* self);

/**
 * RpPrioritySetUpdateHostsParams - PrioritySet::UpdateHostsParams
 */
typedef struct _RpPrioritySetUpdateHostsParams RpPrioritySetUpdateHostsParams;
struct _RpPrioritySetUpdateHostsParams {
    RpHostVector* hosts;
    RpHostVector* healthy_hosts;
    RpHostVector* degraded_hosts;
    RpHostVector* excluded_hosts;

#if WITH_HOSTS_PER_LOCALITY
    RpHostsPerLocality* hosts_per_locality;          // Transfer full (later).
    RpHostsPerLocality* healthy_hosts_per_locality;  // Transfer full.
    RpHostsPerLocality* degraded_hosts_per_locality; // Transfer full.
    RpHostsPerLocality* excluded_hosts_per_locality; // Transfer full.
#endif//WITH_HOSTS_PER_LOCALITY
};

static inline void
rp_priority_set_update_hosts_params_clear(RpPrioritySetUpdateHostsParams* self)
{
    g_return_if_fail(self != NULL);
    g_clear_pointer(&self->hosts, rp_host_vector_unref);
    g_clear_pointer(&self->healthy_hosts, rp_host_vector_unref);
    g_clear_pointer(&self->degraded_hosts, rp_host_vector_unref);
    g_clear_pointer(&self->excluded_hosts, rp_host_vector_unref);
    //TODO...clear locality pointers
}

static inline void
rp_priority_set_update_hosts_params_clear_gdestroy(gpointer arg)
{
    rp_priority_set_update_hosts_params_clear((RpPrioritySetUpdateHostsParams*)arg);
}

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(RpPrioritySetUpdateHostsParams,
                                 rp_priority_set_update_hosts_params_clear)


typedef struct _RpPrioritySetHostUpdateCb RpPrioritySetHostUpdateCb;
struct _RpPrioritySetHostUpdateCb {
    void (*update_hosts)(RpPrioritySetHostUpdateCb*,
                            guint32,
                            RpPrioritySetUpdateHostsParams*,
                            const RpHostVector*,
                            const RpHostVector*,
                            bool*,
                            guint32*);
};

struct _RpPrioritySetInterface {
    GTypeInterface parent_iface;

    //TODO...
    RpCallbackHandlePtr (*add_member_update_cb)(RpPrioritySet*,
                                                RpPrioritySetMemberUpdateCb,
                                                gpointer);
    RpCallbackHandlePtr (*add_priority_update_cb)(RpPrioritySet*,
                                                    RpPrioritySetPriorityUpdateCb,
                                                    gpointer);
    const RpHostSetPtrVector* (*host_sets_per_priority)(RpPrioritySet*);
    RpHostMapSnap* (*cross_priority_host_map)(RpPrioritySet*);
    void (*update_hosts)(RpPrioritySet*,
                            guint32,
                            RpPrioritySetUpdateHostsParams*,
                            const RpHostVector*,
                            const RpHostVector*,
                            bool*,
                            guint32*,
                            RpHostMapSnap*);
};
static inline RpCallbackHandlePtr
rp_priority_set_add_member_update_cb(RpPrioritySet* self, RpPrioritySetMemberUpdateCb cb, gpointer arg)
{
    g_return_val_if_fail(RP_IS_PRIORITY_SET(self), NULL);
    return RP_PRIORITY_SET_GET_IFACE(self)->add_member_update_cb(self, cb, arg);
}
static inline RpCallbackHandlePtr
rp_priority_set_add_priority_update_cb(RpPrioritySet* self, RpPrioritySetPriorityUpdateCb cb, gpointer arg)
{
    g_return_val_if_fail(RP_IS_PRIORITY_SET(self), NULL);
    return RP_PRIORITY_SET_GET_IFACE(self)->add_priority_update_cb(self, cb, arg);
}
static inline const RpHostSetPtrVector*
rp_priority_set_host_sets_per_priority(RpPrioritySet* self)
{
    return RP_IS_PRIORITY_SET(self) ?
        RP_PRIORITY_SET_GET_IFACE(self)->host_sets_per_priority(self) : NULL;
}
static inline RpHostMapSnap*
rp_priority_set_cross_priority_host_map(RpPrioritySet* self)
{
    return RP_IS_PRIORITY_SET(self) ?
        RP_PRIORITY_SET_GET_IFACE(self)->cross_priority_host_map(self) : NULL;
}
static inline void
rp_priority_set_update_hosts(RpPrioritySet* self, guint32 priority, RpPrioritySetUpdateHostsParams* params,
                                const RpHostVector* hosts_added, const RpHostVector* hosts_removed,
                                bool* weighted_priority_health, guint32* overprovisioning_factor, RpHostMapSnap* cross_priority_host_map)
{
    if (RP_IS_PRIORITY_SET(self))
    {
        RP_PRIORITY_SET_GET_IFACE(self)->update_hosts(self,
                                                        priority,
                                                        params,
                                                        hosts_added,
                                                        hosts_removed,
                                                        weighted_priority_health,
                                                        overprovisioning_factor,
                                                        cross_priority_host_map);
    }
}

/**
 * An upstream cluster (group of hosts). This class is the "primary" singleton cluster used amongst
 * all forwarding threads/workers. Individual HostSets are used on the workers themselves.
 */
#define RP_TYPE_CLUSTER rp_cluster_get_type()
G_DECLARE_INTERFACE(RpCluster, rp_cluster, RP, CLUSTER, GObject)

typedef RpStatusCode_e (*RpClusterInitializeCb)(gpointer);

struct _RpClusterInterface {
    GTypeInterface parent_iface;

    //TODO...
    RpClusterInfoConstSharedPtr (*info)(RpCluster*);
    //TODO...
    void (*initialize)(RpCluster*, RpClusterInitializeCb, gpointer);
    RpInitializePhase_e (*initialize_phase)(RpCluster*);
    RpPrioritySet* (*priority_set)(RpCluster*);
};

typedef SHARED_PTR(RpCluster) RpClusterSharedPtr;

static inline bool
rp_cluster_is_a(RpClusterSharedPtr self)
{
    g_return_val_if_fail(self != NULL, false);
    return RP_IS_CLUSTER((GObject*)self);
}
static inline RpClusterInterface*
rp_cluster_iface(RpClusterSharedPtr self)
{
    return RP_CLUSTER_GET_IFACE((GObject*)self);
}
static inline void
rp_cluster_set_object(RpClusterSharedPtr* dst, RpClusterSharedPtr src)
{
    g_return_if_fail(dst != NULL);
    g_set_object((GObject**)dst, (GObject*)src);
}
static inline RpClusterInfoConstSharedPtr
rp_cluster_info(RpClusterSharedPtr self)
{
    g_return_val_if_fail(rp_cluster_is_a(self), NULL);
    return rp_cluster_iface(self)->info(self);
}
static inline void
rp_cluster_initialize(RpClusterSharedPtr self, RpClusterInitializeCb callback, gpointer arg)
{
    g_return_if_fail(rp_cluster_is_a(self));
    rp_cluster_iface(self)->initialize(self, callback, arg);
}
static inline RpInitializePhase_e
rp_cluster_initialize_phase(RpClusterSharedPtr self)
{
    g_return_val_if_fail(rp_cluster_is_a(self), RpInitializePhase_Primary);
    return rp_cluster_iface(self)->initialize_phase(self);
}
static inline RpPrioritySet*
rp_cluster_priority_set(RpClusterSharedPtr self)
{
    g_return_val_if_fail(rp_cluster_is_a(self), NULL);
    return rp_cluster_iface(self)->priority_set(self);
}

G_END_DECLS
