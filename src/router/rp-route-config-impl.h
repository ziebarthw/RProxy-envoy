/*
 * rp-route-config-impl.h
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
#include "rp-thread-local.h"

G_BEGIN_DECLS

/**
 * Implementation of Config that reads from a proto file.
 */
#define RP_TYPE_ROUTE_CONFIG_IMPL rp_route_config_impl_get_type()
G_DECLARE_FINAL_TYPE(RpRouteConfigImpl, rp_route_config_impl, RP, ROUTE_CONFIG_IMPL, GObject)

RpRouteConfigImpl* rp_route_config_impl_create(RpRouteConfiguration* config,
                                                RpServerFactoryContext* factory_context,
                                                RpThreadLocalInstance* tls);

G_END_DECLS
