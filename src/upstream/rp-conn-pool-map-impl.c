/*
 * rp-conn-pool-map-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_conn_pool_map_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_conn_pool_map_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-http-conn-pool.h"
#include "upstream/rp-conn-pool-map-impl.h"

struct _RpConnPoolMapImpl {
    RpConnPoolMap parent_instance;

};

G_DEFINE_FINAL_TYPE(RpConnPoolMapImpl, rp_conn_pool_map_impl, RP_TYPE_CONN_POOL_MAP)

static inline GHashTable*
active_pools_(RpConnPoolMapImpl* self)
{
    GHashTable* active_pools = NULL;
    g_object_get(G_OBJECT(self), "active-pools", &active_pools, NULL);
    return active_pools;
}

static inline RpDispatcher*
dispatcher_(RpConnPoolMapImpl* self)
{
    RpDispatcher* dispatcher = NULL;
    g_object_get(G_OBJECT(self), "dispatcher", &dispatcher, NULL);
    g_object_unref(dispatcher);
    return dispatcher;
}

static inline RpHost*
host_(RpConnPoolMapImpl* self)
{
    RpHost* host = NULL;
    g_object_get(G_OBJECT(self), "host", &host, NULL);
    g_object_unref(host);
    return host;
}

static inline RpResourcePriority_e
priority_(RpConnPoolMapImpl* self)
{
    RpResourcePriority_e priority = RpResourcePriority_Default;
    g_object_get(G_OBJECT(self), "priority", &priority, NULL);
    return priority;
}

static inline GPtrArray*
callbacks_(RpConnPoolMapImpl* self)
{
    GPtrArray* callbacks = NULL;
    g_object_get(G_OBJECT(self), "callbacks", &callbacks, NULL);
    return callbacks;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpConnPoolMapImpl* self = RP_CONN_POOL_MAP_IMPL(obj);
    rp_conn_pool_map_clear_active_pools(RP_CONN_POOL_MAP(self));

    G_OBJECT_CLASS(rp_conn_pool_map_impl_parent_class)->dispose(obj);
}

OVERRIDE void
clear_active_pools(RpConnPoolMap* self)
{
    NOISY_MSG_("(%p)", self);
    //TODO...host_->cluster().resourceManager(priority_).connectionPools().decBy(active_pools_->size());
    g_hash_table_remove_all(active_pools_(RP_CONN_POOL_MAP_IMPL(self)));
}

OVERRIDE bool
free_one_pool(RpConnPoolMap* self)
{
    NOISY_MSG_("(%p)", self);
    GHashTable* pools = active_pools_(RP_CONN_POOL_MAP_IMPL(self));

    GHashTableIter itr;
    gpointer key;
    gpointer value;

    g_hash_table_iter_init(&itr, pools);
    while (g_hash_table_iter_next(&itr, &key, &value))
    {
        RpHttpConnectionPoolInstance* pool = RP_HTTP_CONNECTION_POOL_INSTANCE(value);
        if (!rp_http_connection_pool_instance_has_active_connections(pool))
        {
            g_hash_table_iter_remove(&itr);
            g_object_unref(G_OBJECT(value));
            //TODO...host_->cluster().resourceManager(priority_).connectionPools().dec();
            return true;
        }
    }

    return false;
}

static void
add_cb_func(gpointer key G_GNUC_UNUSED, gpointer value, gpointer user_data)
{
    NOISY_MSG_("(%p, %p, %p)", key, value, user_data);
    RpConnectionPoolInstance* pool = RP_CONNECTION_POOL_INSTANCE(value);
    rp_connection_pool_instance_add_idle_callback(pool, user_data);
}

OVERRIDE void
add_idle_callback(RpConnPoolMap* self, const RpIdleCb cb)
{
    NOISY_MSG_("(%p, %p)", self, cb);
    GHashTable* pools = active_pools_(RP_CONN_POOL_MAP_IMPL(self));
    g_hash_table_foreach(pools, add_cb_func, cb);
}

OVERRIDE void
clear(RpConnPoolMap* self)
{
    NOISY_MSG_("(%p)", self);

    RpConnPoolMapImpl* me = RP_CONN_POOL_MAP_IMPL(self);
    GHashTable* pools = active_pools_(me);
    RpDispatcher* dispatcher = dispatcher_(me);

    GHashTableIter itr;
    gpointer key;
    gpointer value;

    g_hash_table_iter_init(&itr, pools);
    while (g_hash_table_iter_next(&itr, &key, &value))
    {
        g_hash_table_iter_remove(&itr);
        rp_dispatcher_deferred_delete(dispatcher, G_OBJECT(value));
    }

    rp_conn_pool_map_clear_active_pools(self);
}

OVERRIDE void
drain_connections(RpConnPoolMap* self, RpDrainBehavior_e drain_behavior)
{
    NOISY_MSG_("(%p, %d)", self, drain_behavior);
    GHashTable* pools = active_pools_(RP_CONN_POOL_MAP_IMPL(self));
    g_autoptr(GPtrArray) pools_ = g_hash_table_get_values_as_ptr_array(pools);
    for (guint i=0; i < pools_->len; ++i)
    {
        RpConnectionPoolInstance* pi = RP_CONNECTION_POOL_INSTANCE(g_ptr_array_index(pools_, i));
        rp_connection_pool_instance_drain_connections(pi, drain_behavior);
    }
}

OVERRIDE bool
empty(RpConnPoolMap* self)
{
    NOISY_MSG_("(%p)", self);
    return g_hash_table_size(active_pools_(RP_CONN_POOL_MAP_IMPL(self))) == 0;
}

OVERRIDE bool
erase_pool(RpConnPoolMap* self, gconstpointer key)
{
    NOISY_MSG_("(%p, %p)", self, key);
    RpConnPoolMapImpl* me = RP_CONN_POOL_MAP_IMPL(self);
    GHashTable* pools = active_pools_(me);
    gpointer value = g_hash_table_lookup(pools, key);
    if (value)
    {
        rp_dispatcher_deferred_delete(dispatcher_(me), G_OBJECT(value));
        g_hash_table_remove(pools, key);
        //TODO...host_->cluster().resourceManager(priority_).connectionPools().dec();
        return true;
    }
    return false;
}

OVERRIDE RpHttpConnectionPoolInstancePtr
get_pool(RpConnPoolMap* self, gconstpointer key, const RpPoolFactory factory, gconstpointer user_data)
{
    NOISY_MSG_("(%p, %p, %p, %p)", self, key, factory, user_data);

    RpConnPoolMapImpl* me = RP_CONN_POOL_MAP_IMPL(self);
    GHashTable* pools = active_pools_(me);
    gpointer value = g_hash_table_lookup(pools, key);
    if (value)
    {
        NOISY_MSG_("found %p", value);
        return RP_HTTP_CONNECTION_POOL_INSTANCE(value);
    }

    RpResourceLimit* connPoolResource = rp_resource_manager_connection_pools(
                                            rp_cluster_info_resource_manager(
                                                rp_host_description_cluster(RP_HOST_DESCRIPTION(host_(me))), priority_(me)));
    if (!rp_resource_limit_can_create(connPoolResource))
    {
        if (!rp_conn_pool_map_free_one_pool(self))
        {
            //TODO...host_->cluster().trafficStats()->upstream_cx_pool_overflow_.inc();
            return NULL;
        }
    }

    RpHttpConnectionPoolInstancePtr new_pool = factory((gpointer)user_data);
    rp_resource_limit_inc(connPoolResource);

    GPtrArray* callbacks = callbacks_(me);
    for (guint i=0; i < callbacks->len; ++i)
    {
        RpIdleCb cb = g_ptr_array_index(callbacks, i);
        rp_connection_pool_instance_add_idle_callback(RP_CONNECTION_POOL_INSTANCE(new_pool), cb);
    }

    g_hash_table_insert(pools, (gpointer)key, new_pool);
    return new_pool;
}

OVERRIDE gsize
size(RpConnPoolMap* self)
{
    NOISY_MSG_("(%p)", self);
    return g_hash_table_size(active_pools_(RP_CONN_POOL_MAP_IMPL(self)));
}

static void
conn_pool_map_class_init(RpConnPoolMapClass* klass)
{
    NOISY_MSG_("(%p)", klass);
    klass->clear_active_pools = clear_active_pools;
    klass->free_one_pool = free_one_pool;
    klass->add_idle_callback = add_idle_callback;
    klass->clear = clear;
    klass->drain_connections = drain_connections;
    klass->empty = empty;
    klass->erase_pool = erase_pool;
    klass->get_pool = get_pool;
    klass->size = size;
}

static void
rp_conn_pool_map_impl_class_init(RpConnPoolMapImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    conn_pool_map_class_init(RP_CONN_POOL_MAP_CLASS(klass));
}

static void
rp_conn_pool_map_impl_init(RpConnPoolMapImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpConnPoolMapImpl*
rp_conn_pool_map_impl_new(SHARED_PTR(RpDispatcher) dispatcher, SHARED_PTR(RpHost) host, RpResourcePriority_e priority)
{
    LOGD("(%p, %p, %d)", dispatcher, host, priority);

    g_return_val_if_fail(RP_IS_DISPATCHER(dispatcher), NULL);
    g_return_val_if_fail(RP_IS_HOST(host), NULL);

    return g_object_new(RP_TYPE_CONN_POOL_MAP_IMPL,
                        "dispatcher", dispatcher,
                        "host", host,
                        "priority", priority,
                        NULL);
}
