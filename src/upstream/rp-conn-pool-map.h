/*
 * rp-conn-pool-map.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-conn-pool.h"
#include "rp-http-conn-pool.h"
#include "rp-upstream.h"

G_BEGIN_DECLS

typedef UNIQUE_PTR(RpHttpConnectionPoolInstance) (*RpPoolFactory)(gpointer user_data);

/**
 *  A class mapping keys to connection pools, with some recycling logic built in.
 */
#define RP_TYPE_CONN_POOL_MAP rp_conn_pool_map_get_type()
G_DECLARE_DERIVABLE_TYPE(RpConnPoolMap, rp_conn_pool_map, RP, CONN_POOL_MAP, GObject)

struct _RpConnPoolMapClass {
    GObjectClass parent_class;

    UNIQUE_PTR(RpHttpConnectionPoolInstance) (*get_pool)(RpConnPoolMap*,
                                                        gconstpointer,
                                                        const RpPoolFactory,
                                                        gconstpointer);
    bool (*erase_pool)(RpConnPoolMap*, gconstpointer);
    gsize (*size)(RpConnPoolMap*);
    bool (*empty)(RpConnPoolMap*);
    void (*clear)(RpConnPoolMap*);
    void (*add_idle_callback)(RpConnPoolMap*, const RpIdleCb);
    void (*drain_connections)(RpConnPoolMap*, RpDrainBehavior_e);
    bool (*free_one_pool)(RpConnPoolMap*);
    void (*clear_active_pools)(RpConnPoolMap*);
};

static inline UNIQUE_PTR(RpHttpConnectionPoolInstance)
rp_conn_pool_map_get_pool(RpConnPoolMap* self, gconstpointer key, const RpPoolFactory factory, gconstpointer user_data)
{
    return RP_IS_CONN_POOL_MAP(self) ?
        RP_CONN_POOL_MAP_GET_CLASS(self)->get_pool(self, key, factory, user_data) : NULL;
}
static inline bool
rp_conn_pool_map_erase_pool(RpConnPoolMap* self, gconstpointer key)
{
    return RP_IS_CONN_POOL_MAP(self) ?
        RP_CONN_POOL_MAP_GET_CLASS(self)->erase_pool(self, key) : false;
}
static inline gsize
rp_conn_pool_map_size(RpConnPoolMap* self)
{
    return RP_IS_CONN_POOL_MAP(self) ?
        RP_CONN_POOL_MAP_GET_CLASS(self)->size(self) : 0;
}
static inline bool
rp_conn_pool_map_empty(RpConnPoolMap* self)
{
    return RP_IS_CONN_POOL_MAP(self) ?
        RP_CONN_POOL_MAP_GET_CLASS(self)->empty(self) : true;
}
static inline void
rp_conn_pool_map_clear(RpConnPoolMap* self)
{
    if (RP_IS_CONN_POOL_MAP(self)) \
        RP_CONN_POOL_MAP_GET_CLASS(self)->clear(self);
}
static inline void
rp_conn_pool_map_add_idle_callback(RpConnPoolMap* self, const RpIdleCb cb)
{
    if (RP_IS_CONN_POOL_MAP(self)) \
        RP_CONN_POOL_MAP_GET_CLASS(self)->add_idle_callback(self, cb);
}
static inline void
rp_conn_pool_map_drain_connections(RpConnPoolMap* self, RpDrainBehavior_e drain_behavior)
{
    if (RP_IS_CONN_POOL_MAP(self)) \
        RP_CONN_POOL_MAP_GET_CLASS(self)->drain_connections(self, drain_behavior);
}
static inline bool
rp_conn_pool_map_free_one_pool(RpConnPoolMap* self)
{
    return RP_IS_CONN_POOL_MAP(self) ?
        RP_CONN_POOL_MAP_GET_CLASS(self)->free_one_pool(self) : false;
}
static inline void
rp_conn_pool_map_clear_active_pools(RpConnPoolMap* self)
{
    if (RP_IS_CONN_POOL_MAP(self)) \
        RP_CONN_POOL_MAP_GET_CLASS(self)->clear_active_pools(self);
}

G_END_DECLS
