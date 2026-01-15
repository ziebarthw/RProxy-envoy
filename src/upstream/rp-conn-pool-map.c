/*
 * rp-conn-pool-map.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_conn_pool_map_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_conn_pool_map_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-conn-pool-map.h"

typedef struct _RpConnPoolMapPrivate RpConnPoolMapPrivate;
struct _RpConnPoolMapPrivate {
//  absl::flat_hash_map<std::vector<uint8_t>, HttpConnectionPool::Instance> active_pools_;
    UNIQUE_PTR(GHashTable) m_active_pools;
    SHARED_PTR(RpDispatcher) m_thread_local_dispatcher;
    UNIQUE_PTR(GPtrArray) m_cached_callbacks;
    SHARED_PTR(RpHost) m_host;
    RpResourcePriority_e m_priority;
};

enum
{
    PROP_0, // Reserved.
    PROP_DISPATCHER,
    PROP_HOST,
    PROP_PRIORITY,
    PROP_CALLBACKS,
    PROP_ACTIVE_POOLS,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(RpConnPoolMap, rp_conn_pool_map, G_TYPE_OBJECT)

#define PRIV(obj) \
    ((RpConnPoolMapPrivate*)rp_conn_pool_map_get_instance_private(RP_CONN_POOL_MAP(obj)))

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_DISPATCHER:
            g_value_set_object(value, PRIV(obj)->m_thread_local_dispatcher);
            break;
        case PROP_HOST:
            g_value_set_object(value, PRIV(obj)->m_host);
            break;
        case PROP_PRIORITY:
            g_value_set_int(value, PRIV(obj)->m_priority);
            break;
        case PROP_CALLBACKS:
            g_value_set_pointer(value, PRIV(obj)->m_cached_callbacks);
            break;
        case PROP_ACTIVE_POOLS:
            g_value_set_pointer(value, PRIV(obj)->m_active_pools);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_DISPATCHER:
            PRIV(obj)->m_thread_local_dispatcher = g_value_get_object(value);
            break;
        case PROP_HOST:
            PRIV(obj)->m_host = g_value_get_object(value);
            break;
        case PROP_PRIORITY:
            PRIV(obj)->m_priority = g_value_get_int(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpConnPoolMapPrivate* me = PRIV(obj);
    g_clear_pointer(&me->m_active_pools, g_hash_table_unref);
    g_clear_pointer(&me->m_cached_callbacks, g_ptr_array_unref);

    G_OBJECT_CLASS(rp_conn_pool_map_parent_class)->dispose(obj);
}

static void
rp_conn_pool_map_class_init(RpConnPoolMapClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_DISPATCHER] = g_param_spec_object("dispatcher",
                                                    "Dispatcher",
                                                    "Dispatcher Instance",
                                                    RP_TYPE_DISPATCHER,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_HOST] = g_param_spec_object("host",
                                                    "Host",
                                                    "Host Instance",
                                                    RP_TYPE_HOST,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_PRIORITY] = g_param_spec_int("priority",
                                                    "Priority",
                                                    "Priority",
                                                    RpResourcePriority_Default,
                                                    RpResourcePriority_High,
                                                    RpResourcePriority_Default,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_CALLBACKS] = g_param_spec_pointer("callbacks",
                                                    "Callbacks",
                                                    "Callbacks Instance",
                                                    G_PARAM_READABLE|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_ACTIVE_POOLS] = g_param_spec_pointer("active-pools",
                                                    "Active pools",
                                                    "ConnectionPoolInstance Instance",
                                                    G_PARAM_READABLE|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_conn_pool_map_init(RpConnPoolMap* self)
{
    NOISY_MSG_("(%p)", self);

    RpConnPoolMapPrivate* me = PRIV(self);
    me->m_active_pools = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, NULL);
    me->m_cached_callbacks = g_ptr_array_new();
}
