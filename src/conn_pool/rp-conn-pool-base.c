/*
 * rp-conn-pool-base.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_conn_pool_base_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_conn_pool_base_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "tcp/rp-active-tcp-client.h"
#include "rp-router.h"
#include "rp-upstream.h"
#include "rp-pending-stream.h"
#include "rp-conn-pool-base-active-client.h"
#include "rp-http-conn-pool-base.h"
#include "rp-http-conn-pool-base-active-client.h"
#include "rp-conn-pool-base.h"

typedef struct _RpConnPoolImplBasePrivate RpConnPoolImplBasePrivate;
struct _RpConnPoolImplBasePrivate {

    RpHost* m_host;
    RpDispatcher* m_dispatcher;

const char* m_requested_server_name;

    RpResourcePriority_e m_priority;

    GList/*<PendingStreamPtr>*/* m_pending_streams_to_purge;
    GList/*<ActiveClientPtr>*/* m_ready_clients;
    GList/*<ActiveClientPtr>*/* m_busy_clients;
    GList/*<ActiveClientPtr>*/* m_connecting_clients;
    GList/*<ActiveClientPtr>*/* m_early_data_clients;
    GList/*<PendingStreamPtr>*/* m_pending_streams;

    GSList/*<Instance::IdleCb>*/* m_idle_callbacks;

    UNIQUE_PTR(RpSchedulableCallback) m_upstream_ready_cb;

    gint64 m_connecting_and_connected_stream_capacity;

    guint32 m_num_active_streams;
    guint32 m_connecting_stream_capacity;

    bool m_is_draining_for_deletion : 1;
    bool m_deferred_deleting : 1;
};

enum
{
    PROP_0, // Reserved.
    PROP_HOST,
    PROP_DISPATCHER,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void connection_pool_instance_iface_init(RpConnectionPoolInstanceInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpConnPoolImplBase, rp_conn_pool_impl_base, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpConnPoolImplBase)
    G_IMPLEMENT_INTERFACE(RP_TYPE_CONNECTION_POOL_INSTANCE, connection_pool_instance_iface_init)
)

#define PRIV(obj) \
    ((RpConnPoolImplBasePrivate*) rp_conn_pool_impl_base_get_instance_private(RP_CONN_POOL_IMPL_BASE(obj)))

static void
schedule_upstream_ready(RpSchedulableCallback* self G_GNUC_UNUSED, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", self, arg);
    rp_conn_pool_impl_base_on_upstream_ready(RP_CONN_POOL_IMPL_BASE(arg));
}

static RpHostDescription*
host_i(RpConnectionPoolInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HOST_DESCRIPTION(PRIV(self)->m_host);
}

static void
connection_pool_instance_iface_init(RpConnectionPoolInstanceInterface* iface)
{
    LOGD("(%p)", iface);
    iface->host = host_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_HOST:
            g_value_set_object(value, PRIV(obj)->m_host);
            break;
        case PROP_DISPATCHER:
            g_value_set_pointer(value, PRIV(obj)->m_dispatcher);
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
        case PROP_HOST:
            PRIV(obj)->m_host = g_value_get_object(value);
            break;
        case PROP_DISPATCHER:
            PRIV(obj)->m_dispatcher = g_value_get_pointer(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_conn_pool_impl_base_parent_class)->constructed(obj);

    RpConnPoolImplBasePrivate* me = PRIV(obj);

    me->m_upstream_ready_cb = rp_dispatcher_create_schedulable_callback(me->m_dispatcher, schedule_upstream_ready, obj);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpConnPoolImplBasePrivate* me = PRIV(obj);
    g_clear_object(&me->m_upstream_ready_cb);

    G_OBJECT_CLASS(rp_conn_pool_impl_base_parent_class)->dispose(obj);
}

OVERRIDE bool
track_stream_capacity(RpConnPoolImplBase* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return true;
}

static inline RpClusterInfo*
get_cluster_info(RpConnPoolImplBasePrivate* me)
{
    return rp_host_description_cluster(RP_HOST_DESCRIPTION(me->m_host));
}

static inline RpResourceManager*
get_resource_manager(RpConnPoolImplBasePrivate* me)
{
    return rp_cluster_info_resource_manager(get_cluster_info(me), me->m_priority);
}

static inline RpResourceLimit*
get_requests(RpConnPoolImplBasePrivate* me)
{
    return rp_resource_manager_requests(get_resource_manager(me));
}

static inline RpResourceLimit*
get_pending_requests(RpConnPoolImplBasePrivate* me)
{
    return rp_resource_manager_pending_requests(get_resource_manager(me));
}

