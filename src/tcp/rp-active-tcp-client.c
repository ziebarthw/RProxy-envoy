/*
 * rp-active-tcp-client.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_active_tcp_client_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_active_tcp_client_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "tcp/rp-conn-read-filter.h"
#include "tcp/rp-tcp-connection-data.h"
#include "tcp/rp-active-tcp-client.h"

#define CONNECTION(s) RP_ACTIVE_TCP_CLIENT(s)->m_connection
#define CALLBACKS(s) RP_ACTIVE_TCP_CLIENT(s)->m_callbacks
#define NETWORK_CONNECTION(s) RP_NETWORK_CONNECTION(CONNECTION(s))
#define NETWORK_FILTER_MANAGER(s) RP_NETWORK_FILTER_MANAGER(CONNECTION(s))
#define NETWORK_CONNECTION_CALLBACKS(s) RP_NETWORK_CONNECTION_CALLBACKS(CALLBACKS(s))
#define SOCKFD(s) rp_network_connection_sockfd(NETWORK_CONNECTION(s))

struct _RpActiveTcpClient {
    RpConnectionPoolActiveClient parent_instance;

    RpConnPoolImplBase* m_parent;
    UNIQUE_PTR(RpNetworkClientConnection) m_connection;
    RpHost* m_host;
    UNIQUE_PTR(RpTcpConnectionData) m_tcp_connection_data;
    RpTcpConnPoolUpstreamCallbacks* m_callbacks;
    RpConnReadFilter* m_read_filter_handle;

    bool m_associated_before;
};

static void network_connection_callbacks_iface_init(RpNetworkConnectionCallbacksInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpActiveTcpClient, rp_active_tcp_client, RP_TYPE_CONNECTION_POOL_ACTIVE_CLIENT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_CONNECTION_CALLBACKS, network_connection_callbacks_iface_init)
)

static void
on_event_i(RpNetworkConnectionCallbacks* self, RpNetworkConnectionEvent_e event)
{
    NOISY_MSG_("(%p(fd %d), %d)", self, SOCKFD(self), event);

    RpActiveTcpClient* me = RP_ACTIVE_TCP_CLIENT(self);
    RpNetworkConnection* connection_ = NETWORK_CONNECTION(self);
    if (event == RpNetworkConnectionEvent_Connected)
    {
        NOISY_MSG_("disabling read on fd %d", SOCKFD(self));
        rp_network_connection_read_disable(connection_, true);
    }
NOISY_MSG_("calling rp_conn_pool_impl_base_on_connection_event(%p, %p, ..., %d)", me->m_parent, self, event);
    rp_conn_pool_impl_base_on_connection_event(me->m_parent,
                                                RP_CONNECTION_POOL_ACTIVE_CLIENT(self),
                                                rp_network_connection_transport_failure_reason(connection_),
                                                event);
NOISY_MSG_("back from calling rp_conn_pool_impl_base_on_connection_event(%p, %p, ..., %d)", me->m_parent, self, event);

    if (event == RpNetworkConnectionEvent_LocalClose ||
        event == RpNetworkConnectionEvent_RemoteClose)
    {
        //TODO...disableIdleTimer();
        if (me->m_callbacks)
        {
            if (me->m_tcp_connection_data)
            {
                //TODO...reportUpstreamCxDestroyActiveRequest(...)
            }
            rp_network_connection_callbacks_on_event(
                RP_NETWORK_CONNECTION_CALLBACKS(g_steal_pointer(&me->m_callbacks)), event);
        }
    }
}

static void
on_above_write_buffer_high_water_mark_i(RpNetworkConnectionCallbacks* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
    rp_network_connection_callbacks_on_above_write_buffer_high_water_mark(
        NETWORK_CONNECTION_CALLBACKS(self));
}

static void
on_below_write_buffer_low_watermark_i(RpNetworkConnectionCallbacks* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
    rp_network_connection_callbacks_on_below_write_buffer_low_watermark(
        NETWORK_CONNECTION_CALLBACKS(self));
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
dispose(GObject* obj)
{
    LOGD("(%p)", obj);

    RpActiveTcpClient* self = RP_ACTIVE_TCP_CLIENT(obj);
    if (self->m_tcp_connection_data)
    {
        rp_tcp_connection_data_release(self->m_tcp_connection_data);
        rp_conn_pool_impl_base_on_stream_closed(self->m_parent, RP_CONNECTION_POOL_ACTIVE_CLIENT(self), true);
        rp_conn_pool_impl_base_check_for_idle_and_close_idle_conns_if_draining(self->m_parent);
    }
NOISY_MSG_("clearing connection %p", self->m_connection);
    g_clear_object(&self->m_connection);

    G_OBJECT_CLASS(rp_active_tcp_client_parent_class)->dispose(obj);
}

OVERRIDE void
initialize_read_filters(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
    rp_network_filter_manager_initialize_read_filters(NETWORK_FILTER_MANAGER(self));
}

OVERRIDE evhtp_proto
protocol(RpConnectionPoolActiveClient* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return EVHTP_PROTO_INVALID;
}

OVERRIDE void
close_(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
    rp_network_connection_close(NETWORK_CONNECTION(self), RpNetworkConnectionCloseType_NoFlush);
}

OVERRIDE guint32
num_active_streams(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
    return CALLBACKS(self) ? 1 : 0;
}

OVERRIDE bool
closing_with_incomplete_stream(RpConnectionPoolActiveClient* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return false;
}

OVERRIDE guint64
id(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_network_connection_id(NETWORK_CONNECTION(self));
}

OVERRIDE void
connect_(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
    rp_network_client_connection_connect(CONNECTION(self));
}

OVERRIDE void
set_requested_server_name(RpConnectionPoolActiveClient* self, const char* requested_server_name)
{
    NOISY_MSG_("(%p, %p(%s))", self, requested_server_name, requested_server_name);
    rp_connection_info_setter_set_requested_server_name(
        rp_network_connection_connection_info_setter(NETWORK_CONNECTION(self)), requested_server_name);
}

static void
connection_pool_active_client_class_init(RpConnectionPoolActiveClientClass* klass)
{
    LOGD("(%p)", klass);
    klass->initialize_read_filters = initialize_read_filters;
    klass->protocol = protocol;
    klass->close = close_;
    klass->num_active_streams = num_active_streams;
    klass->closing_with_incomplete_stream = closing_with_incomplete_stream;
    klass->id = id;
    klass->connect = connect_;
    klass->set_requested_server_name = set_requested_server_name;
}

static void
rp_active_tcp_client_class_init(RpActiveTcpClientClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    connection_pool_active_client_class_init(RP_CONNECTION_POOL_ACTIVE_CLIENT_CLASS(klass));
}

static void
rp_active_tcp_client_init(RpActiveTcpClient* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpActiveTcpClient*
constructed(RpActiveTcpClient* self)
{
    NOISY_MSG_("(%p)", self);

    RpHost* host = self->m_host;
    RpDispatcher* dispatcher = rp_conn_pool_impl_base_dispatcher(self->m_parent);
    RpCreateConnectionData data = rp_host_create_connection(host, dispatcher);

    rp_connection_pool_active_client_set_real_host_description(RP_CONNECTION_POOL_ACTIVE_CLIENT(self),
                                                                data.m_host_description);
    self->m_connection = g_steal_pointer(&data.m_connection);

    RpNetworkConnection* connection_ = NETWORK_CONNECTION(self);
    rp_network_connection_add_connection_callbacks(connection_, RP_NETWORK_CONNECTION_CALLBACKS(self));

    self->m_read_filter_handle = rp_conn_read_filter_new(self);
    rp_network_filter_manager_add_read_filter(RP_NETWORK_FILTER_MANAGER(connection_), RP_NETWORK_READ_FILTER(self->m_read_filter_handle));

    rp_network_connection_no_delay(connection_, true);
//    rp_network_client_connection_connect(self->m_connection);//TODO...make conditional?
    return self;
}

RpActiveTcpClient*
rp_active_tcp_client_new(RpConnPoolImplBase* parent, RpHost* host, guint64 concurrent_stream_limit/*TODO...idle_timeout*/)
{
    LOGD("(%p, %p, %zu)", parent, host, concurrent_stream_limit);
    RpActiveTcpClient* self =  g_object_new(RP_TYPE_ACTIVE_TCP_CLIENT,
                                            "lifetime-stream-limit", rp_cluster_info_max_requests_per_connection(
                                                                        rp_host_description_cluster(RP_HOST_DESCRIPTION(host))),
                                            "concurrent-stream-limit", concurrent_stream_limit,
                                            NULL);
    self->m_parent = parent;
    self->m_host = host;
    return constructed(self);
}

