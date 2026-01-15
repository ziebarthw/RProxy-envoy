/*
 * rp-listen-socket.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"
#include "rp-socket.h"

G_BEGIN_DECLS

/**
 * A socket passed to a connection. For server connections this represents the accepted socket, and
 * for client connections this represents the socket being connected to a remote address.
 *
 * TODO(jrajahalme): Hide internals (e.g., fd) from listener filters by providing callbacks filters
 * may need (set/getsockopt(), peek(), recv(), etc.)
 */
#define RP_TYPE_CONNECTION_SOCKET rp_connection_socket_get_type()
G_DECLARE_INTERFACE(RpConnectionSocket, rp_connection_socket, RP, CONNECTION_SOCKET, GObject /*RpSocket*/)

struct _RpConnectionSocketInterface {
    /*RpSocketInterface parent_iface;*/
    GTypeInterface parent_iface;

    void (*set_detected_transport_protocol)(RpConnectionSocket*, const char*);
    const char* (*detected_transport_protocol)(RpConnectionSocket*);
    void (*set_requested_application_protocols)(RpConnectionSocket*, GArray*);
    GArray* (*requested_application_protocols)(RpConnectionSocket*);
    void (*set_requested_server_name)(RpConnectionSocket*, const char*);
    const char* (*requested_server_name)(RpConnectionSocket*);
    guint32 (*last_round_trip_time)(RpConnectionSocket*);
    guint64 (*congestion_window_in_bytes)(RpConnectionSocket*);
};

static inline void
rp_connection_socket_set_detected_transport_protocol(RpConnectionSocket* self, const char* protocol)
{
    if (RP_IS_CONNECTION_SOCKET(self)) \
        RP_CONNECTION_SOCKET_GET_IFACE(self)->set_detected_transport_protocol(self, protocol);
}
static inline void
rp_connection_socket_set_requested_application_protocols(RpConnectionSocket* self, GArray* protocol)
{
    if (RP_IS_CONNECTION_SOCKET(self)) \
        RP_CONNECTION_SOCKET_GET_IFACE(self)->set_requested_application_protocols(self, protocol);
}
static inline GArray*
rp_connection_socket_requested_application_protocols(RpConnectionSocket* self)
{
    return RP_IS_CONNECTION_SOCKET(self) ?
        RP_CONNECTION_SOCKET_GET_IFACE(self)->requested_application_protocols(self) :
        NULL;
}
static inline void rp_connection_socket_set_requested_server_name(RpConnectionSocket* self, const char* server_name)
{
    if (RP_IS_CONNECTION_SOCKET(self)) \
        RP_CONNECTION_SOCKET_GET_IFACE(self)->set_requested_server_name(self, server_name);
}
static inline const char*
rp_connection_socket_requested_server_name(RpConnectionSocket* self)
{
    return RP_IS_CONNECTION_SOCKET(self) ?
        RP_CONNECTION_SOCKET_GET_IFACE(self)->requested_server_name(self) :
        NULL;
}
static inline guint32
rp_connection_socket_last_round_trip_time(RpConnectionSocket* self)
{
    return RP_IS_CONNECTION_SOCKET(self) ?
        RP_CONNECTION_SOCKET_GET_IFACE(self)->last_round_trip_time(self) :
        0;
}
static inline guint64
rp_connection_socket_congestion_window_in_bytes(RpConnectionSocket* self)
{
    return RP_IS_CONNECTION_SOCKET(self) ?
        RP_CONNECTION_SOCKET_GET_IFACE(self)->congestion_window_in_bytes(self) :
        0;
}

G_END_DECLS