OVERRIDE void
attach_stream_to_client(RpConnPoolImplBase* self, RpConnectionPoolActiveClient* client, RpConnectionPoolAttachContextPtr context)
{
    NOISY_MSG_("(%p, %p, %p)", self, client, context);
    //TODO...

    RpConnPoolImplBasePrivate* me = PRIV(self);
    if (rp_conn_pool_impl_base_enforce_max_requests(self) &&
        !rp_resource_limit_can_create(get_requests(me)))
    {
        LOGD("max streams overflow");
        RpHostDescription* real_host_description = rp_connection_pool_active_client_get_real_host_description(client);
        rp_conn_pool_impl_base_on_pool_failure(self,
                                                real_host_description,
                                                "",
                                                RpPoolFailureReason_Overflow,
                                                context);
        //TODO...
        return;
    }

    NOISY_MSG_("creating stream");

    gint64 capacity = rp_connection_pool_active_client_current_unused_capacity(client);
    guint32 remaining_streams = rp_connection_pool_active_client_decr_remaining_streams(client);
    if (remaining_streams == 0)
    {
        LOGD("maximum streams per connection; start draining");
        //TODO...traffic_stats.upstream_cx_max_requests_.inc();
        rp_conn_pool_impl_base_transition_active_client_state(self, client, RpConnectionPoolActiveClientState_Draining);
    }
    else if (capacity == 1)
    {
        NOISY_MSG_("setting to busy");
        rp_conn_pool_impl_base_transition_active_client_state(self, client, RpConnectionPoolActiveClientState_Busy);
    }

    if (rp_conn_pool_impl_base_track_stream_capacity(self))
    {
        NOISY_MSG_("calling rp_conn_pool_impl_base_decr_connecting_and_connected_stream_capacity()");
        rp_conn_pool_impl_base_decr_connecting_and_connected_stream_capacity(self, 1, client);
    }
    //TODO...state_.incrActiveStreams(1);
    ++me->m_num_active_streams;
    //TODO...
    rp_resource_limit_inc(get_requests(me));

    NOISY_MSG_("calling rp_conn_pool_impl_base_on_pool_ready(%p, %p, %p)", self, client, context);
    rp_conn_pool_impl_base_on_pool_ready(self, client, context);
}

OVERRIDE bool
enforce_max_requests(RpConnPoolImplBase* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return true;
}

OVERRIDE void
on_connected(RpConnPoolImplBase* self G_GNUC_UNUSED, RpConnectionPoolActiveClient* client G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p)", self, client);
}

OVERRIDE void
on_connect_failed(RpConnPoolImplBase* self G_GNUC_UNUSED, RpConnectionPoolActiveClient* client G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p)", self, client);
}

static void
rp_conn_pool_impl_base_class_init(RpConnPoolImplBaseClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    klass->track_stream_capacity = track_stream_capacity;
    klass->attach_stream_to_client = attach_stream_to_client;
    klass->enforce_max_requests = enforce_max_requests;
    klass->on_connected = on_connected;
    klass->on_connect_failed = on_connect_failed;

    obj_properties[PROP_HOST] = g_param_spec_object("host",
                                                    "Host",
                                                    "Upstream Host Instance",
                                                    RP_TYPE_HOST,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_DISPATCHER] = g_param_spec_pointer("dispatcher",
                                                    "Dispatcher",
                                                    "Dispatcher (evhtp_t) Instance",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_conn_pool_impl_base_init(RpConnPoolImplBase* self)
{
    NOISY_MSG_("(%p)", self);

    RpConnPoolImplBasePrivate* me = PRIV(self);
    me->m_connecting_stream_capacity = 0;
    me->m_connecting_and_connected_stream_capacity = 0;
    me->m_num_active_streams = 0;
    me->m_is_draining_for_deletion = false;
    me->m_deferred_deleting = false;
}

static inline GList**
owning_list(RpConnPoolImplBasePrivate* me, RpConnectionPoolActiveClientState_e state)
{
    NOISY_MSG_("(%d)", state);
    switch (state)
    {
        case RpConnectionPoolActiveClientState_Connecting:
            return &me->m_connecting_clients;
        case RpConnectionPoolActiveClientState_ReadyForEarlyData:
            return &me->m_early_data_clients;
        case RpConnectionPoolActiveClientState_Ready:
            return &me->m_ready_clients;
        case RpConnectionPoolActiveClientState_Busy:
        case RpConnectionPoolActiveClientState_Draining:
            return &me->m_busy_clients;
        case RpConnectionPoolActiveClientState_Closed:
            LOGE("RpConnectionPoolActiveClientState_Closed");
        default:
            LOGE("unexpected");
            break;
    }
    return NULL;
}

void
rp_conn_pool_impl_base_transition_active_client_state(RpConnPoolImplBase* self,
                                                        RpConnectionPoolActiveClientPtr client,
                                                        RpConnectionPoolActiveClientState_e new_state)
{
    LOGD("(%p, %p, %d)", self, client, new_state);
    g_return_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self));
    RpConnPoolImplBasePrivate* me = PRIV(self);
    GList** old_list = owning_list(me, rp_connection_pool_active_client_state(client));
    GList** new_list = owning_list(me, new_state);
    rp_connection_pool_active_client_set_state(client, new_state);

    if (*old_list != *new_list)
    {
        GList* entry = g_list_find(*old_list, client);
        *old_list = g_list_remove_link(*old_list, entry);
        *new_list = g_list_concat(*new_list, entry);
    }
}

