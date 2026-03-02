/*
 * rp-cluster-configuration.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"
#include "rp-dispatcher.h"
#include "rp-host-description.h"

G_BEGIN_DECLS

// Simplified mirrors of Envoy protos (api/envoy/config/cluster/v3/cluster.proto,
// api/envoy/extensions/filters/http/dynamic_forward_proxy/v3/dynamic_forward_proxy.proto)
// + dependencies (common/dynamic_forward_proxy/v3/DnsCacheConfig, SubClusterConfig, etc.).
// Durations simplified to uint64_t ns (use macros for us/ms/s).
// Repeated fields as fixed arrays + count (or dynamic alloc for prod).
// Enums as named constants.

#ifdef WITH_DYNAMIC_CONFIG
#define IF_DYNAMIC_CONFIG(x, ...) x##__VA_ARGS__
#else
#define IF_DYNAMIC_CONFIG(x, ...)
#endif
#ifndef WITH_DYNAMIC_CONFIG
#define IF_STATIC_CONFIG(x, ...) x##__VA_ARGS__
#else
#define IF_STATIC_CONFIG(x, ...)
#endif

// === Helper Macros (Durations) ===
#define NS_PER_S 1000000000ULL
#define NS_PER_MS 1000000ULL
#define DURATION_S(s) ((uint64_t)(s) * NS_PER_S)
#define DURATION_MS(ms) ((uint64_t)(ms) * NS_PER_MS)


// Refer to :ref:`service discovery type <arch_overview_service_discovery_types>`
// for an explanation on each type.
typedef enum {
    // Refer to the :ref:`static discovery type<arch_overview_service_discovery_types_static>`
    // for an explanation.
    RpDiscoveryType_STATIC,

    // Refer to the :ref:`strict DNS discovery
    // type<arch_overview_service_discovery_types_strict_dns>`
    // for an explanation.
    RpDiscoveryType_STRICT_DNS,

    // Refer to the :ref:`logical DNS discovery
    // type<arch_overview_service_discovery_types_logical_dns>`
    // for an explanation.
    RpDiscoveryType_LOCAL_DNS,

    // Refer to the :ref:`service discovery type<arch_overview_service_discovery_types_eds>`
    // for an explanation.
    RpDiscoveryType_EDS,

    // Refer to the :ref:`original destination discovery
    // type<arch_overview_service_discovery_types_original_destination>`
    // for an explanation.
    RpDiscoveryType_ORIGINAL_DST
} RpDiscoveryType_e;

// Refer to :ref:`load balancer type <arch_overview_load_balancing_types>` architecture
// overview section for information on each type.
typedef enum {
    RpLbPolicy_ROUND_ROBIN,
    RpLbPolicy_LEAST_REQUEST,
    RpLbPolicy_RING_HASH,
    RpLbPolicy_RANDOM,
    RpLbPolicy_MAGLEV,
    RpLbPolicy_CLUSTER_PROVIDED,
    RpLbPolicy_LOAD_BALANCING_POLICY_CONFIG
} RpLbPolicy_e;

typedef enum {
    RpProtocol_TCP,
    RpProtocol_UDP
} RpProtocol_e;

typedef enum {
    RpSocketState_STATE_FREEBIND,
    RpSocketState_STATE_BOUND,
    RpSocketState_SSTATE_LISTENING
} RpSocketState_e;

typedef enum {
    RpSocketOptValueType_INT_VALUE,
    RpSocketOptValueType_BUF_VALUE
} RpSocketOptValueType_e;

typedef enum {
    RpPortSpecifierType_PORT_VALUE,
    RpPortSpecifierType_NAMED_PORT
} RpPortSpecifierType_e;

typedef enum {
    RpHostIdentifierType_ENDPOINT,
    RpHostIdentifierType_ENDPOINT_NAME
} RpHostIdentifierType_e;

typedef enum {
    RpClusterDiscoveryTypeType_NONE,
    RpClusterDiscoveryTypeType_TYPE,
    RpClusterDiscoveryTypeType_CLUSTER_TYPE
} RpClusterDiscoveryTypeType_e;

typedef enum {
    RpTypedConfigType_NONE = 0,
    RpTypedConfigType_DFP_CLUSTER_CONFIG
} RpTypedConfigType_e;

typedef enum {
    RpAddressType_SOCKET_ADDRESS,
    RpAddressType_PIPE,
    RpAddressType_RPROXY_INTERNAL_ADDRESS
} RpAddressType_e;

typedef enum {
    RpAddressCfgCase_ADDRESS_NOT_SET,
    RpAddressCfgCase_SOCKET_ADDRESS,
    RpAddressCfgCase_PIPE,
    RpAddressCfgCase_RPROXY_INTERNAL_ADDRESS
} RpAddressCfgCase_e;

typedef enum {
    RpDfpImplementationSpecifierType_DNS_CACHE_CONFIG_TYPE,
    RpDfpImplementationSpecifierType_SUB_CLUSTER_TYPE
} RpDfpImplementationSpecifierType_e;

/**
 * RpSocketOptionCfg
 */
