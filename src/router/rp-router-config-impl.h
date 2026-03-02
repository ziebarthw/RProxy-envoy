/*
 * rp-router-config-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-factory-context.h"
#include "rp-router.h"
#include "rp-codec.h"
#include "rp-cluster-manager.h"
#include "rp-route-configuration.h"

G_BEGIN_DECLS

struct RpRouterConfigImpl;
typedef struct _RpRouterCommonConfigImpl RpRouterCommonConfigImpl;
typedef SHARED_PTR(RpRouterCommonConfigImpl) RpRouterCommonConfigSharedPtr;

/**
 * Shared part of the route configuration implementation.
 */
#define RP_TYPE_ROUTER_COMMON_CONFIG_IMPL rp_router_common_config_impl_get_type()
G_DECLARE_FINAL_TYPE(RpRouterCommonConfigImpl, rp_router_common_config_impl, RP, ROUTER_COMMON_CONFIG_IMPL, GObject)

RpRouterCommonConfig* rp_router_common_config_impl_create(const RpRouteConfiguration* config,
                                                            RpServerFactoryContext* factory_context);


/**
 * Implementation of Config that reads from a proto file.
 */
#define RP_TYPE_ROUTER_CONFIG_IMPL rp_router_config_impl_get_type()
G_DECLARE_FINAL_TYPE(RpRouterConfigImpl, rp_router_config_impl, RP, ROUTER_CONFIG_IMPL, GObject)

RpRouterConfigImpl* rp_router_config_impl_create(const RpRouteConfiguration* config,
                                                    RpServerFactoryContext* factory_context);

G_END_DECLS
