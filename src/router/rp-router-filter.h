/*
 * rp-router-filter.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-server-filter-config.h"
#include "rp-router-filter-interface.h"
#include "router/rp-filter-config.h"

G_BEGIN_DECLS

typedef struct _RpUpstreamRequest RpUpstreamRequest;
typedef struct _RpUpstreamRequest* RpUpstreamRequestPtr;

// https://github.com/envoyproxy/envoy/blob/main/api/envoy/extensions/filters/http/router/v3/router.proto
typedef struct _RpRouterCfg RpRouterCfg;
struct _RpRouterCfg {
    RpFilterConfigBase config;

    bool suppress_rproxy_headers;
};

static inline RpRouterCfg
rp_router_cfg_ctor(RpFactoryContext* context, bool suppress_rproxy_headers)
{
    RpRouterCfg self = {
        .config = rp_filter_config_base_ctor("router-filter-config", context),
        .suppress_rproxy_headers = suppress_rproxy_headers
    };
    return self;
}

/**
 * Service routing filter.
 */
#define RP_TYPE_ROUTER_FILTER rp_router_filter_get_type()
G_DECLARE_FINAL_TYPE(RpRouterFilter, rp_router_filter, RP, ROUTER_FILTER, GObject)

//TODO...RpFilterHeadersStatus_e rp_router_filter_continue_decode_headers()
RpFilterFactoryCb* rp_router_filter_create_filter_factory(RpRouterCfg* config,
                                                            RpFactoryContext* context);
RpCoreResponseFlag_e rp_router_filter_stream_reset_reason_to_response_flag(RpStreamResetReason_e reset_reason);
GSList* rp_router_filter_upstream_requests(RpRouterFilter* self);

G_END_DECLS