static inline bool
should_connect(gsize pending_streams, gsize active_streams,
                gint64 connecting_and_connected_capacity, float preconnect_ratio, bool anticipate_incoming_stream)
{
    NOISY_MSG_("(%zu, %zu, %zd, %.1f, %u)",
        pending_streams, active_streams, connecting_and_connected_capacity,
        preconnect_ratio, anticipate_incoming_stream);
    int anticipated_streams = anticipate_incoming_stream ? 1 : 0;

    return (pending_streams + active_streams + anticipated_streams) * preconnect_ratio >
            connecting_and_connected_capacity + active_streams;
}

static bool
should_create_new_connection(RpConnPoolImplBase* self, float global_preconnect_ratio)
{
    NOISY_MSG_("(%p, %.1f)", self, global_preconnect_ratio);
    //TODO...health check

    RpConnPoolImplBasePrivate* me = PRIV(self);
    if (global_preconnect_ratio != 0)
    {
        bool result = should_connect(g_list_length(me->m_pending_streams), me->m_num_active_streams,
            me->m_connecting_and_connected_stream_capacity, global_preconnect_ratio, true);
        return result;
    }
    else
    {
        bool result = should_connect(g_list_length(me->m_pending_streams), me->m_num_active_streams,
            me->m_connecting_and_connected_stream_capacity, rp_conn_pool_impl_base_per_upstream_preconnect_ratio(self), false);
        return result;
    }
}

static RpConnectionResult_e
try_create_new_connection(RpConnPoolImplBase* self, float global_preconnect_ratio)
{
    NOISY_MSG_("(%p, %.2f)", self, global_preconnect_ratio);
    if (!should_create_new_connection(self, global_preconnect_ratio))
    {
        NOISY_MSG_("returning");
        return RpConnectionResult_ShouldNotConnect;
    }
    NOISY_MSG_("creating new preconnect connection");

    RpConnPoolImplBasePrivate* me = PRIV(self);
    RpHostDescription* host_desc = RP_HOST_DESCRIPTION(me->m_host);
    bool can_create_connection = rp_host_description_can_create_connection(host_desc, me->m_priority);

    if (!can_create_connection)
    {
        NOISY_MSG_("can't create new connection");
        //TODO...host_->cluster().trafficStats()...
    }
    if (can_create_connection ||
        (!me->m_ready_clients && !me->m_busy_clients && !me->m_connecting_clients && !me->m_early_data_clients))
    {
        NOISY_MSG_("creating a new connection");
        RpConnectionPoolActiveClient* client = RP_CONN_POOL_IMPL_BASE_GET_CLASS(self)->instantiate_active_client(self);
        if (!client)
        {
            LOGE("connection creation failed");
            return RpConnectionResult_FailedToCreateConnection;
        }

        NOISY_MSG_("client %p", client);

        if (me->m_requested_server_name)
        {
            NOISY_MSG_("requested server name %p(%s)", me->m_requested_server_name, me->m_requested_server_name);
            rp_connection_pool_active_client_set_requested_server_name(client, me->m_requested_server_name);
        }
        rp_connection_pool_active_client_connect(client);

        gint64 current_unused_capacity = rp_connection_pool_active_client_current_unused_capacity(client);
        NOISY_MSG_("current unused capacity %zd", current_unused_capacity);
        rp_conn_pool_impl_base_incr_connecting_and_connected_stream_capacity(self, current_unused_capacity, client);
        GList** listp = owning_list(me, rp_connection_pool_active_client_state(client));
        *listp = g_list_prepend(*listp, client);
        //TODO...assertCapacityCountsAreCorrect();
        return can_create_connection ? RpConnectionResult_CreatedNewConnection :
                                        RpConnectionResult_CreatedButRateLimited;
    }
    else
    {
        LOGI("not creating a new connection: connection constrained");
        return RpConnectionResult_NoConnectionRateLimited;
    }
}

RpConnectionResult_e
rp_conn_pool_impl_base_try_create_new_connections(RpConnPoolImplBase* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self), RpConnectionResult_FailedToCreateConnection);
    RpConnectionResult_e result;
    for (int i=0; i < 3; ++i)
    {
        result = try_create_new_connection(self, 1.0);
        if (result != RpConnectionResult_CreatedNewConnection)
        {
            NOISY_MSG_("breaking");
            break;
        }
    }
    return result;
}

void
rp_conn_pool_impl_base_on_upstream_ready(RpConnPoolImplBase* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self));
    RpConnPoolImplBasePrivate* me = PRIV(self);
    while (me->m_pending_streams && me->m_ready_clients)
    {
        RpConnectionPoolActiveClient* client = RP_CONNECTION_POOL_ACTIVE_CLIENT(me->m_ready_clients->data);
        GList* pending = g_list_last(me->m_pending_streams);
        RpPendingStream* stream = RP_PENDING_STREAM(pending->data);
        RpConnectionPoolAttachContextPtr context = rp_pending_stream_context(stream);
        rp_conn_pool_impl_base_attach_stream_to_client(self, client, context);
        //TODO...state_.decrPendingStreams(1);
        me->m_pending_streams = g_list_delete_link(me->m_pending_streams, pending);
    }
    if (me->m_pending_streams)
    {
        rp_conn_pool_impl_base_try_create_new_connections(self);
    }
}

