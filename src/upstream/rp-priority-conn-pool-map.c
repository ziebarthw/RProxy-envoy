/*
 * rp-priority-conn-pool-map.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_priority_conn_pool_map_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_priority_conn_pool_map_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-priority-conn-pool-map.h"

typedef struct _RpPriorityConnPoolMapPrivate RpPriorityConnPoolMapPrivate;
// template <typename KEY_TYPE, typename POOL_TYPE> class PriorityConnPoolMap
// PriorityConnPoolMap<std::vector<uint8_t>, Http::ConnectionPool::Instance>
struct _RpPriorityConnPoolMapPrivate {
//  using ConnPoolMapType = ConnPoolMap<std::vector<uint8_t>, HttpConnectionPool::Instance>;
//  std::array<std::unique_ptr<ConnPoolMapType>, NumResourcePriorities> conn_pool_maps_;
    UNIQUE_PTR(GPtrArray) m_conn_pool_maps;
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(RpPriorityConnPoolMap, rp_priority_conn_pool_map, G_TYPE_OBJECT)

#define PRIV(obj) \
    ((RpPriorityConnPoolMapPrivate*)rp_priority_conn_pool_map_get_instance_private(RP_PRIORITY_CONN_POOL_MAP(obj)))

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpPriorityConnPoolMapPrivate* me = PRIV(obj);
    g_ptr_array_free(g_steal_pointer(&me->m_conn_pool_maps), true);

    G_OBJECT_CLASS(rp_priority_conn_pool_map_parent_class)->dispose(obj);
}

static void
rp_priority_conn_pool_map_class_init(RpPriorityConnPoolMapClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_priority_conn_pool_map_init(RpPriorityConnPoolMap* self)
{
    NOISY_MSG_("(%p)", self);
    RpPriorityConnPoolMapPrivate* me = PRIV(self);
    me->m_conn_pool_maps = g_ptr_array_new_full(RpNumResourcePriorities, (GDestroyNotify)g_hash_table_unref);
}

GPtrArray*
rp_priority_conn_pool_map_conn_pool_maps_(RpPriorityConnPoolMap* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_PRIORITY_CONN_POOL_MAP(self), NULL);
    return PRIV(self)->m_conn_pool_maps;
}
