/*
 * rp-virtual-host-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-codec.h"
#include "rp-factory-context.h"
#include "rp-router.h"
#include "rp-route-configuration.h"

G_BEGIN_DECLS

/**
 * Virtual host that holds a collection of routes.
 */
#define RP_TYPE_VIRTUAL_HOST_IMPL rp_virtual_host_impl_get_type()
G_DECLARE_FINAL_TYPE(RpVirtuaHostImpl, rp_virtual_host_impl, RP, VIRTUAL_HOST_IMPL, GObject)

RpVirtuaHostImpl* rp_virtual_host_impl_new(RpVirtualHostCfg* virtual_host, RpRouteCommonConfig* global_route_config, RpServerFactoryContext* factory_context, RpStatusCode_e* creation_status);

G_END_DECLS
