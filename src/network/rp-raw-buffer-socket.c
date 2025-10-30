/*
 * rp-raw-buffer-socket.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_raw_buffer_socket_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_raw_buffer_socket_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include "rp-headers.h"
#include "rp-stream-info.h"
#include "network/rp-io-bev-socket-handle-impl.h"
#include "network/rp-raw-buffer-socket.h"

struct _RpRawBufferSocket {
    GObject parent_instance;

    UNIQUE_PTR(evbev_t) m_bev; // Temporary ownership until create_io_handle() is called.
    evhtp_ssl_t* m_ssl;
    struct evdns_base* m_dns_base;

    RpHandleType_e m_type;

    RpNetworkTransportSocketCallbacks* m_callbacks;
    bool m_shutdown;
};

static void transport_socket_iface_init(RpNetworkTransportSocketInterface* iface);
static void ssl_connection_info_iface_init(RpSslConnectionInfoInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpRawBufferSocket, rp_raw_buffer_socket, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_TRANSPORT_SOCKET, transport_socket_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_SSL_CONNECTION_INFO, ssl_connection_info_iface_init)
)

static RpSslConnectionInfo*
ssl_i(RpNetworkTransportSocket* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_RAW_BUFFER_SOCKET(self)->m_ssl ? RP_SSL_CONNECTION_INFO(self) : NULL;
}

static void
set_transport_socket_callbacks_i(RpNetworkTransportSocket* self, RpNetworkTransportSocketCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    RP_RAW_BUFFER_SOCKET(self)->m_callbacks = callbacks;
}

static void
close_socket_i(RpNetworkTransportSocket* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static void
on_connected_i(RpNetworkTransportSocket* self)
{
    NOISY_MSG_("(%p)", self);
    rp_network_transport_socket_callbacks_raise_event(
        RP_RAW_BUFFER_SOCKET(self)->m_callbacks, RpNetworkConnectionEvent_Connected);
}

static RpIoHandle*
create_io_handle_i(RpNetworkTransportSocket* self)
{
    NOISY_MSG_("(%p)", self);
    RpRawBufferSocket* me = RP_RAW_BUFFER_SOCKET(self);
    RpIoBevSocketHandleImpl* io_handle = rp_io_bev_socket_handle_impl_new(me->m_type, g_steal_pointer(&me->m_bev), NULL);
    return RP_IO_HANDLE(io_handle);
}

static RpIoResult
do_read_i(RpNetworkTransportSocket* self, evbuf_t* buffer)
{
    NOISY_MSG_("(%p, %p)", self, buffer);

    RpRawBufferSocket* me = RP_RAW_BUFFER_SOCKET(self);
    RpNetworkTransportSocketCallbacks* callbacks_ = me->m_callbacks;
    RpIoHandle* io_handle = rp_network_transport_socket_callbacks_io_handle(callbacks_);
    RpPostIoAction_e action = RpPostIoAction_KeepOpen;
    guint64 bytes_read = 0;
    bool end_stream = false;
    int err = 0;
    int result = rp_io_handle_read(io_handle, buffer);

    if (result == 0)
    {
        end_stream = true;
    }
    else if (result < 0)
    {
        err = ECONNRESET;
        action = RpPostIoAction_Close;
    }
    else
    {
        bytes_read = result;
        if (rp_network_transport_socket_callbacks_should_drain_read_buffer(callbacks_))
        {
            NOISY_MSG_("setting readable");
            rp_network_transport_socket_callbacks_set_transport_socket_is_readable(callbacks_);
        }
    }

    return rp_io_result_ctor(action, bytes_read, end_stream, err);
}

static RpIoResult
do_write_i(RpNetworkTransportSocket* self, evbuf_t* buffer, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, buffer, evbuffer_get_length(buffer), end_stream);

    RpRawBufferSocket* me = RP_RAW_BUFFER_SOCKET(self);
    RpNetworkTransportSocketCallbacks* callbacks_ = me->m_callbacks;
    RpIoHandle* io_handle = rp_network_transport_socket_callbacks_io_handle(callbacks_);
    guint64 bytes_written = 0;
    size_t bytes_to_write = evbuffer_get_length(buffer);
    RpPostIoAction_e action = RpPostIoAction_KeepOpen;
    int err = 0;

    if (bytes_to_write == 0)
    {
        if (end_stream && !me->m_shutdown)
        {
            rp_io_handle_shutdown(io_handle, SHUT_RDWR);
            me->m_shutdown = true;
        }
    }
    else if (rp_io_handle_write(io_handle, buffer) == 0)
    {
        bytes_to_write -= evbuffer_get_length(buffer);
        bytes_written += bytes_to_write;
    }
    else
    {
        action = RpPostIoAction_Close;
        err = ECONNRESET;
    }

    return rp_io_result_ctor(action, bytes_written, false, err);
}

static int
connect_i(RpNetworkTransportSocket* self, RpConnectionSocket* socket)
{
    NOISY_MSG_("(%p, %p)", self, socket);
    struct sockaddr* addr = rp_connection_info_provider_remote_address(
                                RP_CONNECTION_INFO_PROVIDER(rp_socket_connection_info_provider(RP_SOCKET(socket))));
RpNetworkConnection* connection = rp_network_transport_socket_callbacks_connection(RP_RAW_BUFFER_SOCKET(self)->m_callbacks);
NOISY_MSG_("connection %p", connection);
RpStreamInfo* stream_info = rp_network_connection_stream_info(connection);
NOISY_MSG_("stream info %p", stream_info);
RpConnectionInfoProvider* info_provider = rp_stream_info_downstream_address_provider(stream_info);
NOISY_MSG_("info provider %p", info_provider);
const char* requested_server_name = rp_connection_info_provider_requested_server_name(info_provider);
NOISY_MSG_("requested server name %p(%s)", requested_server_name, requested_server_name);

    return rp_socket_connect(RP_SOCKET(socket), addr, requested_server_name);
}

static evdns_base_t*
dns_base_i(RpNetworkTransportSocket* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_RAW_BUFFER_SOCKET(self)->m_dns_base;
}

static void
transport_socket_iface_init(RpNetworkTransportSocketInterface* iface)
{
    LOGD("(%p)", iface);
    iface->ssl = ssl_i;
    iface->set_transport_socket_callbacks = set_transport_socket_callbacks_i;
    iface->close_socket = close_socket_i;
    iface->on_connected = on_connected_i;
    iface->create_io_handle = create_io_handle_i;
    iface->do_read = do_read_i;
    iface->do_write = do_write_i;
    iface->connect = connect_i;
    iface->dns_base = dns_base_i;
}

static void
ssl_connection_info_iface_init(RpSslConnectionInfoInterface* iface)
{
    LOGD("(%p)", iface);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpRawBufferSocket* self = RP_RAW_BUFFER_SOCKET(obj);
    g_clear_pointer(&self->m_bev, bufferevent_free);

    G_OBJECT_CLASS(rp_raw_buffer_socket_parent_class)->dispose(obj);
}

static void
rp_raw_buffer_socket_class_init(RpRawBufferSocketClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_raw_buffer_socket_init(RpRawBufferSocket* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpRawBufferSocket*
rp_raw_buffer_socket_new(RpHandleType_e type, evbev_t* bev, evhtp_ssl_t* ssl, struct evdns_base* dns_base)
{
    LOGD("(%d, %p, %p, %p)", type, bev, ssl, dns_base);
    RpRawBufferSocket* self = g_object_new(RP_TYPE_RAW_BUFFER_SOCKET, NULL);
    self->m_type = type;
    self->m_bev = bev;
    self->m_ssl = ssl;
    self->m_dns_base = dns_base;
    return self;
}