typedef struct _RpSocketOptionCfg RpSocketOptionCfg;
struct _RpSocketOptionCfg {
    union {
        gint64 int_value;
        guint8 buf_value[28];
    } value;
    RpSocketOptValueType_e value_type;
    RpSocketState_e state;
};

/**
 * RpSocketAddressCfg
 */
typedef struct _RpSocketAddressCfg RpSocketAddressCfg;
struct _RpSocketAddressCfg {
    RpProtocol_e protocol;
    char address[256];     // e.g., "8.8.8.8" or "xds-server.example.com"
    union {
        guint32 port_value;
        char named_port[128];
    } port_specifier;
    RpPortSpecifierType_e port_specifier_type;
    char resolver_name[128];
    bool ipv4_compat;
    char network_namespace_filepath[256];
};

static inline void
rp_socket_address_cfg_set_address(RpSocketAddressCfg* self, const char* address)
{
    g_strlcpy(self->address, address, sizeof(self->address));
}

static inline const char*
rp_socket_address_cfg_address(const RpSocketAddressCfg* self)
{
    return self->address;
}

static inline const char*
rp_socket_address_cfg_resolver_name(const RpSocketAddressCfg* self)
{
    return self->resolver_name;
}

static inline void
rp_socket_address_cfg_set_port_value(RpSocketAddressCfg* self, guint32 port_value)
{
    self->port_specifier_type = RpPortSpecifierType_PORT_VALUE;
    self->port_specifier.port_value = port_value;
}

static inline guint32
rp_socket_address_cfg_port_value(const RpSocketAddressCfg* self)
{
    return self->port_specifier.port_value;
}

/**
 * RpPipeCfg
 */
typedef struct _RpPipeCfg RpPipeCfg;
struct _RpPipeCfg {
    char path[512];
    guint32 mode; // lte 511.
};

/**
 * RpRproxyInternalAddress
 */
typedef struct _RpRproxyInternalAddress RpRproxyInternalAddress;
struct _RpRproxyInternalAddress {
    union {
        char server_listener_name[128];
    } address_name_specifier;
    char endpoint_id[25];
};

/**
 * RpAddressCfg
 */
typedef struct _RpAddressCfg RpAddressCfg;
struct _RpAddressCfg {
    union {
        RpSocketAddressCfg socket_address;
        RpPipeCfg pipe;
        RpRproxyInternalAddress rproxy_internal_address;
    } address;
    RpAddressType_e address_type;
};

static inline const RpSocketAddressCfg*
rp_address_cfg_socket_address(const RpAddressCfg* self)
{
    return &self->address.socket_address;
}

static inline RpSocketAddressCfg*
rp_address_cfg_mutable_socket_address(RpAddressCfg* self)
{
    self->address_type = RpAddressType_SOCKET_ADDRESS;
    return &self->address.socket_address;
}

static inline RpAddressCfgCase_e
rp_address_cfg_address_case(const RpAddressCfg* self)
{
    switch (self->address_type)
    {
        case RpAddressType_SOCKET_ADDRESS:
            return RpAddressCfgCase_SOCKET_ADDRESS;
        case RpAddressType_PIPE:
            return RpAddressCfgCase_PIPE;
        case RpAddressType_RPROXY_INTERNAL_ADDRESS:
            return RpAddressCfgCase_RPROXY_INTERNAL_ADDRESS;
        default:
            return RpAddressCfgCase_ADDRESS_NOT_SET;
    }
}

/**
 * RpBindCfg
 */
typedef struct _RpBindCfg RpBindCfg;
struct _RpBindCfg {
    RpSocketAddressCfg source_address;
    bool freebind; // default: false;
    IF_DYNAMIC_CONFIG(GPtrArray*/*RpSocketOptionCfg*/ socket_options;)
    IF_STATIC_CONFIG(RpSocketOptionCfg socket_options[8];
    int socket_options_len;)
};

