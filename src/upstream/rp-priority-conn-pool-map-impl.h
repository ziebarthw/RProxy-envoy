/*
 * rp-priority-conn-pool-map-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "upstream/rp-priority-conn-pool-map.h"

G_BEGIN_DECLS

#define RP_TYPE_PRIORITY_CONN_POOL_MAP_IMPL rp_priority_conn_pool_map_impl_get_type()
G_DECLARE_FINAL_TYPE(RpPriorityConnPoolMapImpl, rp_priority_conn_pool_map_impl, RP, PRIORITY_CONN_POOL_MAP_IMPL, RpPriorityConnPoolMap)

RpPriorityConnPoolMapImpl* rp_priority_conn_pool_map_impl_new(RpDispatcher* dispatcher,
                                                                RpHostConstSharedPtr host);

G_END_DECLS
