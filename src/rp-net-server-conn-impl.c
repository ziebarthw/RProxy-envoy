/*
 * rp-net-server-conn-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_net_server_conn_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_net_server_conn_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include "rp-net-server-conn-impl.h"

struct _RpNetworkServerConnectionImpl {
    RpNetworkConnectionImpl parent_instance;

    bool m_transport_connect_pending;
    //TODO...Event::TimePtr transport_socket_connect_timer_;
    //TODO..Stats::Counter8 transport_socket_timeout_stat_;
};

static void network_filter_manager_iface_init(RpNetworkFilterManagerInterface* iface);
static void network_server_connection_iface_init(RpNetworkServerConnectionInterface* iface);
static void network_transport_socket_callbacks_iface_init(RpNetworkTransportSocketCallbacksInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpNetworkServerConnectionImpl, rp_network_server_connection_impl, RP_TYPE_NETWORK_CONNECTION_IMPL,
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_SERVER_CONNECTION, network_server_connection_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_FILTER_MANAGER, network_filter_manager_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_TRANSPORT_SOCKET_CALLBACKS, network_transport_socket_callbacks_iface_init)
)

static inline bool
parent_initialize_read_filters(RpNetworkServerConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    RpNetworkFilterManagerInterface* parent_iface = g_type_interface_peek_parent(RP_NETWORK_FILTER_MANAGER_GET_IFACE(self));
    return parent_iface->initialize_read_filters(RP_NETWORK_FILTER_MANAGER(self));
}

static bool
initialize_read_filters_i(RpNetworkFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    RpNetworkConnectionImpl* connection = RP_NETWORK_CONNECTION_IMPL(self);
    bool initialized = parent_initialize_read_filters(RP_NETWORK_SERVER_CONNECTION_IMPL(self));
    if (initialized)
    {
        rp_network_connection_impl_on_connected_(connection);
    }
    return initialized;
}

static void
network_filter_manager_iface_init(RpNetworkFilterManagerInterface* iface)
{
    NOISY_MSG_("(%p)", iface);
    iface->initialize_read_filters = initialize_read_filters_i;
}

#define SOCKFD(s) rp_network_connection_sockfd(RP_NETWORK_CONNECTION(s))

static void
raise_event_i(RpNetworkTransportSocketCallbacks* self, RpNetworkConnectionEvent_e event)
{
    NOISY_MSG_("(%p(fd %d), %d)", self, SOCKFD(self), event);
    RpNetworkServerConnectionImpl* me = RP_NETWORK_SERVER_CONNECTION_IMPL(self);
    switch (event)
    {
        case RpNetworkConnectionEvent_ConnectedZeroRtt:
            break;
        case RpNetworkConnectionEvent_Connected:
        case RpNetworkConnectionEvent_RemoteClose:
        case RpNetworkConnectionEvent_LocalClose:
            me->m_transport_connect_pending = false;
            //TODO...transport_socket_connect_timer_.reset();
            break;
        default:
            break;

    }
NOISY_MSG_("calling rp_network_connection_impl_raise_event(%p, %d) on fd %d", self, event, SOCKFD(self));
    rp_network_connection_impl_raise_event(RP_NETWORK_CONNECTION_IMPL(self), event);
}

#define PARENT_NETWORK_TRANSPORT_SOCKET_CALLBACKS_IFACE(s) \
    ((RpNetworkTransportSocketCallbacksInterface*)g_type_interface_peek_parent(RP_NETWORK_TRANSPORT_SOCKET_CALLBACKS_GET_IFACE(s)))

static RpNetworkConnection*
connection_i(RpNetworkTransportSocketCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_NETWORK_TRANSPORT_SOCKET_CALLBACKS_IFACE(self)->connection(self);
}

static void
flush_write_buffer_i(RpNetworkTransportSocketCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    PARENT_NETWORK_TRANSPORT_SOCKET_CALLBACKS_IFACE(self)->flush_write_buffer(self);
}

static RpIoHandle*
io_handle_i(RpNetworkTransportSocketCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_NETWORK_TRANSPORT_SOCKET_CALLBACKS_IFACE(self)->io_handle(self);
}

static void
set_transport_socket_is_readable_i(RpNetworkTransportSocketCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    PARENT_NETWORK_TRANSPORT_SOCKET_CALLBACKS_IFACE(self)->set_transport_socket_is_readable(self);
}

static bool
should_drain_read_buffer_i(RpNetworkTransportSocketCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_NETWORK_TRANSPORT_SOCKET_CALLBACKS_IFACE(self)->should_drain_read_buffer(self);
}

static void
network_transport_socket_callbacks_iface_init(RpNetworkTransportSocketCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->raise_event = raise_event_i;
    iface->connection = connection_i;
    iface->flush_write_buffer = flush_write_buffer_i;
    iface->io_handle = io_handle_i;
    iface->set_transport_socket_is_readable = set_transport_socket_is_readable_i;
    iface->should_drain_read_buffer = should_drain_read_buffer_i;
}

static void
set_transport_socket_connect_timeout_i(RpNetworkServerConnection* self, guint64 timeout_ms)
{
    LOGD("(%p, %zu)", self, timeout_ms);
    RpNetworkServerConnectionImpl* me = RP_NETWORK_SERVER_CONNECTION_IMPL(self);
    if (!me->m_transport_connect_pending)
    {
        NOISY_MSG_("returning");
        return;
    }

    //TODO...transport_socket_timeout_....
}

static void
network_server_connection_iface_init(RpNetworkServerConnectionInterface* iface)
{
    NOISY_MSG_("(%p)", iface);
    iface->set_transport_socket_connect_timeout = set_transport_socket_connect_timeout_i;
}

OVERRIDE void
dispose(GObject* object)
{
    LOGD("(%p)", object);
    G_OBJECT_CLASS(rp_network_server_connection_impl_parent_class)->dispose(object);
}

static void
network_connection_impl_class_init(RpNetworkConnectionImplClass* klass)
{
    LOGD("(%p)", klass);
    network_transport_socket_callbacks_iface_init(g_type_interface_peek(klass, RP_TYPE_NETWORK_TRANSPORT_SOCKET_CALLBACKS));
}

static void
rp_network_server_connection_impl_class_init(RpNetworkServerConnectionImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    network_connection_impl_class_init(RP_NETWORK_CONNECTION_IMPL_CLASS(klass));
}

static void
rp_network_server_connection_impl_init(RpNetworkServerConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_transport_connect_pending = true;
}

RpNetworkServerConnectionImpl*
rp_network_server_connection_impl_new(RpDispatcher* dispatcher, RpConnectionSocket* socket,
                                        RpNetworkTransportSocket* transport_socket, RpStreamInfo* stream_info)
{
    LOGD("(%p, %p, %p, %p)", dispatcher, socket, transport_socket, stream_info);
    g_return_val_if_fail(RP_IS_DISPATCHER(dispatcher), NULL);
    g_return_val_if_fail(RP_IS_CONNECTION_SOCKET(socket), NULL);
    g_return_val_if_fail(RP_IS_NETWORK_TRANSPORT_SOCKET(transport_socket), NULL);
    g_return_val_if_fail(RP_IS_STREAM_INFO(stream_info), NULL);
    return g_object_new(RP_TYPE_NETWORK_SERVER_CONNECTION_IMPL,
                        "dispatcher", dispatcher,
                        "socket", socket,
                        "transport-socket", transport_socket,
                        "stream-info", stream_info,
                        "connected", true,
                        NULL);
}