/**
 * RpEndpointCfg - upstream host identifier.
 */
typedef struct _RpEndpointCfg RpEndpointCfg;
struct _RpEndpointCfg {
    RpAddressCfg address;
    char hostname[256];
    struct RpAdditionalAddress {
        RpAddressCfg address;
    } additional_addresses[8];
    gint additional_addresses_len;
};

static inline const RpAddressCfg*
rp_endpoint_cfg_address(const RpEndpointCfg* self)
{
    return &self->address;
}

static inline RpAddressCfg*
rp_endpoint_cfg_mutable_address(RpEndpointCfg* self)
{
    return &self->address;
}

static inline const char*
rp_endpoint_cfg_hostname(const RpEndpointCfg* self)
{
    return self->hostname;
}

static inline void
rp_endpoint_cfg_clear_additional_addresses(RpEndpointCfg* self)
{
    self->additional_addresses_len = 0;
}

/**
 * RpLbEndpointCfg
 */
typedef struct _RpLbEndpointCfg RpLbEndpointCfg;
struct _RpLbEndpointCfg {
    union {
        RpEndpointCfg endpoint;
        char endpoint_name[256];
    } host_identifier;
    RpHostIdentifierType_e host_identifier_type;
    guint32 load_balancing_weight; // gte 1;
    RpMetadataConstSharedPtr metadata;
};

static inline const RpEndpointCfg*
rp_lb_endpoint_cfg_endpoint(const RpLbEndpointCfg* self)
{
    return &self->host_identifier.endpoint;
}

static inline RpEndpointCfg*
rp_lb_endpoint_cfg_mutable_endpoint(RpLbEndpointCfg* self)
{
    self->host_identifier_type = RpHostIdentifierType_ENDPOINT;
    return &self->host_identifier.endpoint;
}

static inline RpMetadataConstSharedPtr
rp_lb_endpoint_cfg_metadata(const RpLbEndpointCfg* self)
{
    return self->metadata;
}

static inline guint32
rp_lb_endpoint_cfg_load_balancing_weight_value(const RpLbEndpointCfg* self)
{
    return self->load_balancing_weight;
}

/**
 * RpLocalityLbEndpointsCfg
 */
typedef struct _RpLocalityLbEndpointsCfg RpLocalityLbEndpointsCfg;
struct _RpLocalityLbEndpointsCfg {
    IF_DYNAMIC_CONFIG(GPtrArray*/*RpLbEndpointCfg*/ lb_endpoints;)
    IF_STATIC_CONFIG(RpLbEndpointCfg lb_endpoints[12];
    guint lb_endpoints_len;)
    guint32 load_balancing_weight; // gte 1;
    guint32 priority; // lte 128;
};

static inline const RpLbEndpointCfg*
rp_locality_lb_endpoints_cfg_lb_endpoints(const RpLocalityLbEndpointsCfg* self)
{
    return self->lb_endpoints;
}

static inline gint
rp_locality_lb_endpoints_cfg_lb_endpoints_len(const RpLocalityLbEndpointsCfg* self)
{
    return self->lb_endpoints_len;
}

static inline gint
rp_locality_lb_endpoints_cfg_lb_endpoints_size(const RpLocalityLbEndpointsCfg* self)
{
    return (gint)(sizeof(self->lb_endpoints) / sizeof(self->lb_endpoints[0]));
}

static inline void
rp_locality_lb_endpoints_cfg_clear_lb_endpoints(RpLocalityLbEndpointsCfg* self)
{
    self->lb_endpoints_len = 0;
}

static inline RpLbEndpointCfg*
rp_locality_lb_endpoints_cfg_add_lb_endpoints(RpLocalityLbEndpointsCfg* self)
{
    return self->lb_endpoints_len < rp_locality_lb_endpoints_cfg_lb_endpoints_size(self) ?
            &self->lb_endpoints[self->lb_endpoints_len++] : NULL;
}

static inline guint32
rp_locality_lb_endpoints_cfg_priority(const RpLocalityLbEndpointsCfg* self)
{
    return self->priority;
}

typedef enum {
    RpTypedExtensionConfigType_NONE,
    RpTypedExtensionConfigType_DUMMY
} RpTypedExtensionConfigType_e;

/**
 * RpDummyTypedExtensionCfg
 */
