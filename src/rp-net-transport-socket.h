/*
 * rp-net-transport-socket.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"
#include "rp-io-handle.h"
#include "rp-net-connection.h"
#include "rp-listen-socket.h"
#include "rp-post-io-action.h"

G_BEGIN_DECLS

typedef struct _RpHostDescription RpHostDescription;


/**
 * Result of each I/O event.
 */
typedef struct _RpIoResult RpIoResult;
struct _RpIoResult {
    RpPostIoAction_e m_action;
    guint64 m_bytes_processed;
    bool m_end_stream_read;
    int m_err_code;
};

static inline RpIoResult
rp_io_result_ctor(RpPostIoAction_e action, guint64 bytes_processed, bool end_stream_read, int err_code)
{
    RpIoResult self = {
        .m_action = action,
        .m_bytes_processed = bytes_processed,
        .m_end_stream_read = end_stream_read,
        .m_err_code = err_code
    };
    return self;
}


/**
 * Callbacks used by transport socket instances to communicate with connection.
 */
#define RP_TYPE_NETWORK_TRANSPORT_SOCKET_CALLBACKS rp_network_transport_socket_callbacks_get_type()
G_DECLARE_INTERFACE(RpNetworkTransportSocketCallbacks, rp_network_transport_socket_callbacks, RP, NETWORK_TRANSPORT_SOCKET_CALLBACKS, GObject)

struct _RpNetworkTransportSocketCallbacksInterface {
    GTypeInterface parent_iface;

    RpIoHandle* (*io_handle)(RpNetworkTransportSocketCallbacks*);
    RpNetworkConnection* (*connection)(RpNetworkTransportSocketCallbacks*);
    bool (*should_drain_read_buffer)(RpNetworkTransportSocketCallbacks*);
    void (*set_transport_socket_is_readable)(RpNetworkTransportSocketCallbacks*);
    void (*raise_event)(RpNetworkTransportSocketCallbacks*,
                        RpNetworkConnectionEvent_e);
    void (*flush_write_buffer)(RpNetworkTransportSocketCallbacks*);
};

static inline RpIoHandle*
rp_network_transport_socket_callbacks_io_handle(RpNetworkTransportSocketCallbacks* self)
{
    return RP_IS_NETWORK_TRANSPORT_SOCKET_CALLBACKS(self) ?
        RP_NETWORK_TRANSPORT_SOCKET_CALLBACKS_GET_IFACE(self)->io_handle(self) :
        NULL;
}
static inline RpNetworkConnection*
rp_network_transport_socket_callbacks_connection(RpNetworkTransportSocketCallbacks* self)
{
    return RP_IS_NETWORK_TRANSPORT_SOCKET_CALLBACKS(self) ?
        RP_NETWORK_TRANSPORT_SOCKET_CALLBACKS_GET_IFACE(self)->connection(self) :
        NULL;
}
static inline bool
rp_network_transport_socket_callbacks_should_drain_read_buffer(RpNetworkTransportSocketCallbacks* self)
{
    return RP_IS_NETWORK_TRANSPORT_SOCKET_CALLBACKS(self) ?
        RP_NETWORK_TRANSPORT_SOCKET_CALLBACKS_GET_IFACE(self)->should_drain_read_buffer(self) :
        false;
}
static inline void
rp_network_transport_socket_callbacks_set_transport_socket_is_readable(RpNetworkTransportSocketCallbacks* self)
{
    if (RP_IS_NETWORK_TRANSPORT_SOCKET_CALLBACKS(self)) \
        RP_NETWORK_TRANSPORT_SOCKET_CALLBACKS_GET_IFACE(self)->set_transport_socket_is_readable(self);
}
static inline void
rp_network_transport_socket_callbacks_raise_event(RpNetworkTransportSocketCallbacks* self, RpNetworkConnectionEvent_e event)
{
    if (RP_IS_NETWORK_TRANSPORT_SOCKET_CALLBACKS(self)) \
        RP_NETWORK_TRANSPORT_SOCKET_CALLBACKS_GET_IFACE(self)->raise_event(self, event);
}
static inline void
rp_network_transport_socket_callbacks_flush_write_buffer(RpNetworkTransportSocketCallbacks* self)
{
    if (RP_IS_NETWORK_TRANSPORT_SOCKET_CALLBACKS(self)) \
        RP_NETWORK_TRANSPORT_SOCKET_CALLBACKS_GET_IFACE(self)->flush_write_buffer(self);
}