void
rp_conn_pool_impl_base_decr_cluster_stream_capacity(RpConnPoolImplBase* self,
                                                            guint32 delta)
{
    NOISY_MSG_("(%p, %u)", self, delta);
    g_return_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self));
    RpConnPoolImplBasePrivate* me = PRIV(self);
    //TODO...state_.decrConnectingAndConnectedStreamCapacity(delta);
    me->m_connecting_and_connected_stream_capacity -= delta;
}

void
rp_conn_pool_impl_base_incr_cluster_stream_capacity(RpConnPoolImplBase* self,
                                                            guint32 delta)
{
    NOISY_MSG_("(%p, %u)", self, delta);
    g_return_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self));
    RpConnPoolImplBasePrivate* me = PRIV(self);
    //TODO...state_.incrConnectingAndConnectedStreamCapacity(delta);
    me->m_connecting_and_connected_stream_capacity += delta;
}

void
rp_conn_pool_impl_base_decr_connecting_and_connected_stream_capacity(RpConnPoolImplBase* self,
                                                                    guint32 delta,
                                                                    RpConnectionPoolActiveClient* client)
{
    NOISY_MSG_("(%p, %u, %p)", self, delta, client);
    g_return_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self));
    RpConnPoolImplBasePrivate* me = PRIV(self);
    rp_conn_pool_impl_base_decr_cluster_stream_capacity(self, delta);

    if (!rp_connection_pool_active_client_has_handshake_completed(client))
    {
        me->m_connecting_stream_capacity -= delta;
    }
}

void
rp_conn_pool_impl_base_incr_connecting_and_connected_stream_capacity(RpConnPoolImplBase* self,
                                                                    guint32 delta,
                                                                    RpConnectionPoolActiveClient* client)
{
    NOISY_MSG_("(%p, %u, %p)", self, delta, client);
    g_return_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self));
    RpConnPoolImplBasePrivate* me = PRIV(self);
    rp_conn_pool_impl_base_incr_cluster_stream_capacity(self, delta);

    if (!rp_connection_pool_active_client_has_handshake_completed(client))
    {
        me->m_connecting_stream_capacity += delta;
    }
}

RpHost*
rp_conn_pool_impl_base_host(RpConnPoolImplBase* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self), NULL);
    return PRIV(self)->m_host;
}

RpResourcePriority_e
rp_conn_pool_impl_base_priority(RpConnPoolImplBase* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self), RpResourcePriority_Default);
    return PRIV(self)->m_priority;
}

float
rp_conn_pool_impl_base_per_upstream_preconnect_ratio(RpConnPoolImplBase* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self), 1.0);
    return rp_cluster_info_per_upstream_preconnect_ratio(get_cluster_info(PRIV(self)));
}

bool
rp_conn_pool_impl_base_connecting_connection_is_excess(RpConnPoolImplBase* self, RpConnectionPoolActiveClient* client)
{
    LOGD("(%p, %p)", self, client);
    g_return_val_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self), false);
    g_return_val_if_fail(RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(client), false);
    RpConnPoolImplBasePrivate* me = PRIV(self);
    return (g_list_length(me->m_pending_streams) + me->m_num_active_streams) *
            rp_conn_pool_impl_base_per_upstream_preconnect_ratio(self) <=
            (me->m_connecting_stream_capacity - rp_connection_pool_active_client_current_unused_capacity(client) + me->m_num_active_streams);
}

static void
close_idle_connection_for_draining_pool(RpConnPoolImplBase* self)
{
    NOISY_MSG_("(%p)", self);

    g_autoptr(GList) to_close = NULL;

    RpConnPoolImplBasePrivate* me = PRIV(self);
    for (GList* entry = me->m_ready_clients; entry; entry = entry->next)
    {
        RpConnectionPoolActiveClient* client = RP_CONNECTION_POOL_ACTIVE_CLIENT(entry->data);
        if (rp_connection_pool_active_client_num_active_streams(client) == 0)
        {
            to_close = g_list_prepend(to_close, client);
        }
    }

    if (!me->m_pending_streams)
    {
        for (GList* entry = me->m_connecting_clients; entry; entry = entry->next)
        {
            to_close = g_list_prepend(to_close, entry->data);
        }
        for (GList* entry = me->m_early_data_clients; entry; entry = entry->next)
        {
            RpConnectionPoolActiveClient* client = RP_CONNECTION_POOL_ACTIVE_CLIENT(entry->data);
            if (rp_connection_pool_active_client_num_active_streams(client) == 0)
            {
                to_close = g_list_prepend(to_close, client);
            }
        }
    }

    if (to_close)
    {
        to_close = g_list_reverse(to_close);
        for (GList* entry = to_close; entry; entry = entry->next)
        {
            NOISY_MSG_("closing idle client %p", entry->data);
            rp_connection_pool_active_client_close(RP_CONNECTION_POOL_ACTIVE_CLIENT(entry->data));
        }
    }
}

