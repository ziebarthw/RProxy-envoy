/*
 * rp-net-connection.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-dispatcher.h"
#include "rp-socket.h"

G_BEGIN_DECLS

typedef struct _RpStreamInfo RpStreamInfo;

/**
 * Events that occur on a connection.
 */
typedef enum {
    RpNetworkConnectionEvent_None,
    RpNetworkConnectionEvent_RemoteClose,
    RpNetworkConnectionEvent_LocalClose,
    RpNetworkConnectionEvent_Connected,
    RpNetworkConnectionEvent_ConnectedZeroRtt,
} RpNetworkConnectionEvent_e;

/**
 * Connections have both a read and write buffer.
 */
typedef enum {
    RpNetworkConnectionBufferType_Read,
    RpNetworkConnectionBufferType_Write
} RpNetworkConnectionBufferType_e;

/**
 * Type of connection close to perform.
 */
typedef enum {
    RpNetworkConnectionCloseType_None,
    RpNetworkConnectionCloseType_FlushWrite,
    RpNetworkConnectionCloseType_NoFlush,
    RpNetworkConnectionCloseType_FlushWriteAndDelay,
    RpNetworkConnectionCloseType_Abort,
    RpNetworkConnectionCloseType_AbortReset
} RpNetworkConnectionCloseType_e;

/**
 * Type of connection close which is detected from the socket.
 */
typedef enum {
    RpDetectedCloseType_Normal,
    RpDetectedCloseType_LocalReset,
    RpDetectedCloseType_RemoteReset,
} RpDetectedCloseType_e;


/**
 * Network level callbacks that happen on a connection.
 */
#define RP_TYPE_NETWORK_CONNECTION_CALLBACKS rp_network_connection_callbacks_get_type()
G_DECLARE_INTERFACE(RpNetworkConnectionCallbacks, rp_network_connection_callbacks, RP, NETWORK_CONNECTION_CALLBACKS, GObject)

struct _RpNetworkConnectionCallbacksInterface {
    GTypeInterface parent_iface;

    void (*on_event)(RpNetworkConnectionCallbacks*, RpNetworkConnectionEvent_e);
    void (*on_above_write_buffer_high_water_mark)(RpNetworkConnectionCallbacks*);
    void (*on_below_write_buffer_low_watermark)(RpNetworkConnectionCallbacks*);
};

static inline void
rp_network_connection_callbacks_on_event(RpNetworkConnectionCallbacks* self, RpNetworkConnectionEvent_e event)
{
    if (RP_IS_NETWORK_CONNECTION_CALLBACKS(self))
    {
        RP_NETWORK_CONNECTION_CALLBACKS_GET_IFACE(self)->on_event(self, event);
    }
}
static inline void
rp_network_connection_callbacks_on_above_write_buffer_high_water_mark(RpNetworkConnectionCallbacks* self)
{
    if (RP_IS_NETWORK_CONNECTION_CALLBACKS(self))
    {
        RP_NETWORK_CONNECTION_CALLBACKS_GET_IFACE(self)->on_above_write_buffer_high_water_mark(self);
    }
}
static inline void
rp_network_connection_callbacks_on_below_write_buffer_low_watermark(RpNetworkConnectionCallbacks* self)
{
    if (RP_IS_NETWORK_CONNECTION_CALLBACKS(self))
    {
        RP_NETWORK_CONNECTION_CALLBACKS_GET_IFACE(self)->on_below_write_buffer_low_watermark(self);
    }
}


/**
 * An abstract raw connection. Free the connection or call close() to disconnect.
 */
#define RP_TYPE_NETWORK_CONNECTION rp_network_connection_get_type()
G_DECLARE_INTERFACE(RpNetworkConnection, rp_network_connection, RP, NETWORK_CONNECTION, GObject)

typedef enum {
    RpNetworkConnectionState_Open,
    RpNetworkConnectionState_Closing,
    RpNetworkConnectionState_Closed
} RpNetworkConnectionState_e;

typedef enum {
    RpReadDisabaleStatus_NoTransition,
    RpReadDisabaleStatus_StillReadDisabled,
    RpReadDisabaleStatus_TransitionedToReadEnabled,
    RpReadDisabaleStatus_TransitionedToReadDisabled
} RpReadDisabaleStatus_e;

typedef bool (*connection_bytes_sent_cb)(RpNetworkConnection*, guint64);

struct _RpNetworkConnectionInterface {
    GTypeInterface parent_iface;

