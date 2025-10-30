/*
 * rp-dynamic-forward-proxy.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-factory-context.h"
#include "rp-pass-through-filter.h"
#include "rp-server-filter-config.h"
#include "rp-dfp-cluster-store.h"

G_BEGIN_DECLS

typedef struct _RpDynamicForwardProxyCfg RpDynamicForwardProxyCfg;
struct _RpDynamicForwardProxyCfg {
    RpFilterConfigBase config;
    RpClusterManager* m_cluster_manager;
};

static inline RpDynamicForwardProxyCfg
rp_dynamic_forward_proxy_cfg_ctor(RpClusterManager* cluster_manager)
{
    RpDynamicForwardProxyCfg self = {
        .config = rp_filter_config_base_ctor("dynamic-forward-proxy"),
        .m_cluster_manager = cluster_manager
    };
    return self;
}

#define RP_TYPE_DYNAMIC_FORWARD_PROXY rp_dynamic_forward_proxy_get_type()
G_DECLARE_FINAL_TYPE(RpDynamicForwardProxy, rp_dynamic_forward_proxy, RP, DYNAMIC_FORWARD_PROXY, RpPassThroughFilter)

RpFilterFactoryCb* rp_dynamic_forward_proxy_create_filter_factory(RpFactoryContext* context);

#if 0
// Accessors for tests or future use
static inline RpDfpClusterStore*
rp_dynamic_forward_proxy_cluster_store(RpDynamicForwardProxy* self)
{
    return (RpDfpClusterStore*)self->m_cluster_store;
}
#endif

G_END_DECLS
