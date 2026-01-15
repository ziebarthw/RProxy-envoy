/*
 * rp-http-conn-pool.h
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

typedef struct _RpHttpConnectionPoolInstance* RpHttpConnectionPoolInstancePtr;

/**
 * Pool callbacks invoked in the context of a newStream() call, either synchronously or
 * asynchronously.
 */
#define RP_TYPE_HTTP_CONN_POOL_CALLBACKS rp_http_conn_pool_callbacks_get_type()
G_DECLARE_INTERFACE(RpHttpConnPoolCallbacks, rp_http_conn_pool_callbacks, RP, HTTP_CONN_POOL_CALLBACKS, GObject)

struct _RpHttpConnPoolCallbacksInterface {
    GTypeInterface parent_iface;

    void (*on_pool_failure)(RpHttpConnPoolCallbacks*,
                            RpPoolFailureReason_e,
                            const char*,
                            RpHostDescription*);
    void (*on_pool_ready)(RpHttpConnPoolCallbacks*,
                            RpRequestEncoder*,
                            RpHostDescription*,
                            RpStreamInfo*,
                            evhtp_proto);
};

static inline void
rp_http_conn_pool_callbacks_on_pool_failure(RpHttpConnPoolCallbacks* self, RpPoolFailureReason_e reason,
                                            const char* transport_failure_reason, RpHostDescription* host)
{
    if (RP_IS_HTTP_CONN_POOL_CALLBACKS(self))
    {
        RP_HTTP_CONN_POOL_CALLBACKS_GET_IFACE(self)->on_pool_failure(self, reason, transport_failure_reason, host);
    }
}
static inline void
rp_http_conn_pool_callbacks_on_pool_ready(RpHttpConnPoolCallbacks* self, RpRequestEncoder* encoder,
                                            RpHostDescription* host, RpStreamInfo* info, evhtp_proto protocol)
{
    if (RP_IS_HTTP_CONN_POOL_CALLBACKS(self))
    {
        RP_HTTP_CONN_POOL_CALLBACKS_GET_IFACE(self)->on_pool_ready(self, encoder, host, info, protocol);
    }
}


/**
 * An instance of a generic connection pool.
 */
#define RP_TYPE_HTTP_CONNECTION_POOL_INSTANCE rp_http_connection_pool_instance_get_type()
G_DECLARE_INTERFACE(RpHttpConnectionPoolInstance, rp_http_connection_pool_instance, RP, HTTP_CONNECTION_POOL_INSTANCE, RpConnectionPoolInstance)

typedef struct _RpHttpConnPoolInstStreamOptions* RpHttpConnPoolInstStreamOptionsPtr;
typedef struct _RpHttpConnPoolInstStreamOptions RpHttpConnPoolInstStreamOptions;
struct _RpHttpConnPoolInstStreamOptions {
    bool m_can_send_early_data;
    bool m_can_use_http3;
};
static inline RpHttpConnPoolInstStreamOptions
rp_http_conn_pool_inst_stream_options_ctor(bool can_send_early_data, bool can_use_http3)
{
    RpHttpConnPoolInstStreamOptions self = {
        .m_can_send_early_data = can_send_early_data,
        .m_can_use_http3 = can_use_http3
    };
    return self;
}

struct _RpHttpConnectionPoolInstanceInterface {
    RpConnectionPoolInstanceInterface parent_iface;

    bool (*has_active_connections)(RpHttpConnectionPoolInstance*);
    RpCancellable* (*new_stream)(RpHttpConnectionPoolInstance*, RpResponseDecoder*, RpHttpConnPoolCallbacks*, RpHttpConnPoolInstStreamOptionsPtr);
    const char* (*protocol_description)(RpHttpConnectionPoolInstance*);
};

static inline bool
rp_http_connection_pool_instance_has_active_connections(RpHttpConnectionPoolInstance* self)
{
    return RP_IS_HTTP_CONNECTION_POOL_INSTANCE(self) ?
        RP_HTTP_CONNECTION_POOL_INSTANCE_GET_IFACE(self)->has_active_connections(self) :
        false;
}
static inline RpCancellable*
rp_http_connection_pool_instance_new_stream(RpHttpConnectionPoolInstance* self, RpResponseDecoder* decoder,
                                            RpHttpConnPoolCallbacks* callbacks, RpHttpConnPoolInstStreamOptionsPtr options)
{
    return RP_IS_HTTP_CONNECTION_POOL_INSTANCE(self) ?
        RP_HTTP_CONNECTION_POOL_INSTANCE_GET_IFACE(self)->new_stream(self, decoder, callbacks, options) :
        NULL;
}
static inline const char*
rp_http_connection_pool_instance_protocol_description(RpHttpConnectionPoolInstance* self)
{
    return RP_IS_HTTP_CONNECTION_POOL_INSTANCE(self) ?
        RP_HTTP_CONNECTION_POOL_INSTANCE_GET_IFACE(self)->protocol_description(self) :
        "";
}


/**
 * Pool callbacks invoked to track the lifetime of connections in the pool.
 */
#define RP_TYPE_HTTP_CONNECTION_LIFETIME_CALLBACKS rp_http_connection_lifetime_callbacks_get_type()
G_DECLARE_INTERFACE(RpHttpConnectionLifetimeCallbacks, rp_http_connection_lifetime_callbacks, RP, HTTP_CONNECTION_LIFETIME_CALLBACKS, GObject)

struct _RpHttpConnectionLifetimeCallbacksInterface {
    GTypeInterface parent_iface;

    void (*on_connection_open)(RpHttpConnectionLifetimeCallbacks*, RpHttpConnectionPoolInstance*, RpNetworkConnection*);
    void (*on_connection_draining)(RpHttpConnectionLifetimeCallbacks*, RpNetworkConnection*);
};

static inline void
rp_http_connection_lifetime_callbacks_on_connection_open(RpHttpConnectionLifetimeCallbacks* self,
                                                            RpHttpConnectionPoolInstance* pool,
                                                            RpNetworkConnection* connection)
{
    if (RP_IS_HTTP_CONNECTION_LIFETIME_CALLBACKS(self))
    {
        RP_HTTP_CONNECTION_LIFETIME_CALLBACKS_GET_IFACE(self)->on_connection_open(self, pool, connection);
    }
}
static inline void
rp_http_connection_lifetime_callbacks_on_connection_draining(RpHttpConnectionLifetimeCallbacks* self, RpNetworkConnection* connection)
{
    if (RP_IS_HTTP_CONNECTION_LIFETIME_CALLBACKS(self))
    {
        RP_HTTP_CONNECTION_LIFETIME_CALLBACKS_GET_IFACE(self)->on_connection_draining(self, connection);
    }
}

G_END_DECLS
