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

typedef struct _RpPreconnectPolicy RpPreconnectPolicy;
typedef struct _RpClusterFactory RpClusterFactory;

// Configuration for a single upstream cluster.
typedef struct _RpClusterCfg RpClusterCfg;
struct _RpClusterCfg {

    struct PreconnectPolicy{
        double per_upstream_preconnect_ratio; //lte 3.0, gte 1.0
        double predictive_preconnect_ratio; //lte 3.0, gte 1.0
    } preconnect_policy;

    char* name;

    RpDiscoveryType_e cluster_discovery_type;

    guint32 connect_timeout_secs; // default 5s

    guint32 per_connection_buffer_limit_bytes; // default 1M

    RpLbPolicy_e lb_policy;

    RpDnsLookupFamily_e dns_lookup_family;

    bool connection_pool_per_downstream_connection;

    lztq* lb_endpoints;

    RpClusterFactory* factory;
    RpDispatcher* dispatcher;
    rule_t* rule;
};

static inline void
rp_cluster_cfg_dtor(RpClusterCfg* self)
{
    g_clear_pointer(&self->name, g_free);
}

static inline void
rp_cluster_cfg_set_name(RpClusterCfg* self, const char* name)
{
    self->name = g_strdup(name);
}

static inline void
rp_cluster_cfg_set_type(RpClusterCfg* self, RpDiscoveryType_e type)
{
    self->cluster_discovery_type = type;
}

G_END_DECLS