bool
rp_conn_pool_impl_base_is_idle_impl(RpConnPoolImplBase* self)
{
    LOGD("(%p)", self);
    RpConnPoolImplBasePrivate* me = PRIV(self);
    return !me->m_pending_streams &&
            !me->m_ready_clients &&
            !me->m_busy_clients &&
            !me->m_connecting_clients &&
            !me->m_early_data_clients;
}

static void
check_for_idle_and_notify(RpConnPoolImplBase* self)
{
    NOISY_MSG_("(%p)", self);
    if (rp_conn_pool_impl_base_is_idle_impl(self))
    {
        LOGD("invoking idle callbacks");
        RpConnPoolImplBasePrivate* me = PRIV(self);
        for (GSList* entry = me->m_idle_callbacks; entry; entry = entry->next)
        {
            NOISY_MSG_("invoking idle cb %p(%p)", entry->data, self);
            ((idle_cb)entry->data)(RP_CONNECTION_POOL_INSTANCE(self));
        }
        g_slist_free(g_steal_pointer(&me->m_idle_callbacks));
    }
}

void
rp_conn_pool_impl_base_check_for_idle_and_close_idle_conns_if_draining(RpConnPoolImplBase* self)
{
    NOISY_MSG_("(%p)", self);
    if (PRIV(self)->m_is_draining_for_deletion)
    {
        close_idle_connection_for_draining_pool(self);
    }

    check_for_idle_and_notify(self);
}

static void
purge_pending_streams(RpConnPoolImplBase* self, RpHostDescription* host_description, const char* failure_reason, RpPoolFailureReason_e reason)
{
    NOISY_MSG_("(%p, %p, %p(%s), %d)",
        self, host_description, failure_reason, failure_reason, reason);

    //TODO...state_.decrPendingStreams(pending_streams_.size());
    RpConnPoolImplBasePrivate* me = PRIV(self);
    me->m_pending_streams_to_purge = me->m_pending_streams;
    me->m_pending_streams = NULL;
    while (me->m_pending_streams_to_purge)
    {
        RpPendingStream* stream = me->m_pending_streams_to_purge->data;
        me->m_pending_streams_to_purge = g_list_remove(me->m_pending_streams_to_purge, stream);
        //TODO...
        rp_conn_pool_impl_base_on_pool_failure(self, host_description, failure_reason, reason, rp_pending_stream_context(stream));
    }
}

void
rp_conn_pool_impl_base_on_pending_stream_cancel(RpConnPoolImplBase* self,
                                                RpPendingStreamPtr stream,
                                                RpCancelPolicy_e policy)
{
    LOGD("(%p, %p, %d)", self, stream, policy);
    g_return_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self));
    RpConnPoolImplBasePrivate* me = PRIV(self);
    if (me->m_pending_streams_to_purge)
    {
        me->m_pending_streams_to_purge = g_list_remove(me->m_pending_streams_to_purge, stream);
    }
    else
    {
        //TODO...state_.decrPendingStreams(1);
        me->m_pending_streams = g_list_remove(me->m_pending_streams, stream);
    }
    if (policy == RpCancelPolicy_CloseExcess)
    {
        if (me->m_connecting_clients &&
            rp_conn_pool_impl_base_connecting_connection_is_excess(self, RP_CONNECTION_POOL_ACTIVE_CLIENT(me->m_connecting_clients->data)))
        {
            RpConnectionPoolActiveClient* client = RP_CONNECTION_POOL_ACTIVE_CLIENT(me->m_connecting_clients->data);
            rp_conn_pool_impl_base_transition_active_client_state(self, client, RpConnectionPoolActiveClientState_Draining);
            rp_connection_pool_active_client_close(client);
        }
        else if (me->m_early_data_clients)
        {
            for (GList* entry = me->m_early_data_clients; entry; entry = entry->next)
            {
                RpConnectionPoolActiveClient* client = RP_CONNECTION_POOL_ACTIVE_CLIENT(entry->data);
                if (rp_connection_pool_active_client_num_active_streams(client) == 0)
                {
                    if (rp_conn_pool_impl_base_connecting_connection_is_excess(self, client))
                    {
                        rp_conn_pool_impl_base_transition_active_client_state(self, client, RpConnectionPoolActiveClientState_Draining);
                        rp_connection_pool_active_client_close(client);
                    }
                    break;
                }
            }
        }
    }

    //TODO...host_->cluster().trafficStats()...
    rp_conn_pool_impl_base_check_for_idle_and_close_idle_conns_if_draining(self);
}

void
rp_conn_pool_impl_base_add_idle_callback_impl(RpConnPoolImplBase* self, idle_cb cb)
{
    LOGD("(%p, %p)", self, cb);
    g_return_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self));
    RpConnPoolImplBasePrivate* me = PRIV(self);
    me->m_idle_callbacks = g_slist_append(me->m_idle_callbacks, cb);
}

