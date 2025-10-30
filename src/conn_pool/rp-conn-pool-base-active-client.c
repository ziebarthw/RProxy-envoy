/*
 * rp-conn-pool-base-active-client.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_conn_pool_base_active_client_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_conn_pool_base_active_client_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include "rp-conn-pool-base-active-client.h"

typedef struct _RpConnectionPoolActiveClientPrivate RpConnectionPoolActiveClientPrivate;
struct _RpConnectionPoolActiveClientPrivate {

    RpConnPoolImplBase* m_parent;
    RpHostDescription* m_real_host_description;

    RpConnectionPoolActiveClientState_e m_state;

    guint32 m_lifetime_stream_limit;
    guint32 m_effective_concurrent_streams;

    guint32 m_remaining_streams;
    guint32 m_configured_stream_limit;
    guint32 m_concurrent_stream_limit;

    bool m_resources_released : 1;
    bool m_timed_out : 1;
    bool m_has_handshake_completed : 1;
};

enum
{
    PROP_0, // Reserved.
    PROP_PARENT,
    PROP_LIFETIME_STREAM_LIMIT,
    PROP_EFFECTIVE_CONCURRENT_STREAMS,
    PROP_CONCURRENT_STREAM_LIMIT,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void network_connection_callbacks_iface_init(RpNetworkConnectionCallbacksInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpConnectionPoolActiveClient, rp_connection_pool_active_client, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpConnectionPoolActiveClient)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_CONNECTION_CALLBACKS, network_connection_callbacks_iface_init)
)

#define PRIV(obj) \
    ((RpConnectionPoolActiveClientPrivate*) rp_connection_pool_active_client_get_instance_private(RP_CONNECTION_POOL_ACTIVE_CLIENT(obj)))

static void
on_above_write_buffer_high_water_mark_i(RpNetworkConnectionCallbacks* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static void
on_below_write_buffer_low_watermark_i(RpNetworkConnectionCallbacks* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static void
network_connection_callbacks_iface_init(RpNetworkConnectionCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_above_write_buffer_high_water_mark = on_above_write_buffer_high_water_mark_i;
    iface->on_below_write_buffer_low_watermark = on_below_write_buffer_low_watermark_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_PARENT:
            g_value_set_object(value, PRIV(obj)->m_parent);
            break;
        case PROP_LIFETIME_STREAM_LIMIT:
            g_value_set_uint(value, PRIV(obj)->m_lifetime_stream_limit);
            break;
        case PROP_EFFECTIVE_CONCURRENT_STREAMS:
            g_value_set_uint(value, PRIV(obj)->m_effective_concurrent_streams);
            break;
        case PROP_CONCURRENT_STREAM_LIMIT:
            g_value_set_uint(value, PRIV(obj)->m_concurrent_stream_limit);
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
        case PROP_PARENT:
            PRIV(obj)->m_parent = g_value_get_object(value);
            break;
        case PROP_LIFETIME_STREAM_LIMIT:
            PRIV(obj)->m_lifetime_stream_limit = g_value_get_uint(value);
            break;
        case PROP_EFFECTIVE_CONCURRENT_STREAMS:
            PRIV(obj)->m_effective_concurrent_streams = g_value_get_uint(value);
            break;
        case PROP_CONCURRENT_STREAM_LIMIT:
            PRIV(obj)->m_concurrent_stream_limit = g_value_get_uint(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static inline guint32
translate_zero_to_unlimited(guint32 limit)
{
NOISY_MSG_("(%u)", limit);
    return limit != 0 ? limit : G_MAXUINT32;
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_connection_pool_active_client_parent_class)->constructed(obj);

    RpConnectionPoolActiveClientPrivate* me = PRIV(obj);
    me->m_remaining_streams = translate_zero_to_unlimited(me->m_lifetime_stream_limit);
NOISY_MSG_("remaining streams %u", me->m_remaining_streams);
    me->m_configured_stream_limit = translate_zero_to_unlimited(me->m_effective_concurrent_streams);
    me->m_concurrent_stream_limit = translate_zero_to_unlimited(me->m_concurrent_stream_limit);

    //TODO...timer stuff...
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_connection_pool_active_client_parent_class)->dispose(obj);
}

OVERRIDE void
release_resources(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
    rp_connection_pool_active_client_release_resources_base(self);
}

OVERRIDE guint32
effective_concurrent_stream_limit(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
    RpConnectionPoolActiveClientPrivate* me = PRIV(self);
    return MIN(me->m_remaining_streams, me->m_concurrent_stream_limit);
}

OVERRIDE gint64
current_unused_capacity(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
    RpConnectionPoolActiveClientPrivate* me = PRIV(self);
    gint64 remaining_concurrent_streams =
        (gint64)(me->m_concurrent_stream_limit) - rp_connection_pool_active_client_num_active_streams(self);
    return (gint64)MIN(me->m_remaining_streams, remaining_concurrent_streams);
}

OVERRIDE bool
ready_for_stream(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
NOISY_MSG_("%u, %d", PRIV(self)->m_state == RpConnectionPoolActiveClientState_Ready, PRIV(self)->m_state);
    return PRIV(self)->m_state == RpConnectionPoolActiveClientState_Ready;
}

OVERRIDE bool
had_negative_delta_on_stream_closed(RpConnectionPoolActiveClient* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return false;
}

OVERRIDE bool
has_handshake_completed(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_state != RpConnectionPoolActiveClientState_Connecting;
}

OVERRIDE bool
supports_early_data(RpConnectionPoolActiveClient* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return false;
}

OVERRIDE void
drain(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
    gint64 current_unused_capacity = rp_connection_pool_active_client_current_unused_capacity(self);
    if (current_unused_capacity <= 0)
    {
        NOISY_MSG_("returning");
        return;
    }
    RpConnectionPoolActiveClientPrivate* me = PRIV(self);
    rp_conn_pool_impl_base_decr_connecting_and_connected_stream_capacity(me->m_parent, current_unused_capacity, self);

    me->m_remaining_streams = 0;
}

static void
rp_connection_pool_active_client_class_init(RpConnectionPoolActiveClientClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    klass->release_resources = release_resources;
    klass->effective_concurrent_stream_limit = effective_concurrent_stream_limit;
    klass->current_unused_capacity = current_unused_capacity;
    klass->ready_for_stream = ready_for_stream;
    klass->had_negative_delta_on_stream_closed = had_negative_delta_on_stream_closed;
    klass->has_handshake_completed = has_handshake_completed;
    klass->supports_early_data = supports_early_data;
    klass->drain = drain;

    obj_properties[PROP_PARENT] = g_param_spec_object("parent",
                                                    "Parent",
                                                    "Parent ConnPoolImplBase Instance",
                                                    RP_TYPE_CONN_POOL_IMPL_BASE,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_LIFETIME_STREAM_LIMIT] = g_param_spec_uint("lifetime-stream-limit",
                                                    "Lifetime stream limit",
                                                    "Lifetime Stream Limit",
                                                    0,
                                                    G_MAXUINT32,
                                                    0,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_EFFECTIVE_CONCURRENT_STREAMS] = g_param_spec_uint("effective-concurrent-streams",
                                                    "Effective concurrent streams",
                                                    "Effective Concurrent Streams",
                                                    0,
                                                    G_MAXUINT32,
                                                    0,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_CONCURRENT_STREAM_LIMIT] = g_param_spec_uint("concurrent-stream-limit",
                                                    "Concurrent stream limit",
                                                    "Concurrent Stream Limit",
                                                    0,
                                                    G_MAXUINT32,
                                                    0,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_connection_pool_active_client_init(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p)", self);

    RpConnectionPoolActiveClientPrivate* me = PRIV(self);
    me->m_state = RpConnectionPoolActiveClientState_Connecting;
    me->m_resources_released = false;
    me->m_timed_out = false;
    me->m_has_handshake_completed = false;
}

void
rp_connection_pool_active_client_release_resources_base(RpConnectionPoolActiveClient* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self));
    RpConnectionPoolActiveClientPrivate* me = PRIV(self);
    if (!me->m_resources_released)
    {
        me->m_resources_released = true;

        //TODO...conn_length_->complete(); //total length (duration) of conn.

        //TODO...parent_.host()->...
    }
}

RpConnectionPoolActiveClientState_e
rp_connection_pool_active_client_state(RpConnectionPoolActiveClient* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self), RpConnectionPoolActiveClientState_Closed);
    return PRIV(self)->m_state;
}

void
rp_connection_pool_active_client_on_connect_timeout(RpConnectionPoolActiveClient* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self));
    RpConnectionPoolActiveClientPrivate* me = PRIV(self);
    //TODO...parent_.host()->cluster()...
    me->m_timed_out = true;
    rp_connection_pool_active_client_close(self);
}

void
rp_connection_pool_active_client_on_connection_duration_timeout(RpConnectionPoolActiveClient* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self));
    RpConnectionPoolActiveClientPrivate* me = PRIV(self);
    if (!rp_connection_pool_active_client_has_handshake_completed(self))
    {
        LOGD("max connection duration reached while connecting");
        return;
    }

    if (me->m_state == RpConnectionPoolActiveClientState_Closed)
    {
        LOGD("max connection duration reached while closed");
        return;
    }

    if (me->m_state == RpConnectionPoolActiveClientState_Draining)
    {
        NOISY_MSG_("draining");
        return;
    }

    LOGD("max connection duration reached, start draining");
    //TODO...parent_.host()->cluster()...
    rp_conn_pool_impl_base_transition_active_client_state(me->m_parent,
                                                            self,
                                                            RpConnectionPoolActiveClientState_Draining);
}

void
rp_connection_pool_active_client_set_state(RpConnectionPoolActiveClient* self,
                                            RpConnectionPoolActiveClientState_e state)
{
    LOGD("(%p, %d)", self, state);
    g_return_if_fail(RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self));
    RpConnectionPoolActiveClientPrivate* me = PRIV(self);
    if (state == RpConnectionPoolActiveClientState_ReadyForEarlyData &&
        !rp_connection_pool_active_client_supports_early_data(self))
    {
        LOGE("Unable to set state to ReadyForEarlyData in a client which does not support early data.");
        return;
    }
    if (state == RpConnectionPoolActiveClientState_Draining)
    {
        NOISY_MSG_("draining");
        rp_connection_pool_active_client_drain(self);
    }
    me->m_state = state;
}

RpHostDescription*
rp_connection_pool_active_client_get_real_host_description(RpConnectionPoolActiveClient* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self), NULL);
    return PRIV(self)->m_real_host_description;
}

void
rp_connection_pool_active_client_set_real_host_description(RpConnectionPoolActiveClient* self, RpHostDescription* host_desc)
{
    LOGD("(%p, %p)", self, host_desc);
    g_return_if_fail(RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self));
    PRIV(self)->m_real_host_description = host_desc;
}

guint32
rp_connection_pool_active_client_decr_remaining_streams(RpConnectionPoolActiveClient* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self), 0);
    RpConnectionPoolActiveClientPrivate* me = PRIV(self);
    --me->m_remaining_streams;
    return me->m_remaining_streams;
}

void
rp_connection_pool_active_client_set_remaining_streams(RpConnectionPoolActiveClient* self, guint32 count)
{
    LOGD("(%p, %u)", self, count);
    g_return_if_fail(RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self));
    PRIV(self)->m_remaining_streams = count;
}

void
rp_connection_pool_active_client_set_has_handshake_completed(RpConnectionPoolActiveClient* self, bool v)
{
    LOGD("(%p, %u)", self, v);
    g_return_if_fail(RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self));
    PRIV(self)->m_has_handshake_completed = v;
}