typedef struct _RpDummyTypedExtensionCfg RpDummyTypedExtensionCfg;
struct _RpDummyTypedExtensionCfg {
    int property_1;
};

/**
 * RpLoadBalancingPolicyPolicyCfg - a single Policy message.
 */
typedef struct _RpLoadBalancingPolicyPolicyCfg RpLoadBalancingPolicyPolicyCfg;
struct _RpLoadBalancingPolicyPolicyCfg {
    union {
        RpDummyTypedExtensionCfg dummy_typed_config;
    } typed_extension_config;
    RpTypedExtensionConfigType_e typed_extension_config_type;
};

/**
 * RpLoadBalancingPolicyCfg - collection of one or more Policy messages.
 */
typedef struct _RpLoadBalancingPolicyCfg RpLoadBalancingPolicyCfg;
struct _RpLoadBalancingPolicyCfg {
    RpLoadBalancingPolicyPolicyCfg policies[1]; //REVISIT when implemented!
    int policies_len;
};

/**
 * RpLbPolicyCfg
 */
typedef struct _RpLbPolicyCfg RpLbPolicyCfg;
struct _RpLbPolicyCfg {
    guint32 overprovisioning_factor; // gt 0.
    guint64 endpoint_stale_after;
    bool weighted_priority_health;
};

/**
 * RpClusterLoadAssignmentCfg
 */
typedef struct _RpClusterLoadAssignmentCfg RpClusterLoadAssignmentCfg;
struct _RpClusterLoadAssignmentCfg {
    // Load balancing policy settings.
    RpLbPolicyCfg policy;
    char cluster_name[128];
    IF_DYNAMIC_CONFIG(GPtrArray*/*RpLocalityLbEndpointsCfg*/ endpoints;)
    IF_STATIC_CONFIG(RpLocalityLbEndpointsCfg endpoints[12];
    gint endpoints_len;)
};

typedef UNIQUE_PTR(RpClusterLoadAssignmentCfg) RpClusterLoadAssignmentCfgPtr;

static inline RpClusterLoadAssignmentCfgPtr
rp_cluster_load_assignment_cfg_new(const RpClusterLoadAssignmentCfg* cfg)
{
    RpClusterLoadAssignmentCfg* self = g_new0(RpClusterLoadAssignmentCfg, 1);
    return memcpy(self, cfg, sizeof(*self));
}

static inline void
rp_cluster_load_assignment_cfg_set_cluster_name(RpClusterLoadAssignmentCfg* self, const char* cluster_name)
{
    g_strlcpy(self->cluster_name, cluster_name, sizeof(self->cluster_name));
}

static inline const char*
rp_cluster_load_assignment_cfg_cluster_name(const RpClusterLoadAssignmentCfg* self)
{
    return self->cluster_name;
}

static inline const RpLocalityLbEndpointsCfg*
rp_cluster_load_assignment_cfg_endpoints(const RpClusterLoadAssignmentCfg* self)
{
    return self->endpoints;
}

static inline gint
rp_cluster_load_assignment_cfg_endpoints_len(const RpClusterLoadAssignmentCfg* self)
{
    return self->endpoints_len;
}

static inline gint
rp_cluster_load_assignment_cfg_endpoints_size(const RpClusterLoadAssignmentCfg* self)
{
    return (gint)(sizeof(self->endpoints) / sizeof(self->endpoints[0]));
}

static inline void
rp_cluster_load_assignment_cfg_clear_endpoints(RpClusterLoadAssignmentCfg* self)
{
    while (self->endpoints_len > 0)
    {
        --self->endpoints_len;
        rp_locality_lb_endpoints_cfg_clear_lb_endpoints(&self->endpoints[self->endpoints_len]);
    }
}

static inline RpLocalityLbEndpointsCfg*
rp_cluster_load_assignment_cfg_add_endpoints(RpClusterLoadAssignmentCfg* self)
{
    return self->endpoints_len < rp_cluster_load_assignment_cfg_endpoints_size(self) ?
            &self->endpoints[self->endpoints_len++] : NULL;
}

static inline const RpLbPolicyCfg*
rp_cluster_load_assignment_cfg_policy(const RpClusterLoadAssignmentCfg* self)
{
    return &self->policy;
}

/**
 * RpDfpDnsCacheCfg (RpDynamicForwardProsxyCacheCfg)
 */
