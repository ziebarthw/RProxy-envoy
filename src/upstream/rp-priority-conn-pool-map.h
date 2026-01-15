/*
 * rp-priority-conn-pool-map.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-conn-pool.h"
#include "upstream/rp-conn-pool-map.h"
#include "rp-upstream.h"

G_BEGIN_DECLS

/**
 *  A class mapping keys to connection pools, with some recycling logic built in.
 */
#define RP_TYPE_PRIORITY_CONN_POOL_MAP rp_priority_conn_pool_map_get_type()
G_DECLARE_DERIVABLE_TYPE(RpPriorityConnPoolMap, rp_priority_conn_pool_map, RP, PRIORITY_CONN_POOL_MAP, GObject)

struct _RpPriorityConnPoolMapClass {
    GObjectClass parent_class;

    UNIQUE_PTR(RpHttpConnectionPoolInstance) (*get_pool)(RpPriorityConnPoolMap*,
                                                            RpResourcePriority_e,
                                                            gpointer,
                                                            RpPoolFactory,
                                                            gpointer);
    bool (*erase_pool)(RpPriorityConnPoolMap*, RpResourcePriority_e, gpointer);
    gsize (*size)(RpPriorityConnPoolMap*);
    bool (*empty)(RpPriorityConnPoolMap*);
    void (*clear)(RpPriorityConnPoolMap*);
    void (*add_idle_callback)(RpPriorityConnPoolMap*, const RpIdleCb);
    void (*drain_connections)(RpPriorityConnPoolMap*, RpDrainBehavior_e);
};

static inline UNIQUE_PTR(RpHttpConnectionPoolInstance)
rp_priority_conn_pool_map_get_pool(RpPriorityConnPoolMap* self, RpResourcePriority_e priority, const gpointer key, const RpPoolFactory factory, gpointer user_data)
{
    return RP_IS_PRIORITY_CONN_POOL_MAP(self) ?
        RP_PRIORITY_CONN_POOL_MAP_GET_CLASS(self)->get_pool(self, priority, key, factory, user_data) : NULL;
}
static inline bool
rp_priority_conn_pool_map_erase_pool(RpPriorityConnPoolMap* self, RpResourcePriority_e priority, const gpointer key)
{
    return RP_IS_PRIORITY_CONN_POOL_MAP(self) ?
        RP_PRIORITY_CONN_POOL_MAP_GET_CLASS(self)->erase_pool(self, priority, key) : false;
}
static inline gsize
rp_priority_conn_pool_map_size(RpPriorityConnPoolMap* self)
{
    return RP_IS_PRIORITY_CONN_POOL_MAP(self) ?
        RP_PRIORITY_CONN_POOL_MAP_GET_CLASS(self)->size(self) : 0;
}
static inline bool
rp_priority_conn_pool_map_empty(RpPriorityConnPoolMap* self)
{
    return RP_IS_PRIORITY_CONN_POOL_MAP(self) ?
        RP_PRIORITY_CONN_POOL_MAP_GET_CLASS(self)->empty(self) : true;
}
static inline void
rp_priority_conn_pool_map_clear(RpPriorityConnPoolMap* self)
{
    if (RP_IS_PRIORITY_CONN_POOL_MAP(self)) \
        RP_PRIORITY_CONN_POOL_MAP_GET_CLASS(self)->clear(self);
}
static inline void
rp_priority_conn_pool_map_add_idle_callback(RpPriorityConnPoolMap* self, const RpIdleCb cb)
{
    if (RP_IS_PRIORITY_CONN_POOL_MAP(self)) \
        RP_PRIORITY_CONN_POOL_MAP_GET_CLASS(self)->add_idle_callback(self, cb);
}
static inline void
rp_priority_conn_pool_map_drain_connections(RpPriorityConnPoolMap* self, RpDrainBehavior_e drain_behavior)
{
    if (RP_IS_PRIORITY_CONN_POOL_MAP(self)) \
        RP_PRIORITY_CONN_POOL_MAP_GET_CLASS(self)->drain_connections(self, drain_behavior);
}

GPtrArray* rp_priority_conn_pool_map_conn_pool_maps_(RpPriorityConnPoolMap* self);

G_END_DECLS
