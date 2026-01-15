/*
 * rp-tcp-conn-pool.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-conn-pool.h"
#include "rp-codec.h"
#include "rp-upstream.h"

G_BEGIN_DECLS

typedef struct _RpTcpConnPoolInstance* RpTcpConnPoolInstancePtr;

#define RP_TYPE_TCP_CONN_POOL_UPSTREAM_CALLBACKS rp_tcp_conn_pool_upstream_callbacks_get_type()
G_DECLARE_INTERFACE(RpTcpConnPoolUpstreamCallbacks, rp_tcp_conn_pool_upstream_callbacks, RP, TCP_CONN_POOL_UPSTREAM_CALLBACKS, GObject)

struct _RpTcpConnPoolUpstreamCallbacksInterface {
    GTypeInterface parent_iface;/*Network::ConnectionCallbacks*/

    void (*on_upstream_data)(RpTcpConnPoolUpstreamCallbacks*, evbuf_t*, bool);
};

static inline void
rp_tcp_conn_pool_upstream_callbacks_on_upstream_data(RpTcpConnPoolUpstreamCallbacks* self, evbuf_t* data, bool end_stream)
{
    if (RP_IS_TCP_CONN_POOL_UPSTREAM_CALLBACKS(self)) \
        RP_TCP_CONN_POOL_UPSTREAM_CALLBACKS_GET_IFACE(self)->on_upstream_data(self, data, end_stream);
}


/*
 * ConnectionData wraps a ClientConnection allocated to a caller. Open ClientConnections are
 * released back to the pool for re-use when their containing ConnectionData is destroyed.
 */
#define RP_TYPE_TCP_CONN_POOL_CONNECTION_DATA rp_tcp_conn_pool_connection_data_get_type()
G_DECLARE_INTERFACE(RpTcpConnPoolConnectionData, rp_tcp_conn_pool_connection_data, RP, TCP_CONN_POOL_CONNECTION_DATA, GObject)

struct _RpTcpConnPoolConnectionDataInterface {
    GTypeInterface parent_iface;

    RpNetworkClientConnection* (*connection)(RpTcpConnPoolConnectionData*);
    //TODO...void (*set_connection_state(RpTcpConnPoolConnectionData*);
    void (*add_upstream_callbacks)(RpTcpConnPoolConnectionData*,
                                    RpTcpConnPoolUpstreamCallbacks*);
};

static inline RpNetworkClientConnection*
rp_tcp_conn_pool_connection_data_connection(RpTcpConnPoolConnectionData* self)
{
    return RP_IS_TCP_CONN_POOL_CONNECTION_DATA(self) ?
        RP_TCP_CONN_POOL_CONNECTION_DATA_GET_IFACE(self)->connection(self) :
        NULL;
}
static inline void
rp_tcp_conn_pool_connection_data_add_upstream_callbacks(RpTcpConnPoolConnectionData* self,
                                                        RpTcpConnPoolUpstreamCallbacks* callbacks)
{
    if (RP_IS_TCP_CONN_POOL_CONNECTION_DATA(self)) \
        RP_TCP_CONN_POOL_CONNECTION_DATA_GET_IFACE(self)->add_upstream_callbacks(self, callbacks);
}


/**
 * Pool callbacks invoked in the context of a newStream() call, either synchronously or
 * asynchronously.
 */
#define RP_TYPE_TCP_CONN_POOL_CALLBACKS rp_tcp_conn_pool_callbacks_get_type()
G_DECLARE_INTERFACE(RpTcpConnPoolCallbacks, rp_tcp_conn_pool_callbacks, RP, TCP_CONN_POOL_CALLBACKS, GObject)

struct _RpTcpConnPoolCallbacksInterface {
    GTypeInterface parent_iface;

    void (*on_pool_failure)(RpTcpConnPoolCallbacks*,
                            RpPoolFailureReason_e,
                            const char*,
                            RpHostDescription*);
    void (*on_pool_ready)(RpTcpConnPoolCallbacks*,
                            RpTcpConnPoolConnectionData*,
                            RpHostDescription*);
};

static inline void
rp_tcp_conn_pool_callbacks_on_pool_failure(RpTcpConnPoolCallbacks* self, RpPoolFailureReason_e reason,
                                            const char* transport_failure_reason, RpHostDescription* host)
{
    if (RP_IS_TCP_CONN_POOL_CALLBACKS(self)) \
        RP_TCP_CONN_POOL_CALLBACKS_GET_IFACE(self)->on_pool_failure(self, reason, transport_failure_reason, host);
}
static inline void
rp_tcp_conn_pool_callbacks_on_pool_ready(RpTcpConnPoolCallbacks* self, RpTcpConnPoolConnectionData* conn_data, RpHostDescription* host)
{
    if (RP_IS_TCP_CONN_POOL_CALLBACKS(self)) \
        RP_TCP_CONN_POOL_CALLBACKS_GET_IFACE(self)->on_pool_ready(self, conn_data, host);
}


/**
 * An instance of a generic connection pool.
 */
#define RP_TYPE_TCP_CONN_POOL_INSTANCE rp_tcp_conn_pool_instance_get_type()
G_DECLARE_INTERFACE(RpTcpConnPoolInstance, rp_tcp_conn_pool_instance, RP, TCP_CONN_POOL_INSTANCE, RpConnectionPoolInstance)

struct _RpTcpConnPoolInstanceInterface {
    RpConnectionPoolInstanceInterface parent_iface;

    void (*close_connections)(RpTcpConnPoolInstance*);
    RpCancellable* (*new_connection)(RpTcpConnPoolInstance*,
                                        RpTcpConnPoolCallbacks*);
};

static inline void
rp_tcp_conn_pool_instance_close_connections(RpTcpConnPoolInstance* self)
{
    if (RP_IS_TCP_CONN_POOL_INSTANCE(self)) \
        RP_TCP_CONN_POOL_INSTANCE_GET_IFACE(self)->close_connections(self);
}
static inline RpCancellable*
rp_tcp_conn_pool_instance_new_connection(RpTcpConnPoolInstance* self, RpTcpConnPoolCallbacks* callbacks)
{
    return RP_IS_TCP_CONN_POOL_INSTANCE(self) ?
        RP_TCP_CONN_POOL_INSTANCE_GET_IFACE(self)->new_connection(self, callbacks) :
        NULL;
}

G_END_DECLS