static void
drain_clients(RpConnPoolImplBase* self, GList** clients)
{
    NOISY_MSG_("(%p, %p)", self, clients);
    while (*clients)
    {
        RpConnectionPoolActiveClient* client = RP_CONNECTION_POOL_ACTIVE_CLIENT((*clients)->data);
        rp_conn_pool_impl_base_transition_active_client_state(self, client, RpConnectionPoolActiveClientState_Draining);
    }
}

void
rp_conn_pool_impl_base_drain_connections_impl(RpConnPoolImplBase* self, RpDrainBehavior_e drain_behavior)
{
    LOGD("(%p, %d)", self, drain_behavior);
    g_return_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self));
    RpConnPoolImplBasePrivate* me = PRIV(self);
    if (drain_behavior == RpDrainBehavior_DrainAndDelete)
    {
        me->m_is_draining_for_deletion = true;
        rp_conn_pool_impl_base_check_for_idle_and_close_idle_conns_if_draining(self);
        return;
    }

    close_idle_connection_for_draining_pool(self);

    if (!me->m_pending_streams)
    {
        NOISY_MSG_("draining early data clients");
        drain_clients(self, &me->m_early_data_clients);
    }

    drain_clients(self, &me->m_ready_clients);

    for (GList* entry = me->m_busy_clients; entry; entry = entry->next)
    {
        RpConnectionPoolActiveClient* busy_client = RP_CONNECTION_POOL_ACTIVE_CLIENT(entry->data);
        if (rp_connection_pool_active_client_state(busy_client) == RpConnectionPoolActiveClientState_Draining)
        {
            NOISY_MSG_("continuing");
            continue;
        }
        rp_conn_pool_impl_base_transition_active_client_state(self, busy_client, RpConnectionPoolActiveClientState_Draining);
    }
}

static void
destroy_connections(GList** list)
{
    NOISY_MSG_("(%p(%p))", list, *list);

    while (*list)
    {
        RpConnectionPoolActiveClient* active_client = (*list)->data;
        rp_connection_pool_active_client_close(active_client);
    }
}

void
rp_conn_pool_impl_base_destruct_all_connections(RpConnPoolImplBase* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self));
    RpConnPoolImplBasePrivate* me = PRIV(self);
    destroy_connections(&me->m_ready_clients);
    destroy_connections(&me->m_busy_clients);
    destroy_connections(&me->m_connecting_clients);
    destroy_connections(&me->m_early_data_clients);

    rp_dispatcher_clear_deferred_delete_list(me->m_dispatcher);
}

RpCancellable*
rp_conn_pool_impl_base_new_stream_impl(RpConnPoolImplBase* self, RpConnectionPoolAttachContextPtr context, bool can_send_early_data)
{
    LOGD("(%p, %p, %u)", self, context, can_send_early_data);
    g_return_val_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self), NULL);
    //TODO...assertCapacityCountsAreCorrect();

    RpConnPoolImplBasePrivate* me = PRIV(self);
NOISY_MSG_("ready clients %p", me->m_ready_clients);
    if (me->m_ready_clients)
    {
        RpConnectionPoolActiveClient* client = RP_CONNECTION_POOL_ACTIVE_CLIENT(me->m_ready_clients->data);
        LOGD("using existing connection");
        rp_conn_pool_impl_base_attach_stream_to_client(self, client, context);
        rp_conn_pool_impl_base_try_create_new_connections(self);
        return NULL;
    }

    if (can_send_early_data && me->m_early_data_clients)
    {
        RpConnectionPoolActiveClient* client = RP_CONNECTION_POOL_ACTIVE_CLIENT(me->m_early_data_clients->data);
        LOGD("using existing early data ready connection");
        rp_conn_pool_impl_base_attach_stream_to_client(self, client, context);
        rp_conn_pool_impl_base_try_create_new_connections(self);
        return NULL;
    }

    RpResourceLimit* pending_requests = get_pending_requests(me);
    if (!rp_resource_limit_can_create(pending_requests))
    {
        LOGD("max pending streams overflow");
        rp_conn_pool_impl_base_on_pool_failure(self, NULL, "", RpPoolFailureReason_Overflow, context);
        //TODO...host_->cluster().trafficStats()...
        return NULL;
    }

    RpCancellable* pending = rp_conn_pool_impl_base_new_pending_stream(self, context, can_send_early_data);
    LOGD("trying to create new connection");

    RpResponseDecoder* decoder = ((RpHttpAttachContextPtr)context)->m_decoder;
    RpNetworkConnection* connection = rp_upstream_to_downstream_connection(RP_UPSTREAM_TO_DOWNSTREAM(decoder));
    me->m_requested_server_name = rp_connection_info_provider_requested_server_name(
                                    rp_network_connection_connection_info_provider(connection));
    NOISY_MSG_("requested server name \"%s\"", me->m_requested_server_name);

#if 0
    guint32 old_capacity = me->m_connecting_stream_capacity;
#endif//0
    RpConnectionResult_e result = rp_conn_pool_impl_base_try_create_new_connections(self);
    if (result == RpConnectionResult_FailedToCreateConnection)
    {
        rp_cancellable_cancel(pending, RpCancelPolicy_CloseExcess);
        rp_conn_pool_impl_base_on_pool_failure(self, NULL, "", RpPoolFailureReason_LocalConnectionFailure, context);
        return NULL;
    }

    NOISY_MSG_("pending %p", pending);
    return pending;
}