typedef struct _RpDfpDnsCacheCfg RpDfpDnsCacheCfg;
struct _RpDfpDnsCacheCfg {
    char name[128];
    RpDnsLookupFamily_e dns_lookup_family;
    struct timeval host_ttl;
    guint32 max_hosts;
};

/**
 * RpDfpSubClustersCfg (RpDynamicForwardProxySubClustersCfg)
 */
typedef struct _RpDfpSubClustersCfg RpDfpSubClustersCfg;
struct _RpDfpSubClustersCfg {
    RpLbPolicy_e lb_policy;         // default: ROUND_ROBIN
    guint32 max_sub_clusters;       // default: 1024
    struct timeval sub_cluster_ttl; // default: 5m
    //TODO...RpSocketAddressCfg preresolve_clusters[6???];
};

static inline RpLbPolicy_e
rp_dfp_sub_clusters_cfg_lb_policy(const RpDfpSubClustersCfg* self)
{
    return self ? self->lb_policy : RpLbPolicy_ROUND_ROBIN;
}

/**
 * RpDfpClusterCfg (RpDynamicForwardProxyClusterCfg)
 */
typedef struct _RpDfpClusterCfg RpDfpClusterCfg;
struct _RpDfpClusterCfg {
    union {
        RpDfpDnsCacheCfg dns_cache_config;
        RpDfpSubClustersCfg sub_clusters_config;
    } implementation_specifier;
    RpDfpImplementationSpecifierType_e implementation_specifier_type;
    bool save_upstream_address;
    bool allow_dynamic_host_from_filter_state;
};

static inline RpDfpClusterCfg*
rp_dfp_cluster_cfg_new(const RpDfpClusterCfg* cfg)
{
//    RpDfpClusterCfg* self = g_new(RpDfpClusterCfg, 1);
//    return memcpy(self, cfg, sizeof(*self));
    return g_memdup2(cfg, sizeof(*cfg));
}

static inline bool
rp_dfp_cluster_cfg_has_sub_clusters_cfg(const RpDfpClusterCfg* self)
{
    return self->implementation_specifier_type == RpDfpImplementationSpecifierType_SUB_CLUSTER_TYPE;
}

static inline const RpDfpSubClustersCfg*
rp_dfp_cluster_cfg_sub_cluster_cfg(const RpDfpClusterCfg* self)
{
    return rp_dfp_cluster_cfg_has_sub_clusters_cfg(self) ?
        &self->implementation_specifier.sub_clusters_config : NULL;
}

/**
 * RpCustomClusterTypeCfg
 */
typedef struct _RpCustomClusterTypeCfg RpCustomClusterTypeCfg;
struct _RpCustomClusterTypeCfg {
    char name[128];
    union {
        RpDfpClusterCfg dfp_cluster_config;
        //TODO...others.
    } typed_config;
    RpTypedConfigType_e typed_config_type;
};

static inline bool
rp_cluster_type_cfg_has_typed_config(const RpCustomClusterTypeCfg* self)
{
    return self->typed_config_type != RpTypedConfigType_NONE;
}

static inline const char*
rp_cluster_type_cfg_name(const RpCustomClusterTypeCfg* self)
{
    return self->name;
}

static inline gconstpointer
rp_cluster_type_cfg_typed_config(const RpCustomClusterTypeCfg* self)
{
    // REVISIT: this is stupid since the return address is the same regardless
    //          of which one is "active". Just a placeholder for somehow
    //          getting the actual type.
    switch (self->typed_config_type)
    {
        case RpTypedConfigType_DFP_CLUSTER_CONFIG:
            return &self->typed_config.dfp_cluster_config;
        default:
            return NULL;
    }
}

/**
 * RpPreconnectPolicyCfg
 */
typedef struct _RpPreconnectPolicyCfg RpPreconnectPolicyCfg;
struct _RpPreconnectPolicyCfg {
    double per_upstream_preconnect_ratio; // lte 3.0, gte 1.0;
    double predictive_preconnect_ratio; // lte 3.0, gte 1.0;
};

/**
 * RpClusterCfg - Configuration for a single upstream cluster.
 */
