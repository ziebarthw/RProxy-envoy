/*
 * rp-http-pool-data.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_http_pool_data_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_http_pool_data_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-http-pool-data.h"

struct _RpHttpPoolData {
    GObject parent_instance;

    RpOnNewStreamFn m_on_new_stream;
    RpHttpConnectionPoolInstance* m_pool;
    void* m_arg;
};

enum
{
    PROP_0, // Reserved.
    PROP_ON_NEW_STREAM,
    PROP_POOL,
    PROP_ARG,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_FINAL_TYPE(RpHttpPoolData, rp_http_pool_data, G_TYPE_OBJECT)

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_ON_NEW_STREAM:
            g_value_set_pointer(value, RP_HTTP_POOL_DATA(obj)->m_on_new_stream);
            break;
        case PROP_ARG:
            g_value_set_pointer(value, RP_HTTP_POOL_DATA(obj)->m_arg);
            break;
        case PROP_POOL:
            g_value_set_object(value, RP_HTTP_POOL_DATA(obj)->m_pool);
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
        case PROP_ON_NEW_STREAM:
            RP_HTTP_POOL_DATA(obj)->m_on_new_stream = g_value_get_pointer(value);
            break;
        case PROP_ARG:
            RP_HTTP_POOL_DATA(obj)->m_arg = g_value_get_pointer(value);
            break;
        case PROP_POOL:
            RP_HTTP_POOL_DATA(obj)->m_pool = g_value_get_object(value);
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
    G_OBJECT_CLASS(rp_http_pool_data_parent_class)->dispose(obj);
}

static void
rp_http_pool_data_class_init(RpHttpPoolDataClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_ON_NEW_STREAM] = g_param_spec_pointer("on-new-stream",
                                                    "On new stream",
                                                    "On New Stream Callback",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_ARG] = g_param_spec_pointer("arg",
                                                    "Arg",
                                                    "On New Stream Callback Argument",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_POOL] = g_param_spec_object("pool",
                                                    "Pool",
                                                    "ConnectionPool Instance",
                                                    RP_TYPE_HTTP_CONNECTION_POOL_INSTANCE,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_http_pool_data_init(RpHttpPoolData* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpHttpPoolData*
rp_http_pool_data_new(RpOnNewStreamFn on_new_stream, void* arg, RpHttpConnectionPoolInstance* pool)
{
    LOGD("(%p, %p, %p)", on_new_stream, arg, pool);
    return g_object_new(RP_TYPE_HTTP_POOL_DATA,
                        "on-new-stream", on_new_stream,
                        "arg", arg,
                        "pool", pool,
                        NULL);
}

RpCancellable*
rp_http_pool_data_new_stream(RpHttpPoolData* self, RpResponseDecoder* response_decoder,
                                RpHttpConnPoolCallbacks* callbacks, RpHttpConnPoolInstStreamOptionsPtr stream_options)
{
    LOGD("(%p, %p, %p)", self, response_decoder, callbacks);
    g_return_val_if_fail(RP_IS_HTTP_POOL_DATA(self), NULL);
    g_return_val_if_fail(RP_IS_RESPONSE_DECODER(response_decoder), NULL);
    g_return_val_if_fail(RP_IS_HTTP_CONN_POOL_CALLBACKS(callbacks), NULL);
    self->m_on_new_stream(self->m_arg);
    return rp_http_connection_pool_instance_new_stream(self->m_pool, response_decoder, callbacks, stream_options);
}

bool
rp_http_pool_data_has_active_connections(RpHttpPoolData* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_POOL_DATA(self), false);
    return rp_http_connection_pool_instance_has_active_connections(self->m_pool);
}

void
rp_http_pool_data_add_idle_callbacks(RpHttpPoolData* self, idle_cb cb)
{
    LOGD("(%p, %p)", self, cb);
    g_return_if_fail(RP_IS_HTTP_POOL_DATA(self));
    rp_connection_pool_instance_add_idle_callback(RP_CONNECTION_POOL_INSTANCE(self->m_pool), cb);
}

void
rp_http_pool_data_drain_connections(RpHttpPoolData* self, RpDrainBehavior_e drain_behavior)
{
    LOGD("(%p, %d)", self, drain_behavior);
    g_return_if_fail(RP_IS_HTTP_POOL_DATA(self));
    rp_connection_pool_instance_drain_connections(RP_CONNECTION_POOL_INSTANCE(self->m_pool), drain_behavior);
}

RpHostDescription*
rp_http_pool_data_host(RpHttpPoolData* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_POOL_DATA(self), NULL);
    return rp_connection_pool_instance_host(RP_CONNECTION_POOL_INSTANCE(self->m_pool));
}