bool
rp_conn_pool_impl_base_maybe_preconnect_impl(RpConnPoolImplBase* self, float global_preconnect_ratio)
{
    LOGD("(%p, %.1f)", self, global_preconnect_ratio);
    g_return_val_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self), false);
    return try_create_new_connection(self, global_preconnect_ratio) == RpConnectionResult_CreatedNewConnection;
}

bool
rp_conn_pool_impl_base_has_pending_streams(RpConnPoolImplBase* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self), false);
    return PRIV(self)->m_pending_streams != NULL;
}

bool
rp_conn_pool_impl_base_has_active_streams(RpConnPoolImplBase* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self), false);
    return PRIV(self)->m_num_active_streams > 0;
}

RpDispatcher*
rp_conn_pool_impl_base_dispatcher(RpConnPoolImplBase* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self), NULL);
    return PRIV(self)->m_dispatcher;
}

void
rp_conn_pool_impl_base_schedule_on_upstream_ready(RpConnPoolImplBase* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self));
    rp_schedulable_callback_schedule_callback_current_iteration(PRIV(self)->m_upstream_ready_cb);
}

void
rp_conn_pool_impl_base_on_upstream_ready_for_early_data(RpConnPoolImplBase* self, RpConnectionPoolActiveClient* client)
{
    LOGD("(%p, %p)", self, client);
    g_return_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self));
    g_return_if_fail(RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(client));
    RpConnPoolImplBasePrivate* me = PRIV(self);
    if (!me->m_pending_streams)
    {
        NOISY_MSG_("returning");
        return;
    }
    GList* it = g_list_last(me->m_pending_streams);
    while (rp_connection_pool_active_client_current_unused_capacity(client) > 0)
    {
        RpPendingStream* stream = it->data;
        bool stop_iteration = false;
        if (it != me->m_pending_streams)
        {
            it = it->prev;
        }
        else
        {
            stop_iteration = true;
        }

        if (rp_pending_stream_can_send_early_data_(stream))
        {
            NOISY_MSG_("creating stream for early data");
            attach_stream_to_client(self, client, rp_pending_stream_context(stream));
            //TODO...cluster_connectivity_state_.decrPendingStreams(1);
            me->m_pending_streams = g_list_remove(me->m_pending_streams, stream);
        }
        if (stop_iteration)
        {
            NOISY_MSG_("returning");
            return;
        }
    }
}

RpCancellable*
rp_conn_pool_impl_base_add_pending_stream(RpConnPoolImplBase* self, RpPendingStream* pending_stream)
{
    LOGD("(%p, %p)", self, pending_stream);
    g_return_val_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self), NULL);
    g_return_val_if_fail(RP_IS_PENDING_STREAM(pending_stream), NULL);
    RpConnPoolImplBasePrivate* me = PRIV(self);
    //TODO...state_.incrPendingStreams(1);
    me->m_pending_streams = g_list_append(me->m_pending_streams, pending_stream);
    return RP_CANCELLABLE(pending_stream);
}