    void (*add_connection_callbacks)(RpNetworkConnection*, RpNetworkConnectionCallbacks*);
    void (*remove_connection_callbacks)(RpNetworkConnection*, RpNetworkConnectionCallbacks*);
    void (*add_bytes_sent_callback)(RpNetworkConnection*, connection_bytes_sent_cb);
    void (*enable_half_close)(RpNetworkConnection*, bool);
    bool (*is_half_close_enabled)(RpNetworkConnection*);
    void (*close)(RpNetworkConnection*, RpNetworkConnectionCloseType_e);
    RpDetectedCloseType_e (*detected_close_type)(RpNetworkConnection*);
    RpDispatcher* (*dispatcher)(RpNetworkConnection*);
    guint64 (*id)(RpNetworkConnection*);
    const char* (*next_protocol)(RpNetworkConnection*);
    void (*no_delay)(RpNetworkConnection*, bool);
    RpReadDisabaleStatus_e (*read_disable)(RpNetworkConnection*, bool);
    void (*detect_early_close_when_read_disabled)(RpNetworkConnection*, bool);
    bool (*read_enabled)(RpNetworkConnection*);
    RpConnectionInfoSetter* (*connection_info_setter)(RpNetworkConnection*);
    RpConnectionInfoProvider* (*connection_info_provider)(RpNetworkConnection*);
    RpSslConnectionInfo* (*ssl)(RpNetworkConnection*);
    const char* (*requested_server_name)(RpNetworkConnection*);
    RpNetworkConnectionState_e (*state)(RpNetworkConnection*);
    bool (*connecting)(RpNetworkConnection*);
    void (*write)(RpNetworkConnection*, evbuf_t*, bool);
void (*flush)(RpNetworkConnection*);
    void (*set_buffer_limits)(RpNetworkConnection*, guint32);
    guint32 (*buffer_limit)(RpNetworkConnection*);
    bool (*above_high_water_mark)(RpNetworkConnection*);
    void (*set_delayed_close_timeout)(RpNetworkConnection*, guint64);
    RpStreamInfo* (*stream_info)(RpNetworkConnection*);
    const char* (*transport_failure_reason)(RpNetworkConnection*);
    const char* (*local_close_reason)(RpNetworkConnection*);
    bool (*start_secure_transport)(RpNetworkConnection*);
    int (*sockfd)(RpNetworkConnection*);
};