/**
 * A transport socket that does actual read / write. It can also do some transformations on
 * the data (e.g. TLS).
 */
#define RP_TYPE_NETWORK_TRANSPORT_SOCKET rp_network_transport_socket_get_type()
G_DECLARE_INTERFACE(RpNetworkTransportSocket, rp_network_transport_socket, RP, NETWORK_TRANSPORT_SOCKET, GObject)

struct _RpNetworkTransportSocketInterface {
    GTypeInterface parent_iface;

    void (*set_transport_socket_callbacks)(RpNetworkTransportSocket*, RpNetworkTransportSocketCallbacks*);
    const char* (*protocol)(RpNetworkTransportSocket*);
    const char* (*failure_reason)(RpNetworkTransportSocket*);
    bool (*can_flush_close)(RpNetworkTransportSocket*);
    int (*connect)(RpNetworkTransportSocket*, RpConnectionSocket*);
    void (*close_socket)(RpNetworkTransportSocket*);
    RpIoResult (*do_read)(RpNetworkTransportSocket*, evbuf_t*);
    RpIoResult (*do_write)(RpNetworkTransportSocket*, evbuf_t*, bool);
    void (*on_connected)(RpNetworkTransportSocket*);
    RpSslConnectionInfo* (*ssl)(RpNetworkTransportSocket*);
    bool (*start_secure_transport)(RpNetworkTransportSocket*);

    RpIoHandle* (*create_io_handle)(RpNetworkTransportSocket*);
    evdns_base_t* (*dns_base)(RpNetworkTransportSocket*);
};

typedef UNIQUE_PTR(RpNetworkTransportSocket) RpNetworkTransportSocketPtr;

