/*
 * rp-http-conn-pool-base-active-client.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_http_conn_pool_base_active_client_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_http_conn_pool_base_active_client_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-codec-client.h"
#include "rp-net-connection.h"
#include "rp-http-conn-pool-base.h"
#include "rp-http-conn-pool-base-active-client.h"

#define PARENT_NETWORK_CONNECTION_CALLBACKS_IFACE(s) \
    ((RpNetworkConnectionCallbacksInterface*)g_type_interface_peek_parent(RP_NETWORK_CONNECTION_CALLBACKS_GET_IFACE(self)))

typedef struct _RpHttpConnPoolBaseActiveClientPrivate RpHttpConnPoolBaseActiveClientPrivate;
struct _RpHttpConnPoolBaseActiveClientPrivate {

    RpCreateConnectionDataPtr m_opt_data;

    RpCodecClient* m_codec_client;
    RpConnPoolImplBase* m_parent;
};

enum
{
    PROP_0, // Reserved.
    PROP_OPT_DATA,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void network_connection_callbacks_iface_init(RpNetworkConnectionCallbacksInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpHttpConnPoolBaseActiveClient, rp_http_conn_pool_base_active_client, RP_TYPE_CONNECTION_POOL_ACTIVE_CLIENT,
    G_ADD_PRIVATE(RpHttpConnPoolBaseActiveClient)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_CONNECTION_CALLBACKS, network_connection_callbacks_iface_init)
)

#define PRIV(obj) \
    ((RpHttpConnPoolBaseActiveClientPrivate*) rp_http_conn_pool_base_active_client_get_instance_private(RP_HTTP_CONN_POOL_BASE_ACTIVE_CLIENT(obj)))

static void
on_event_i(RpNetworkConnectionCallbacks* self, RpNetworkConnectionEvent_e event)
{
    NOISY_MSG_("(%p, %d)", self, event);
    RpHttpConnPoolBaseActiveClientPrivate* me = PRIV(self);
    rp_conn_pool_impl_base_on_connection_event(me->m_parent,
                                                RP_CONNECTION_POOL_ACTIVE_CLIENT(self),
                                                rp_codec_client_connection_failure_reason(me->m_codec_client),
                                                event);
}

static void
on_above_write_buffer_high_water_mark_i(RpNetworkConnectionCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    PARENT_NETWORK_CONNECTION_CALLBACKS_IFACE(self)->on_above_write_buffer_high_water_mark(self);
}

static void
on_below_write_buffer_low_watermark_i(RpNetworkConnectionCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    PARENT_NETWORK_CONNECTION_CALLBACKS_IFACE(self)->on_below_write_buffer_low_watermark(self);
}

static void
network_connection_callbacks_iface_init(RpNetworkConnectionCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_event = on_event_i;
    iface->on_above_write_buffer_high_water_mark = on_above_write_buffer_high_water_mark_i;
    iface->on_below_write_buffer_low_watermark = on_below_write_buffer_low_watermark_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_OPT_DATA:
            g_value_set_pointer(value, PRIV(obj)->m_opt_data);
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
        case PROP_OPT_DATA:
            PRIV(obj)->m_opt_data = g_value_get_pointer(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
initialize(RpHttpConnPoolBaseActiveClient* self, RpCreateConnectionDataPtr data, RpHttpConnPoolImplBase* parent)
{
    NOISY_MSG_("(%p, %p, %p)", self, data, parent);
    RpHttpConnPoolBaseActiveClientPrivate* me = PRIV(self);
NOISY_MSG_("connection %p, host %p", data->m_connection, data->m_host_description);
    rp_connection_pool_active_client_set_real_host_description(RP_CONNECTION_POOL_ACTIVE_CLIENT(self), data->m_host_description);
    me->m_codec_client = rp_http_conn_pool_impl_base_create_codec_client(parent, data);
    rp_codec_client_add_connection_callbacks(me->m_codec_client, RP_NETWORK_CONNECTION_CALLBACKS(self));
    //TODO...
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_http_conn_pool_base_active_client_parent_class)->constructed(obj);

    RpHttpConnPoolBaseActiveClient* self = RP_HTTP_CONN_POOL_BASE_ACTIVE_CLIENT(obj);
    RpHttpConnPoolBaseActiveClientPrivate* me = PRIV(self);
    me->m_parent = rp_connection_pool_active_client_parent_(RP_CONNECTION_POOL_ACTIVE_CLIENT(self));
NOISY_MSG_("parent %p", me->m_parent);
    if (me->m_opt_data)
    {
        initialize(self, me->m_opt_data, RP_HTTP_CONN_POOL_IMPL_BASE(me->m_parent));
    }
    else
    {
        RpConnPoolImplBase* parent = me->m_parent;
        RpHost* host = rp_conn_pool_impl_base_host(parent);
NOISY_MSG_("host %p", host);
        RpDispatcher* dispatcher = rp_conn_pool_impl_base_dispatcher(parent);
        RpCreateConnectionData data = rp_host_create_connection(host, dispatcher);
        initialize(self, &data, RP_HTTP_CONN_POOL_IMPL_BASE(parent));
    }
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpHttpConnPoolBaseActiveClient* self = RP_HTTP_CONN_POOL_BASE_ACTIVE_CLIENT(obj);
    RpHttpConnPoolBaseActiveClientPrivate* me = PRIV(self);
    g_clear_object(&me->m_codec_client);

    G_OBJECT_CLASS(rp_http_conn_pool_base_active_client_parent_class)->dispose(obj);
}

OVERRIDE guint32
num_active_streams(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_codec_client_num_active_requests(PRIV(self)->m_codec_client);
}

OVERRIDE void
initialize_read_filters(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
    rp_codec_client_initialize_read_filters(PRIV(self)->m_codec_client);
}

OVERRIDE void
close_(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
    rp_codec_client_close_(PRIV(self)->m_codec_client);
}

OVERRIDE void
connect_(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
    rp_codec_client_connect(PRIV(self)->m_codec_client);
}

OVERRIDE void
set_requested_server_name(RpConnectionPoolActiveClient* self, const char* requested_server_name)
{
    NOISY_MSG_("(%p, %p(%s))", self, requested_server_name, requested_server_name);
    rp_codec_client_set_requested_server_name(PRIV(self)->m_codec_client, requested_server_name);
}

static void
conn_pool_base_active_client_class_init(RpConnectionPoolActiveClientClass* klass)
{
    LOGD("(%p)", klass);
    klass->num_active_streams = num_active_streams;
    klass->initialize_read_filters = initialize_read_filters;
    klass->close = close_;
    klass->connect = connect_;
    klass->set_requested_server_name = set_requested_server_name;
}

static void
rp_http_conn_pool_base_active_client_class_init(RpHttpConnPoolBaseActiveClientClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    conn_pool_base_active_client_class_init(RP_CONNECTION_POOL_ACTIVE_CLIENT_CLASS(klass));

    obj_properties[PROP_OPT_DATA] = g_param_spec_pointer("opt-data",
                                                    "Optional data",
                                                    "Optional CreateConnectionData",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_http_conn_pool_base_active_client_init(RpHttpConnPoolBaseActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
    PRIV(self)->m_opt_data = NULL;
}

RpCodecClient*
rp_http_conn_pool_base_active_client_codec_client_(RpHttpConnPoolBaseActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_CONN_POOL_BASE_ACTIVE_CLIENT(self), NULL);
    return PRIV(self)->m_codec_client;
}
