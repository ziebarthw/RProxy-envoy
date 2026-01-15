/*
 * rp-tcp-pool-data.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_tcp_pool_data_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_tcp_pool_data_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-tcp-pool-data.h"

struct _RpTcpPoolData {
    GObject parent_instance;

    RpOnNewConnectionFn m_on_new_connection;
    RpTcpConnPoolInstance* m_pool;
    gpointer m_arg;
};

enum
{
    PROP_0, // Reserved.
    PROP_ON_NEW_CONNECTION,
    PROP_POOL,
    PROP_ARG,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_FINAL_TYPE(RpTcpPoolData, rp_tcp_pool_data, G_TYPE_OBJECT)

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_ON_NEW_CONNECTION:
            g_value_set_pointer(value, RP_TCP_POOL_DATA(obj)->m_on_new_connection);
            break;
        case PROP_ARG:
            g_value_set_pointer(value, RP_TCP_POOL_DATA(obj)->m_arg);
            break;
        case PROP_POOL:
            g_value_set_object(value, RP_TCP_POOL_DATA(obj)->m_pool);
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
        case PROP_ON_NEW_CONNECTION:
            RP_TCP_POOL_DATA(obj)->m_on_new_connection = g_value_get_pointer(value);
            break;
        case PROP_ARG:
            RP_TCP_POOL_DATA(obj)->m_arg = g_value_get_pointer(value);
            break;
        case PROP_POOL:
            RP_TCP_POOL_DATA(obj)->m_pool = g_value_get_object(value);
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
    G_OBJECT_CLASS(rp_tcp_pool_data_parent_class)->dispose(obj);
}

static void
rp_tcp_pool_data_class_init(RpTcpPoolDataClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_ON_NEW_CONNECTION] = g_param_spec_pointer("on-new-connection",
                                                    "On new connection",
                                                    "On New Connection Callback",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_ARG] = g_param_spec_pointer("arg",
                                                    "Arg",
                                                    "On New Stream Callback Argument",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_POOL] = g_param_spec_object("pool",
                                                    "Pool",
                                                    "ConnectionPool Instance",
                                                    RP_TYPE_TCP_CONN_POOL_INSTANCE,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_tcp_pool_data_init(RpTcpPoolData* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpTcpPoolData*
rp_tcp_pool_data_new(RpOnNewConnectionFn on_new_connection, void* arg, RpTcpConnPoolInstance* pool)
{
    LOGD("(%p, %p, %p)", on_new_connection, arg, pool);
    return g_object_new(RP_TYPE_TCP_POOL_DATA,
                        "on-new-connection", on_new_connection,
                        "arg", arg,
                        "pool", pool,
                        NULL);
}

RpCancellable*
rp_tcp_pool_data_new_connection(RpTcpPoolData* self, RpTcpConnPoolCallbacks* callbacks)
{
    LOGD("(%p, %p)", self, callbacks);
    g_return_val_if_fail(RP_IS_TCP_POOL_DATA(self), NULL);
    g_return_val_if_fail(RP_IS_TCP_CONN_POOL_CALLBACKS(callbacks), NULL);
    self->m_on_new_connection(self->m_arg, self->m_pool);
    return rp_tcp_conn_pool_instance_new_connection(self->m_pool, callbacks);
}

RpHostDescription*
rp_tcp_pool_data_host(RpTcpPoolData* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_TCP_POOL_DATA(self), NULL);
    return rp_connection_pool_instance_host(RP_CONNECTION_POOL_INSTANCE(self->m_pool));
}