static inline void
rp_network_transport_socket_set_transport_socket_callbacks(RpNetworkTransportSocket* self, RpNetworkTransportSocketCallbacks* callbacks)
{
    if (RP_IS_NETWORK_TRANSPORT_SOCKET(self)) \
        RP_NETWORK_TRANSPORT_SOCKET_GET_IFACE(self)->set_transport_socket_callbacks(self, callbacks);
}
static inline const char*
rp_network_transport_socket_protocol(RpNetworkTransportSocket* self)
{
    return RP_IS_NETWORK_TRANSPORT_SOCKET(self) ?
        RP_NETWORK_TRANSPORT_SOCKET_GET_IFACE(self)->protocol(self) : "";
}
static inline const char*
rp_network_transport_socket_failure_reason(RpNetworkTransportSocket* self)
{
    return RP_IS_NETWORK_TRANSPORT_SOCKET(self) ?
        RP_NETWORK_TRANSPORT_SOCKET_GET_IFACE(self)->failure_reason(self) : "";
}
static inline bool
rp_network_transport_socket_can_flush_close(RpNetworkTransportSocket* self)
{
    return RP_IS_NETWORK_TRANSPORT_SOCKET(self) ?
        RP_NETWORK_TRANSPORT_SOCKET_GET_IFACE(self)->can_flush_close(self) : false;
}
static inline int
rp_network_transport_socket_connect(RpNetworkTransportSocket* self, RpConnectionSocket* socket)
{
    return RP_IS_NETWORK_TRANSPORT_SOCKET(self) ?
        RP_NETWORK_TRANSPORT_SOCKET_GET_IFACE(self)->connect(self, socket) : -1;
}
static inline void
rp_network_transport_socket_close_socket(RpNetworkTransportSocket* self)
{
    if (RP_IS_NETWORK_TRANSPORT_SOCKET(self)) \
        RP_NETWORK_TRANSPORT_SOCKET_GET_IFACE(self)->close_socket(self);
}
static inline RpIoResult
rp_network_transport_socket_do_read(RpNetworkTransportSocket* self, evbuf_t* buffer)
{
    return RP_IS_NETWORK_TRANSPORT_SOCKET(self) ?
        RP_NETWORK_TRANSPORT_SOCKET_GET_IFACE(self)->do_read(self, buffer) :
        rp_io_result_ctor(RpPostIoAction_Close, 0, true, EINVAL);
}
static inline RpIoResult
rp_network_transport_socket_do_write(RpNetworkTransportSocket* self, evbuf_t* buffer, bool end_stream)
{
    return RP_IS_NETWORK_TRANSPORT_SOCKET(self) ?
        RP_NETWORK_TRANSPORT_SOCKET_GET_IFACE(self)->do_write(self, buffer, end_stream) :
        rp_io_result_ctor(RpPostIoAction_Close, 0, end_stream, EINVAL);
}
static inline void
rp_network_transport_socket_on_connected(RpNetworkTransportSocket* self)
{
    if (RP_IS_NETWORK_TRANSPORT_SOCKET(self)) \
        RP_NETWORK_TRANSPORT_SOCKET_GET_IFACE(self)->on_connected(self);
}
static inline RpSslConnectionInfo*
rp_network_transport_socket_ssl(RpNetworkTransportSocket* self)
{
    return RP_IS_NETWORK_TRANSPORT_SOCKET(self) ?
        RP_NETWORK_TRANSPORT_SOCKET_GET_IFACE(self)->ssl(self) : NULL;
}
static inline bool
rp_network_transport_socket_start_secure_transport(RpNetworkTransportSocket* self)
{
    return RP_IS_NETWORK_TRANSPORT_SOCKET(self) ?
        RP_NETWORK_TRANSPORT_SOCKET_GET_IFACE(self)->start_secure_transport(self) : false;
}
static inline RpIoHandle*
rp_network_transport_socket_create_io_handle(RpNetworkTransportSocket* self)
{
    return RP_IS_NETWORK_TRANSPORT_SOCKET(self) ?
        RP_NETWORK_TRANSPORT_SOCKET_GET_IFACE(self)->create_io_handle(self) :
        NULL;
}
static inline evdns_base_t*
rp_network_transport_socket_dns_base(RpNetworkTransportSocket* self)
{
    return RP_IS_NETWORK_TRANSPORT_SOCKET(self) ?
        RP_NETWORK_TRANSPORT_SOCKET_GET_IFACE(self)->dns_base(self) : NULL;
}

/**
 * A factory for creating transport sockets.
 **/
#define RP_TYPE_TRANSPORT_SOCKET_FACTORY_BASE rp_transport_socket_factory_base_get_type()
G_DECLARE_INTERFACE(RpTransportSocketFactoryBase, rp_transport_socket_factory_base, RP, TRANSPORT_SOCKET_FACTORY_BASE, GObject)

struct _RpTransportSocketFactoryBaseInterface {
    GTypeInterface parent_iface;

    bool (*implements_secure_transport)(RpTransportSocketFactoryBase*);
};

static inline bool
rp_transport_socket_factory_base_implements_secure_transport(RpTransportSocketFactoryBase* self)
{
    return RP_IS_TRANSPORT_SOCKET_FACTORY_BASE(self) ?
        RP_TRANSPORT_SOCKET_FACTORY_BASE_GET_IFACE(self)->implements_secure_transport(self) :
        false;
}


/**
 * A factory for creating upstream transport sockets. It will be associated to clusters.
 */
#define RP_TYPE_UPSTREAM_TRANSPORT_SOCKET_FACTORY rp_upstream_transport_socket_factory_get_type()
G_DECLARE_INTERFACE(RpUpstreamTransportSocketFactory, rp_upstream_transport_socket_factory, RP, UPSTREAM_TRANSPORT_SOCKET_FACTORY, RpTransportSocketFactoryBase)