void
rp_conn_pool_impl_base_on_connection_event(RpConnPoolImplBase* self, RpConnectionPoolActiveClient* client,
                                            const char* failure_reason, RpNetworkConnectionEvent_e event)
{
    LOGD("(%p, %p, %p(%s), %d)", self, client, failure_reason, failure_reason, event);
    g_return_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self));
    RpConnPoolImplBasePrivate* me = PRIV(self);
    switch (event)
    {
        case RpNetworkConnectionEvent_RemoteClose:
        case RpNetworkConnectionEvent_LocalClose:
        {
            NOISY_MSG_("closed");
            //TODO...if (client.connect_timer_)...
            rp_conn_pool_impl_base_decr_connecting_and_connected_stream_capacity(self,
                rp_connection_pool_active_client_current_unused_capacity(client), client);

            rp_connection_pool_active_client_set_remaining_streams(client, 0);

            //TODO...Envoy::Upstream::reportUpstreamCxDestroy(host_, event);
            bool incomplete_stream = rp_connection_pool_active_client_closing_with_incomplete_stream(client);
            if (incomplete_stream)
            {
                //TODO...Envoy::Upstream::reportUpstreamCxDestroyActiveRequest(host_, event);
            }

            if (!rp_connection_pool_active_client_has_handshake_completed(client))
            {
                rp_connection_pool_active_client_set_has_handshake_completed(client, true);
                //TODO...

                on_connect_failed(self, client);
                //TODO...
                RpPoolFailureReason_e reason = event == RpNetworkConnectionEvent_LocalClose ?
                    RpPoolFailureReason_LocalConnectionFailure : RpPoolFailureReason_RemoteConnectionFailure;
                purge_pending_streams(self, rp_connection_pool_active_client_get_real_host_description(client), ""/*TODO*/, reason);
            }

            rp_connection_pool_active_client_release_resources(client);
            //TODO...

            GList** list = owning_list(me, rp_connection_pool_active_client_state(client));
g_assert(list != NULL);
g_assert(*list != NULL);
            *list = g_list_remove(*list, client);
            rp_dispatcher_deferred_delete(me->m_dispatcher, G_OBJECT(client));

            check_for_idle_and_notify(self);

            rp_connection_pool_active_client_set_state(client, RpConnectionPoolActiveClientState_Closed);

            if (me->m_pending_streams)
            {
                NOISY_MSG_("trying to create new connections");
                rp_conn_pool_impl_base_try_create_new_connections(self);
            }
            break;
        }
        case RpNetworkConnectionEvent_Connected:
        {
            NOISY_MSG_("connected");
            //TODO...client.connect_timer_->disableTimer();
            gint64 current_unused_capacity = rp_connection_pool_active_client_current_unused_capacity(client);
            me->m_connecting_and_connected_stream_capacity -= current_unused_capacity;
            rp_connection_pool_active_client_set_has_handshake_completed(client, true);
            //TODO...client.conn_connect_ms_->complete();
            RpConnectionPoolActiveClientState_e state = rp_connection_pool_active_client_state(client);
            if (state == RpConnectionPoolActiveClientState_Connecting ||
                state == RpConnectionPoolActiveClientState_ReadyForEarlyData)
            {
                NOISY_MSG_("current unused capacity %ld", current_unused_capacity);
                rp_conn_pool_impl_base_transition_active_client_state(self,
                                                                        client,
                                                                        (current_unused_capacity > 0 ?
                                                                            RpConnectionPoolActiveClientState_Ready :
                                                                            RpConnectionPoolActiveClientState_Busy));
            }
            //TODO...set up a timer for max connection duration...
            rp_connection_pool_active_client_initialize_read_filters(client);
            RP_CONN_POOL_IMPL_BASE_GET_CLASS(self)->on_connected(self, client);
            if (rp_connection_pool_active_client_ready_for_stream(client))
            {
                rp_conn_pool_impl_base_on_upstream_ready(self);
            }
            rp_conn_pool_impl_base_check_for_idle_and_close_idle_conns_if_draining(self);
            break;
        }
        case RpNetworkConnectionEvent_ConnectedZeroRtt:
        {
            NOISY_MSG_("connected zero rtt");
            g_assert(rp_connection_pool_active_client_state(client) == RpConnectionPoolActiveClientState_Connecting);
            //TODO...host()->cluster().trafficStats()...
            rp_conn_pool_impl_base_transition_active_client_state(self, client,
                (rp_connection_pool_active_client_current_unused_capacity(client) > 0 ?
                    RpConnectionPoolActiveClientState_ReadyForEarlyData : RpConnectionPoolActiveClientState_Busy));
            break;
        }
        case RpNetworkConnectionEvent_None:
            NOISY_MSG_("none");
            break;
    }
}

void
rp_conn_pool_impl_base_on_stream_closed(RpConnPoolImplBase* self, RpConnectionPoolActiveClient* client, bool delay_attaching_stream)
{
    LOGD("(%p, %p, %u)", self, client, delay_attaching_stream);
    g_return_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self));
    g_return_if_fail(RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(client));
    RpConnPoolImplBasePrivate* me = PRIV(self);
    --me->m_num_active_streams;
    //TODO...
    rp_resource_limit_dec(get_requests(me));
    //TODO...

    if (rp_connection_pool_active_client_state(client) == RpConnectionPoolActiveClientState_Draining &&
        rp_connection_pool_active_client_num_active_streams(client) == 0)
    {
        NOISY_MSG_("closing client");
        rp_connection_pool_active_client_close(client);
    }
    else if (rp_connection_pool_active_client_state(client) == RpConnectionPoolActiveClientState_Busy &&
                rp_connection_pool_active_client_current_unused_capacity(client) > 0)
    {
        if (!rp_connection_pool_active_client_has_handshake_completed(client))
        {
            NOISY_MSG_("transitioning");
            rp_conn_pool_impl_base_transition_active_client_state(self, client, RpConnectionPoolActiveClientState_ReadyForEarlyData);
            if (!delay_attaching_stream)
            {
                rp_conn_pool_impl_base_on_upstream_ready_for_early_data(self, client);
            }
        }
        else
        {
            rp_conn_pool_impl_base_transition_active_client_state(self, client, RpConnectionPoolActiveClientState_Ready);
            if (!delay_attaching_stream)
            {
                rp_conn_pool_impl_base_on_upstream_ready(self);
            }
        }
    }
}

GList**
rp_conn_pool_impl_base_ready_clients_(RpConnPoolImplBase* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self), NULL);
    return &PRIV(self)->m_ready_clients;
}

GList**
rp_conn_pool_impl_base_busy_clients_(RpConnPoolImplBase* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self), NULL);
    return &PRIV(self)->m_busy_clients;
}

GList**
rp_conn_pool_impl_base_connecting_clients_(RpConnPoolImplBase* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CONN_POOL_IMPL_BASE(self), NULL);
    return &PRIV(self)->m_connecting_clients;
}