void
rp_active_tcp_client_set_tcp_connection_data_(RpActiveTcpClient* self, RpTcpConnectionData* data)
{
    LOGD("(%p, %p)", self, data);
    g_return_if_fail(RP_IS_ACTIVE_TCP_CLIENT(self));
    g_return_if_fail(RP_IS_TCP_CONNECTION_DATA(data));
    self->m_tcp_connection_data = g_steal_pointer(&data);
}

RpNetworkClientConnection*
rp_active_tcp_client_connection_(RpActiveTcpClient* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_ACTIVE_TCP_CLIENT(self), NULL);
    return self->m_connection;
}

void
rp_active_tcp_client_set_callbacks_(RpActiveTcpClient* self,
                                            RpTcpConnPoolUpstreamCallbacks* callbacks)
{
    LOGD("(%p(fd %d), %p)", self, SOCKFD(self), callbacks);
    g_return_if_fail(RP_IS_ACTIVE_TCP_CLIENT(self));
    g_return_if_fail(RP_IS_TCP_CONN_POOL_UPSTREAM_CALLBACKS(callbacks));
    self->m_callbacks = callbacks;
}

void
rp_active_tcp_client_read_enable_if_new(RpActiveTcpClient* self)
{
    LOGD("(%p(fd %d))", self, SOCKFD(self));
    g_return_if_fail(RP_IS_ACTIVE_TCP_CLIENT(self));
    if (!self->m_associated_before)
    {
        self->m_associated_before = true;
        RpNetworkConnection* connection_ = NETWORK_CONNECTION(self);
        rp_network_connection_read_disable(connection_, false);
        rp_network_connection_detect_early_close_when_read_disabled(connection_, false);
    }
}

