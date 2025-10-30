/*
 * rp-filter-config.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-cluster-manager.h"
#include "rp-factory-context.h"
#include "rp-router.h"

G_BEGIN_DECLS

typedef struct _RpRouterCfg RpRouterCfg;

/**
 * Configuration for the router filter.
 */
#define RP_TYPE_ROUTER_FILTER_CONFIG rp_router_filter_config_get_type()
G_DECLARE_FINAL_TYPE(RpRouterFilterConfig, rp_router_filter_config, RP, ROUTER_FILTER_CONFIG, GObject)

RpRouterFilterConfig* rp_router_filter_config_new(RpRouterCfg* config_proto,
                                                    RpFactoryContext* factory_context);

G_END_DECLS
