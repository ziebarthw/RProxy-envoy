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
    RpDnsLookupFamily_AUTO,
    RpDnsLookupFamily_V4_ONLY,
    RpDnsLookupFamily_V6_ONLY,
    RpDnsLookupFamily_V4_PREFERRED,
    RpDnsLookupFamily_ALL
} RpDnsLookupFamily_e;

typedef enum {
    RpProtocol_TCP,
    RpProtocol_UDP
} RpProtocol_e;

typedef enum {
    RpSocketState_STATE_FREEBIND,
    RpSocketState_STATE_BOUND,
    RpSocketState_SSTATE_LISTENING
} RpSocketState_e;

#if 0
typedef struct _RpSocketOptionCfg RpSocketOptionCfg;
struct _RpSocketOption {
    union {
        gint64 int_value;
        guint8* buf_value;
    } value;
    RpSocketState_e state;
};

typedef struct _RpSocketAddressCfg RpSocketAddressCfg;
struct _RpSocketAddressCfg {
    RpProtocol_e protocol;
    char* address;
    union {
        guint32 port_value;
        char* named_port;
    } port_specifier;
    bool ipv4_compat;
};

typedef struct _RpBindCfg RpBindCfg;
struct _RpBindCfg {
    RpSocketAddressCfg source_address;
    bool freebind; // default: false;
    GPtrArray*/*RpSocketOptionCfg*/ socket_options;
};

typedef struct _RpEndpointCfg RpEndpointCfg;
struct _RpEndpointCfg {
    RpNetworkAddressInstanceConstSharedPtr address;
    char* hostname;
};

typedef struct _RpLbEndpointCfg RpLbEndpointCfg;
struct _RpLbEndpointCfg {
    union {
        RpEndpointCfg endpoint;
        char* endpoint_name;
    } host_identifier;
    guint32 load_balancing_weight; // gte 1;
};

typedef struct _RpLocalityLbEndpointsCfg RpLocalityLbEndpointsCfg;
struct _RpLocalityLbEndpointsCfg {
    GPtrArray*/*RpLbEndpointCfg*/ lb_endpoints;
    guint32 load_balancing_weight; // gte 1;
    guint32 priority; // lte 128;
};

typedef struct _RpClusterLoadAssignmentCfg RpClusterLoadAssignmentCfg;
struct _RpClusterLoadAssignmentCfg {
    char* cluster_name;
    GPtrArray*/*RpLocalityLbEndpointsCfg*/ endpoints;
};

// Configuration for a single upstream cluster.
typedef struct _RpClusterCfg RpClusterCfg;
struct _RpClusterCfg {
    struct PreconnectPolicy{
        double per_upstream_preconnect_ratio; // lte 3.0, gte 1.0;
        double predictive_preconnect_ratio; // lte 3.0, gte 1.0;
    } preconnect_policy;
    guint32 connect_timeout_secs; // default 5s;
    guint32 per_connection_buffer_limit_bytes; // default 1M;
    RpDiscoveryType_e type;
    RpLbPolicy_e lb_policy;
    RpClusterLoadAssignmentCfg load_assignment;
    RpDnsLookupFamily_e dns_lookup_family;
    RpBindCfg upstream_bind_config;
    bool connection_pool_per_downstream_connection; // default: false;
};
#endif//0

typedef struct _RpEndpointCfg RpEndpointCfg;
struct _RpEndpointCfg {
    RpNetworkAddressInstanceConstSharedPtr address;
    char* hostname;
};

typedef struct _RpLbEndpointCfg RpLbEndpointCfg;
struct _RpLbEndpointCfg {
    union {
        RpEndpointCfg endpoint;
        char* endpoint_name;
    } host_identifier;
    guint32 load_balancing_weight; // gte 1;
    RpMetadataConstSharedPtr metadata;
};

typedef struct _RpLocalityLbEndpointsCfg RpLocalityLbEndpointsCfg;
struct _RpLocalityLbEndpointsCfg {
    lztq* lb_endpoints;
    guint32 load_balancing_weight; // gte 1;
    guint32 priority; // lte 128;
};

/**
 * Configuration for a single upstream cluster.
 */
typedef struct _RpClusterCfg RpClusterCfg;
struct _RpClusterCfg {

    struct PreconnectPolicy{
        double per_upstream_preconnect_ratio; //lte 3.0, gte 1.0
        double predictive_preconnect_ratio; //lte 3.0, gte 1.0
    } preconnect_policy;

    char* name;

    RpDiscoveryType_e type;

    guint32 connect_timeout_secs; // default 5s

    guint32 per_connection_buffer_limit_bytes; // default 1M

    RpLbPolicy_e lb_policy;

    RpDnsLookupFamily_e dns_lookup_family;

    bool connection_pool_per_downstream_connection;

    RpLocalityLbEndpointsCfg endpoints;

    rule_t* rule;
};

static inline const RpClusterCfg*
rp_cluster_cfg_empty(void)
{
    static const RpClusterCfg EMPTY = {0};
    return &EMPTY;
}

static inline void
rp_cluster_cfg_dtor(RpClusterCfg* self)
{
    g_return_if_fail(self != NULL);
    g_clear_pointer(&self->name, g_free);
}

static inline void
rp_cluster_cfg_set_name(RpClusterCfg* self, const char* name)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(name != NULL);
    self->name = g_strdup(name);
}

static inline void
rp_cluster_cfg_set_type(RpClusterCfg* self, RpDiscoveryType_e type)
{
    g_return_if_fail(self != NULL);
    self->type = type;
}

G_END_DECLS