void
rp_active_tcp_client_on_upstream_data(RpActiveTcpClient* self, evbuf_t* data, bool end_stream)
{
    LOGD("(%p(fd %d), %p(%zu), %u)", self, SOCKFD(self), data, data ? evbuffer_get_length(data) : 0, end_stream);
    g_return_if_fail(RP_IS_ACTIVE_TCP_CLIENT(self));
    if (self->m_callbacks)
    {
        rp_tcp_conn_pool_upstream_callbacks_on_upstream_data(self->m_callbacks, data, end_stream);
    }
    else
    {
        close_(RP_CONNECTION_POOL_ACTIVE_CLIENT(self));
    }
}

void
rp_active_tcp_client_clear_callbacks(RpActiveTcpClient* self)
{
    LOGD("(%p(fd %d))", self, SOCKFD(self));
    g_return_if_fail(RP_IS_ACTIVE_TCP_CLIENT(self));
    if (rp_connection_pool_active_client_state(RP_CONNECTION_POOL_ACTIVE_CLIENT(self)) == RpConnectionPoolActiveClientState_Busy &&
        rp_conn_pool_impl_base_has_pending_streams(self->m_parent))
    {
        NOISY_MSG_("schedule on upstream ready");
        rp_conn_pool_impl_base_schedule_on_upstream_ready(self->m_parent);
    }
    self->m_callbacks = NULL;
    g_clear_object(&self->m_tcp_connection_data);
    rp_conn_pool_impl_base_on_stream_closed(self->m_parent, RP_CONNECTION_POOL_ACTIVE_CLIENT(self), true);
    //TODO...setIdleTimer();
    rp_conn_pool_impl_base_check_for_idle_and_close_idle_conns_if_draining(self->m_parent);
}