struct _RpUpstreamTransportSocketFactoryInterface {
    RpTransportSocketFactoryBaseInterface parent_iface;

    RpNetworkTransportSocketPtr (*create_transport_socket)(RpUpstreamTransportSocketFactory*,
                                                            RpHostDescription*);
    bool (*supports_alpn)(RpUpstreamTransportSocketFactory*);
    const char* (*default_server_name_indication)(RpUpstreamTransportSocketFactory*);
    //TODO...
    SSL_CTX* (*ssl_ctx)(RpUpstreamTransportSocketFactory*);
};

typedef UNIQUE_PTR(RpUpstreamTransportSocketFactory) RpUpstreamTransportSocketFactoryPtr;

static inline RpNetworkTransportSocketPtr
rp_upstream_transport_socket_factory_create_transport_socket(RpUpstreamTransportSocketFactory* self, RpHostDescription* host)
{
    return RP_IS_UPSTREAM_TRANSPORT_SOCKET_FACTORY(self) ?
        RP_UPSTREAM_TRANSPORT_SOCKET_FACTORY_GET_IFACE(self)->create_transport_socket(self, host) :
        NULL;
}
static inline bool
rp_upstream_transport_socket_factory_supports_alpn(RpUpstreamTransportSocketFactory* self)
{
    return RP_IS_UPSTREAM_TRANSPORT_SOCKET_FACTORY(self) ?
        RP_UPSTREAM_TRANSPORT_SOCKET_FACTORY_GET_IFACE(self)->supports_alpn(self) :
        false;
}
static inline const char*
rp_upstream_transport_socket_factory_default_server_name_indication(RpUpstreamTransportSocketFactory* self)
{
    return RP_IS_UPSTREAM_TRANSPORT_SOCKET_FACTORY(self) ?
        RP_UPSTREAM_TRANSPORT_SOCKET_FACTORY_GET_IFACE(self)->default_server_name_indication(self) :
        NULL;
}
static inline SSL_CTX*
rp_upstream_transport_socket_factory_ssl_ctx(RpUpstreamTransportSocketFactory* self)
{
    return RP_IS_UPSTREAM_TRANSPORT_SOCKET_FACTORY(self) ?
        RP_UPSTREAM_TRANSPORT_SOCKET_FACTORY_GET_IFACE(self)->ssl_ctx(self) :
        NULL;
}


/**
 * A factory for creating downstream transport sockets. It will be associated to listeners.
 */
#define RP_TYPE_DOWNSTREAM_TRANSPORT_SOCKET_FACTORY rp_downstream_transport_socket_factory_get_type()
G_DECLARE_INTERFACE(RpDownstreamTransportSocketFactory, rp_downstream_transport_socket_factory, RP, DOWNSTREAM_TRANSPORT_SOCKET_FACTORY, RpTransportSocketFactoryBase)

struct _RpDownstreamTransportSocketFactoryInterface {
    RpTransportSocketFactoryBaseInterface parent_iface;

    RpNetworkTransportSocketPtr (*create_downstream_transport_socket)(RpDownstreamTransportSocketFactory*,
                                                                        evhtp_connection_t*);
};

typedef UNIQUE_PTR(RpDownstreamTransportSocketFactory) RpDownstreamTransportSocketFactoryPtr;

static inline RpNetworkTransportSocketPtr
rp_downstream_transport_socket_factory_create_downstream_transport_socket(RpDownstreamTransportSocketFactory* self, evhtp_connection_t* conn)
{
    return RP_IS_DOWNSTREAM_TRANSPORT_SOCKET_FACTORY(self) ?
        RP_DOWNSTREAM_TRANSPORT_SOCKET_FACTORY_GET_IFACE(self)->create_downstream_transport_socket(self, conn) :
        NULL;
}

G_END_DECLS
