/*
 * rp-conn-pool-map-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "upstream/rp-conn-pool-map.h"

G_BEGIN_DECLS

#define RP_TYPE_CONN_POOL_MAP_IMPL rp_conn_pool_map_impl_get_type()
G_DECLARE_FINAL_TYPE(RpConnPoolMapImpl, rp_conn_pool_map_impl, RP, CONN_POOL_MAP_IMPL, RpConnPoolMap)

RpConnPoolMapImpl* rp_conn_pool_map_impl_new(SHARED_PTR(RpDispatcher) dispatcher,
                                                SHARED_PTR(RpHost) host,
                                                RpResourcePriority_e priority);

G_END_DECLS