typedef struct _RpClusterCfg RpClusterCfg;
struct _RpClusterCfg {
    RpPreconnectPolicyCfg preconnect_policy;
    union {
        RpDiscoveryType_e type;
        RpCustomClusterTypeCfg cluster_type;
    } cluster_discovery_type;
    RpClusterDiscoveryTypeType_e cluster_discovery_type_type;
    guint32 connect_timeout_secs; // default 5s;
    guint32 per_connection_buffer_limit_bytes; // default 1M;
    RpLbPolicy_e lb_policy;
    RpClusterLoadAssignmentCfg load_assignment;
    RpDnsLookupFamily_e dns_lookup_family;
    RpBindCfg upstream_bind_config;
    RpLoadBalancingPolicyCfg load_balancing_policy;
    //TODO..RpLbSubsetCfg lb_subset_config;
    RpMetadataConstSharedPtr metadata;
    bool connection_pool_per_downstream_connection; // default: false;
    bool load_balancing_policy_set; // default: false;

    // Custom.
    rule_t* rule;

guint32 magic;
};

typedef UNIQUE_PTR(RpClusterCfg) RpClusterCfgPtr;

//#define WITH_CFG_GUARDED
#ifdef WITH_CFG_GUARDED
#define rp_cluster_cfg_new rp_cluster_cfg_dup_guarded
#define rp_cluster_cfg_free rp_cluster_cfg_free_guarded
RpClusterCfg* rp_cluster_cfg_dup_guarded(const RpClusterCfg* src);
void rp_cluster_cfg_free_guarded(RpClusterCfg* p);
#else
static inline RpClusterCfgPtr
rp_cluster_cfg_new(void)
{
    RpClusterCfg* self = g_new0(RpClusterCfg, 1);
    return self;
}
static inline RpClusterCfgPtr
rp_cluster_cfg_dup(const RpClusterCfg* cfg)
{
    g_return_val_if_fail(cfg != NULL, NULL);
    return g_memdup2(cfg, sizeof(*cfg));
}
static inline void
rp_cluster_cfg_free(RpClusterCfg* self)
{
    g_return_if_fail(self != NULL);
    g_free(self);
}
#endif

static inline bool
rp_cluster_cfg_has_cluster_type(const RpClusterCfg* self)
{
    return self->cluster_discovery_type_type == RpClusterDiscoveryTypeType_CLUSTER_TYPE;
}

static inline const RpCustomClusterTypeCfg*
rp_cluster_cfg_cluster_type(const RpClusterCfg* self)
{
    return &self->cluster_discovery_type.cluster_type;
}

static inline void
rp_cluster_cfg_clear_cluster_type(const RpClusterCfg* self)
{
    ((RpClusterCfg*)self)->cluster_discovery_type_type = RpClusterDiscoveryTypeType_NONE;
}

static inline RpDiscoveryType_e
rp_cluster_cfg_type(const RpClusterCfg* self)
{
    return self->cluster_discovery_type.type;
}

static inline const RpClusterLoadAssignmentCfg*
rp_cluster_cfg_load_assignment(const RpClusterCfg* self)
{
    return &self->load_assignment;
}

static inline RpClusterLoadAssignmentCfg*
rp_cluster_cfg_mutable_load_assignment(RpClusterCfg* self)
{
    return &self->load_assignment;
}

static inline rule_t*
rp_cluster_cfg_rule(const RpClusterCfg* self)
{
    return self->rule;
}

static inline void
rp_cluster_cfg_set_name(RpClusterCfg* self, const char* name)
{
    rp_cluster_load_assignment_cfg_set_cluster_name(&self->load_assignment, name);
}

static inline const char*
rp_cluster_cfg_name(const RpClusterCfg* self)
{
    return rp_cluster_load_assignment_cfg_cluster_name(&self->load_assignment);
}

static inline void
rp_cluster_cfg_set_type(RpClusterCfg* self, RpDiscoveryType_e type)
{
    g_return_if_fail(self != NULL);
    self->cluster_discovery_type.type = type;
    self->cluster_discovery_type_type = RpClusterDiscoveryTypeType_TYPE;
}

static inline void
rp_cluster_cfg_set_lb_policy(RpClusterCfg* self, RpLbPolicy_e lb_policy)
{
    self->lb_policy = lb_policy;
}

static inline RpLbPolicy_e
rp_cluster_cfg_lb_policy(const RpClusterCfg* self)
{
    return self->lb_policy;
}

static inline bool
rp_cluster_cfg_has_load_balancing_policy(const RpClusterCfg* self)
{
    return self->load_balancing_policy_set;
}

static inline bool
rp_cluster_cfg_has_lb_subset_config(const RpClusterCfg* self)
{
    return false; //TODO...not implemented.
}

G_END_DECLS
