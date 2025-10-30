/*
 * rp-common-virtual-host-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-factory-context.h"
#include "rp-router.h"
#include "rp-route-configuration.h"

G_BEGIN_DECLS

/**
 * Implementation of VirtualHost that reads from the proto config. This class holds all shared
 * data for all routes in the virtual host.
 */
#define RP_TYPE_COMMON_VIRTUAL_HOST_IMPL rp_common_virtual_host_impl_get_type()
G_DECLARE_FINAL_TYPE(RpCommonVirtualHostImpl, rp_common_virtual_host_impl, RP, COMMON_VIRTUAL_HOST_IMPL, GObject)

RpCommonVirtualHostImpl* rp_common_virtual_host_impl_create(RpVirtualHostCfg* virtual_host, RpRouteCommonConfig* global_route_config, RpServerFactoryContext* factory_context);

G_END_DECLS