static inline void
rp_network_connection_add_connection_callbacks(RpNetworkConnection* self, RpNetworkConnectionCallbacks* callbacks)
{
    if (RP_IS_NETWORK_CONNECTION(self))
    {
        RP_NETWORK_CONNECTION_GET_IFACE(self)->add_connection_callbacks(self, callbacks);
    }
}
static inline void
rp_network_connection_enable_half_close(RpNetworkConnection* self, bool enabled)
{
    if (RP_IS_NETWORK_CONNECTION(self)) \
        RP_NETWORK_CONNECTION_GET_IFACE(self)->enable_half_close(self, enabled);
}
static inline bool
rp_network_connection_is_half_close_enabled(RpNetworkConnection* self)
{
    return RP_IS_NETWORK_CONNECTION(self) ?
        RP_NETWORK_CONNECTION_GET_IFACE(self)->is_half_close_enabled(self) : false;
}
static inline void
rp_network_connection_close(RpNetworkConnection* self, RpNetworkConnectionCloseType_e type)
{
    if (RP_IS_NETWORK_CONNECTION(self))
    {
        RP_NETWORK_CONNECTION_GET_IFACE(self)->close(self, type);
    }
}
static inline RpDispatcher*
rp_network_connection_dispatcher(RpNetworkConnection* self)
{
    return RP_IS_NETWORK_CONNECTION(self) ?
        RP_NETWORK_CONNECTION_GET_IFACE(self)->dispatcher(self) : NULL;
}
static inline guint64
rp_network_connection_id(RpNetworkConnection* self)
{
    return RP_IS_NETWORK_CONNECTION(self) ?
        RP_NETWORK_CONNECTION_GET_IFACE(self)->id(self) : 0;
}
static inline RpNetworkConnectionState_e
rp_network_connection_state(RpNetworkConnection* self)
{
    return RP_IS_NETWORK_CONNECTION(self) ?
        RP_NETWORK_CONNECTION_GET_IFACE(self)->state(self) :
        RpNetworkConnectionState_Closed;
}
static inline const char*
rp_network_connection_transport_failure_reason(RpNetworkConnection* self)
{
    return RP_IS_NETWORK_CONNECTION(self) ?
        RP_NETWORK_CONNECTION_GET_IFACE(self)->transport_failure_reason(self) : "";
}
static inline void
rp_network_connection_no_delay(RpNetworkConnection* self, bool enable)
{
    if (RP_IS_NETWORK_CONNECTION(self))
    {
        RP_NETWORK_CONNECTION_GET_IFACE(self)->no_delay(self, enable);
    }
}
static inline void
rp_network_connection_read_disable(RpNetworkConnection* self, bool disable)
{
    if (RP_IS_NETWORK_CONNECTION(self))
    {
        RP_NETWORK_CONNECTION_GET_IFACE(self)->read_disable(self, disable);
    }
}
static inline bool
rp_network_connection_connecting(RpNetworkConnection* self)
{
    return RP_IS_NETWORK_CONNECTION(self) ?
        RP_NETWORK_CONNECTION_GET_IFACE(self)->connecting(self) : false;
}
static inline void
rp_network_connection_write(RpNetworkConnection* self, evbuf_t* data, bool end_stream)
{
    if (RP_IS_NETWORK_CONNECTION(self))
    {
        RP_NETWORK_CONNECTION_GET_IFACE(self)->write(self, data, end_stream);
    }
}
static inline void
rp_network_connection_flush(RpNetworkConnection* self)
{
    if (RP_IS_NETWORK_CONNECTION(self)) \
        RP_NETWORK_CONNECTION_GET_IFACE(self)->flush(self);
}
static inline const char*
rp_network_connection_local_close_reason(RpNetworkConnection* self)
{
    return RP_IS_NETWORK_CONNECTION(self) ?
        RP_NETWORK_CONNECTION_GET_IFACE(self)->local_close_reason(self) : "";
}
static inline void
rp_network_connection_detect_early_close_when_read_disabled(RpNetworkConnection* self, bool should_detect)
{
    if (RP_IS_NETWORK_CONNECTION(self)) \
        RP_NETWORK_CONNECTION_GET_IFACE(self)->detect_early_close_when_read_disabled(self, should_detect);
}
static inline RpConnectionInfoSetter*
rp_network_connection_connection_info_setter(RpNetworkConnection* self)
{
    return RP_IS_NETWORK_CONNECTION(self) ?
        RP_NETWORK_CONNECTION_GET_IFACE(self)->connection_info_setter(self) :
        NULL;
}
static inline RpConnectionInfoProvider*
rp_network_connection_connection_info_provider(RpNetworkConnection* self)
{
    return RP_IS_NETWORK_CONNECTION(self) ?
        RP_NETWORK_CONNECTION_GET_IFACE(self)->connection_info_provider(self) :
        NULL;
}
static inline RpSslConnectionInfo*
rp_network_connection_ssl(RpNetworkConnection* self)
{
    return RP_IS_NETWORK_CONNECTION(self) ?
        RP_NETWORK_CONNECTION_GET_IFACE(self)->ssl(self) : NULL;
}
static inline RpStreamInfo*
rp_network_connection_stream_info(RpNetworkConnection* self)
{
    return RP_IS_NETWORK_CONNECTION(self) ?
        RP_NETWORK_CONNECTION_GET_IFACE(self)->stream_info(self) : NULL;
}
static inline int
rp_network_connection_sockfd(RpNetworkConnection* self)
{
    return RP_IS_NETWORK_CONNECTION(self) ?
        RP_NETWORK_CONNECTION_GET_IFACE(self)->sockfd(self) : -1;
}


/**
 * Connections servicing inbound connects.
 */
#define RP_TYPE_NETWORK_SERVER_CONNECTION rp_network_server_connection_get_type()
G_DECLARE_INTERFACE(RpNetworkServerConnection, rp_network_server_connection, RP, NETWORK_SERVER_CONNECTION, RpNetworkConnection)

struct _RpNetworkServerConnectionInterface {
    RpNetworkConnectionInterface parent_iface;

    void (*set_transport_socket_connect_timeout)(RpNetworkServerConnection*, guint64);
};

static inline void
rp_network_server_connection_set_transport_socket_connect_timeout(RpNetworkServerConnection* self, guint64 timeout_ms)
{
    if (RP_IS_NETWORK_SERVER_CONNECTION(self))
    {
        RP_NETWORK_SERVER_CONNECTION_GET_IFACE(self)->set_transport_socket_connect_timeout(self, timeout_ms);
    }
}


/**
 * Connections capable of outbound connects.
 */
#define RP_TYPE_NETWORK_CLIENT_CONNECTION rp_network_client_connection_get_type()
G_DECLARE_INTERFACE(RpNetworkClientConnection, rp_network_client_connection, RP, NETWORK_CLIENT_CONNECTION, RpNetworkConnection)

struct _RpNetworkClientConnectionInterface {
    RpNetworkConnectionInterface parent_iface;

    void (*connect)(RpNetworkClientConnection*);
};

static inline void
rp_network_client_connection_connect(RpNetworkClientConnection* self)
{
    if (RP_IS_NETWORK_CLIENT_CONNECTION(self))
    {
        RP_NETWORK_CLIENT_CONNECTION_GET_IFACE(self)->connect(self);
    }
}

G_END_DECLS
