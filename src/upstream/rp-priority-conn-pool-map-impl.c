/*
 * rp-priority-conn-pool-map-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_priority_conn_pool_map_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_priority_conn_pool_map_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-conn-pool-map-impl.h"
#include "upstream/rp-priority-conn-pool-map-impl.h"

struct _RpPriorityConnPoolMapImpl {
    RpPriorityConnPoolMap parent_instance;

};

G_DEFINE_FINAL_TYPE(RpPriorityConnPoolMapImpl, rp_priority_conn_pool_map_impl, RP_TYPE_PRIORITY_CONN_POOL_MAP)

static inline guint
get_priority_index(RpResourcePriority_e priority)
{
    guint index = (guint)priority;
    g_assert(index < RpNumResourcePriorities);
    return index;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_priority_conn_pool_map_impl_parent_class)->dispose(obj);
}

OVERRIDE RpHttpConnectionPoolInstance*
get_pool(RpPriorityConnPoolMap* self, RpResourcePriority_e priority, GBytes* key, RpPoolFactory factory, gpointer user_data)
{
    NOISY_MSG_("(%p, %d, %p, %p, %p)", self, priority, key, factory, user_data);
    RpConnPoolMap** conn_pool_maps_ = rp_priority_conn_pool_map_conn_pool_maps_(self);
    RpConnPoolMap* pool_map = conn_pool_maps_[priority];
    return rp_conn_pool_map_get_pool(pool_map, key, factory, user_data);
}

OVERRIDE bool
erase_pool(RpPriorityConnPoolMap* self, RpResourcePriority_e priority, GBytes* key)
{
    NOISY_MSG_("(%p, %d, %p)", self, priority, key);
    RpConnPoolMap** conn_pool_maps_ = rp_priority_conn_pool_map_conn_pool_maps_(self);
    RpConnPoolMap* pool_map = conn_pool_maps_[get_priority_index(priority)];
    return rp_conn_pool_map_erase_pool(pool_map, key);
}

OVERRIDE gsize
size(RpPriorityConnPoolMap* self)
{
    NOISY_MSG_("(%p)", self);
    gsize size = 0;
    RpConnPoolMap** conn_pool_maps_ = rp_priority_conn_pool_map_conn_pool_maps_(self);
    for (guint i = 0; i < RpNumResourcePriorities; ++i)
    {
        RpConnPoolMap* pool_map = conn_pool_maps_[i];
        size += rp_conn_pool_map_size(pool_map);
    }
    return size;
}

OVERRIDE bool
empty(RpPriorityConnPoolMap* self)
{
    NOISY_MSG_("(%p)", self);
    RpConnPoolMap** conn_pool_maps_ = rp_priority_conn_pool_map_conn_pool_maps_(self);
    for (guint i = 0; i < RpNumResourcePriorities; ++i)
    {
        RpConnPoolMap* pool_map = conn_pool_maps_[i];
        if (!rp_conn_pool_map_empty(pool_map))
        {
            NOISY_MSG_("nope");
            return false;
        }
    }
    NOISY_MSG_("yep");
    return true;
}

OVERRIDE void
clear(RpPriorityConnPoolMap* self)
{
    NOISY_MSG_("(%p)", self);
    RpConnPoolMap** conn_pool_maps_ = rp_priority_conn_pool_map_conn_pool_maps_(self);
    for (guint i = 0; i < RpNumResourcePriorities; ++i)
    {
        RpConnPoolMap* pool_map = conn_pool_maps_[i];
        rp_conn_pool_map_clear(pool_map);
    }
}

OVERRIDE void
add_idle_callback(RpPriorityConnPoolMap* self, const RpIdleCb cb)
{
    NOISY_MSG_("(%p, %p)", self, cb);
    RpConnPoolMap** conn_pool_maps_ = rp_priority_conn_pool_map_conn_pool_maps_(self);
    for (guint i = 0; i < RpNumResourcePriorities; ++i)
    {
        RpConnPoolMap* pool_map = conn_pool_maps_[i];
        rp_conn_pool_map_add_idle_callback(pool_map, cb);
    }
}

OVERRIDE void
drain_connections(RpPriorityConnPoolMap* self, RpDrainBehavior_e drain_behavior)
{
    NOISY_MSG_("(%p, %d)", self, drain_behavior);
    RpConnPoolMap** conn_pool_maps_ = rp_priority_conn_pool_map_conn_pool_maps_(self);
    for (guint i = 0; i < RpNumResourcePriorities; ++i)
    {
        RpConnPoolMap* pool_map = conn_pool_maps_[i];
        rp_conn_pool_map_drain_connections(pool_map, drain_behavior);
    }
}

static void
priority_conn_pool_map_class_init(RpPriorityConnPoolMapClass* klass)
{
    LOGD("(%p)", klass);
    klass->get_pool = get_pool;
    klass->erase_pool = erase_pool;
    klass->size = size;
    klass->empty = empty;
    klass->clear = clear;
    klass->add_idle_callback = add_idle_callback;
    klass->drain_connections = drain_connections;
}

static void
rp_priority_conn_pool_map_impl_class_init(RpPriorityConnPoolMapImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    priority_conn_pool_map_class_init(RP_PRIORITY_CONN_POOL_MAP_CLASS(klass));
}

static void
rp_priority_conn_pool_map_impl_init(RpPriorityConnPoolMapImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpPriorityConnPoolMapImpl*
constructed(RpPriorityConnPoolMapImpl* self, RpDispatcher* dispatcher, RpHost* host)
{
    NOISY_MSG_("(%p, %p, %p)", self, dispatcher, host);

    RpConnPoolMap** conn_pool_maps_ = rp_priority_conn_pool_map_conn_pool_maps_(RP_PRIORITY_CONN_POOL_MAP(self));
    for (gsize pool_map_index=0; pool_map_index < RpNumResourcePriorities; ++pool_map_index)
    {
        RpResourcePriority_e priority = (RpResourcePriority_e)pool_map_index;
        conn_pool_maps_[pool_map_index] = RP_CONN_POOL_MAP(rp_conn_pool_map_impl_new(dispatcher, host, priority));
    }
    return self;
}

RpPriorityConnPoolMapImpl*
rp_priority_conn_pool_map_impl_new(RpDispatcher* dispatcher, RpHost* host)
{
    LOGD("(%p, %p)", dispatcher, host);
    g_return_val_if_fail(RP_IS_DISPATCHER(dispatcher), NULL);
    g_return_val_if_fail(rp_host_is_a(host), NULL);
    return constructed(g_object_new(RP_TYPE_PRIORITY_CONN_POOL_MAP_IMPL, NULL), dispatcher, host);
}
