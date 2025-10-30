/*
 * rp-socket.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <evhtp.h>
#include "rp-io-handle.h"

G_BEGIN_DECLS

typedef struct _RpSslConnectionInfo RpSslConnectionInfo;


typedef enum {
    RpSocketType_Stream,
    RpSocketType_Datagram
} RpSocketType_e;


/**
 * Interfaces for providing a socket's various addresses. This is split into a getters interface
 * and a getters + setters interface. This is so that only the getters portion can be overridden
 * in certain cases.
 */
#define RP_TYPE_CONNECTION_INFO_PROVIDER rp_connection_info_provider_get_type()
G_DECLARE_INTERFACE(RpConnectionInfoProvider, rp_connection_info_provider, RP, CONNECTION_INFO_PROVIDER, GObject)

struct _RpConnectionInfoProviderInterface {
    GTypeInterface parent_iface;

    struct sockaddr* (*local_address)(RpConnectionInfoProvider*);
    struct sockaddr* (*direct_local_address)(RpConnectionInfoProvider*);
    bool (*local_address_restored)(RpConnectionInfoProvider*);
    struct sockaddr* (*remote_address)(RpConnectionInfoProvider*);
    struct sockaddr* (*direct_remote_address)(RpConnectionInfoProvider*);
    const char* (*requested_server_name)(RpConnectionInfoProvider*);
    guint64 (*connection_id)(RpConnectionInfoProvider*);
    const char* (*interface_name)(RpConnectionInfoProvider*);
    RpSslConnectionInfo* (*ssl_connection)(RpConnectionInfoProvider*);
    //TODO...
};

static inline struct sockaddr*
rp_connection_info_provider_local_address(RpConnectionInfoProvider* self)
{
    return RP_IS_CONNECTION_INFO_PROVIDER(self) ?
        RP_CONNECTION_INFO_PROVIDER_GET_IFACE(self)->local_address(self) : NULL;
}
static inline struct sockaddr*
rp_connection_info_provider_direct_local_address(RpConnectionInfoProvider* self)
{
    return RP_IS_CONNECTION_INFO_PROVIDER(self) ?
        RP_CONNECTION_INFO_PROVIDER_GET_IFACE(self)->direct_local_address(self) :
        NULL;
}
static inline bool
rp_connection_info_provider_local_address_restored(RpConnectionInfoProvider* self)
{
    return RP_IS_CONNECTION_INFO_PROVIDER(self) ?
        RP_CONNECTION_INFO_PROVIDER_GET_IFACE(self)->local_address_restored(self) :
        false;
}
static inline struct sockaddr*
rp_connection_info_provider_remote_address(RpConnectionInfoProvider* self)
{
    return RP_IS_CONNECTION_INFO_PROVIDER(self) ?
        RP_CONNECTION_INFO_PROVIDER_GET_IFACE(self)->remote_address(self) : NULL;
}
static inline struct sockaddr*
rp_connection_info_provider_direct_remote_address(RpConnectionInfoProvider* self)
{
    return RP_IS_CONNECTION_INFO_PROVIDER(self) ?
        RP_CONNECTION_INFO_PROVIDER_GET_IFACE(self)->direct_remote_address(self) :
        NULL;
}
static inline const char*
rp_connection_info_provider_requested_server_name(RpConnectionInfoProvider* self)
{
    return RP_IS_CONNECTION_INFO_PROVIDER(self) ?
        RP_CONNECTION_INFO_PROVIDER_GET_IFACE(self)->requested_server_name(self) :
        NULL;
}
static inline guint64
rp_connection_info_provider_connection_id(RpConnectionInfoProvider* self)
{
    return RP_IS_CONNECTION_INFO_PROVIDER(self) ?
        RP_CONNECTION_INFO_PROVIDER_GET_IFACE(self)->connection_id(self) : 0;
}
static inline const char*
rp_connection_info_provider_interface_name(RpConnectionInfoProvider* self)
{
    return RP_IS_CONNECTION_INFO_PROVIDER(self) ?
        RP_CONNECTION_INFO_PROVIDER_GET_IFACE(self)->interface_name(self) : NULL;
}
static inline RpSslConnectionInfo*
rp_connection_info_provider_ssl_connection(RpConnectionInfoProvider* self)
{
    return RP_IS_CONNECTION_INFO_PROVIDER(self) ?
        RP_CONNECTION_INFO_PROVIDER_GET_IFACE(self)->ssl_connection(self) : NULL;
}


#define RP_TYPE_CONNECTION_INFO_SETTER rp_connection_info_setter_get_type()
G_DECLARE_INTERFACE(RpConnectionInfoSetter, rp_connection_info_setter, RP, CONNECTION_INFO_SETTER, RpConnectionInfoProvider)

struct _RpConnectionInfoSetterInterface {
    RpConnectionInfoProviderInterface parent_iface;

    void (*set_local_address)(RpConnectionInfoSetter*, struct sockaddr*);
    void (*restore_local_address)(RpConnectionInfoSetter*, struct sockaddr*);
    void (*set_remote_address)(RpConnectionInfoSetter*, struct sockaddr*);
    void (*set_requested_server_name)(RpConnectionInfoSetter*, const char*);
    void (*set_connection_id)(RpConnectionInfoSetter*, guint64);
    void (*enable_setting_interface_name)(RpConnectionInfoSetter*, bool);
    void (*set_ssl_connection)(RpConnectionInfoSetter*, RpSslConnectionInfo*);
};

static inline void
rp_connection_info_setter_set_local_address(RpConnectionInfoSetter* self, struct sockaddr* local_address)
{
    if (RP_IS_CONNECTION_INFO_SETTER(self))
    {
        RP_CONNECTION_INFO_SETTER_GET_IFACE(self)->set_local_address(self, local_address);
    }
}
static inline void
rp_connection_info_setter_restore_local_address(RpConnectionInfoSetter* self, struct sockaddr* local_address)
{
    if (RP_IS_CONNECTION_INFO_SETTER(self))
    {
        RP_CONNECTION_INFO_SETTER_GET_IFACE(self)->restore_local_address(self, local_address);
    }
}
static inline void
rp_connection_info_setter_set_remote_address(RpConnectionInfoSetter* self, struct sockaddr* remote_address)
{
    if (RP_IS_CONNECTION_INFO_SETTER(self))
    {
        RP_CONNECTION_INFO_SETTER_GET_IFACE(self)->set_remote_address(self, remote_address);
    }
}
static inline void
rp_connection_info_setter_set_requested_server_name(RpConnectionInfoSetter* self, const char* requested_server_name)
{
    if (RP_IS_CONNECTION_INFO_SETTER(self))
    {
        RP_CONNECTION_INFO_SETTER_GET_IFACE(self)->set_requested_server_name(self, requested_server_name);
    }
}
static inline void rp_connection_info_setter_set_connection_id(RpConnectionInfoSetter* self, guint64 id)
{
    if (RP_IS_CONNECTION_INFO_SETTER(self))
    {
        RP_CONNECTION_INFO_SETTER_GET_IFACE(self)->set_connection_id(self, id);
    }
}
static inline void
rp_connection_info_setter_enable_setting_interface_name(RpConnectionInfoSetter* self, bool enable)
{
    if (RP_IS_CONNECTION_INFO_SETTER(self))
    {
        RP_CONNECTION_INFO_SETTER_GET_IFACE(self)->enable_setting_interface_name(self, enable);
    }
}
static inline void
rp_connection_info_setter_set_ssl_connection(RpConnectionInfoSetter* self, RpSslConnectionInfo* ssl_connection_info)
{
    if (RP_IS_CONNECTION_INFO_SETTER(self))
    {
        RP_CONNECTION_INFO_SETTER_GET_IFACE(self)->set_ssl_connection(self, ssl_connection_info);
    }
}


/**
 * Base class for Sockets
 */
#define RP_TYPE_SOCKET rp_socket_get_type()
G_DECLARE_INTERFACE(RpSocket, rp_socket, RP, SOCKET, GObject)

struct _RpSocketInterface {
    GTypeInterface parent_iface;

    RpConnectionInfoSetter* (*connection_info_provider)(RpSocket*);
    RpIoHandle* (*io_handle)(RpSocket*);
    RpSocketType_e (*socket_type)(RpSocket*);
    //TODO...
    void (*close)(RpSocket*);
    bool (*is_open)(RpSocket*);
    int (*connect)(RpSocket*, struct sockaddr*, const char*);
    int (*sockfd)(RpSocket*);
    //TODO...
};

static inline RpConnectionInfoSetter*
rp_socket_connection_info_provider(RpSocket* self)
{
    return RP_IS_SOCKET(self) ?
        RP_SOCKET_GET_IFACE(self)->connection_info_provider(self) : NULL;
}
static inline RpIoHandle*
rp_socket_io_handle(RpSocket* self)
{
    return RP_IS_SOCKET(self) ?
        RP_SOCKET_GET_IFACE(self)->io_handle(self) : NULL;
}
static inline RpSocketType_e
rp_socket_socket_type(RpSocket* self)
{
    return RP_IS_SOCKET(self) ?
        RP_SOCKET_GET_IFACE(self)->socket_type(self) : RpSocketType_Stream;
}
static inline void
rp_socket_close(RpSocket* self)
{
    if (RP_IS_SOCKET(self)) \
        RP_SOCKET_GET_IFACE(self)->close(self);
}
static inline bool
rp_socket_is_open(RpSocket* self)
{
    return RP_IS_SOCKET(self) ? RP_SOCKET_GET_IFACE(self)->is_open(self) : false;
}
static inline int
rp_socket_connect(RpSocket* self, struct sockaddr* address, const char* requested_server_name)
{
    return RP_IS_SOCKET(self) ?
        RP_SOCKET_GET_IFACE(self)->connect(self, address, requested_server_name) :
        -1;
}
static inline int
rp_socket_sockfd(RpSocket* self)
{
    return RP_IS_SOCKET(self) ? RP_SOCKET_GET_IFACE(self)->sockfd(self) : -1;
}


/**
 * Base connection interface for all SSL connections.
 */
#define RP_TYPE_SSL_CONNECTION_INFO rp_ssl_connection_info_get_type()
G_DECLARE_INTERFACE(RpSslConnectionInfo, rp_ssl_connection_info, RP, SSL_CONNECTION_INFO, GObject)

struct _RpSslConnectionInfoInterface {
    GTypeInterface parent_iface;

    bool (*peer_certificate_presented)(RpSslConnectionInfo*);
    bool (*peer_certificate_validated)(RpSslConnectionInfo*);
    //TODO...
    const char* (*subject_local_certificate)(RpSslConnectionInfo*);
    //TODO...
    const char* (*sni)(RpSslConnectionInfo*);
};

static inline bool
rp_ssl_connection_info_peer_certificate_presented(RpSslConnectionInfo* self)
{
    return RP_IS_SSL_CONNECTION_INFO(self) ?
        RP_SSL_CONNECTION_INFO_GET_IFACE(self)->peer_certificate_presented(self) :
        false;
}
static inline bool
rp_ssl_connection_info_peer_certificate_validated(RpSslConnectionInfo* self)
{
    return RP_IS_SSL_CONNECTION_INFO(self) ?
        RP_SSL_CONNECTION_INFO_GET_IFACE(self)->peer_certificate_validated(self) :
        false;
}
static inline const char*
rp_ssl_connection_info_subject_local_certificate(RpSslConnectionInfo* self)
{
    return RP_IS_SSL_CONNECTION_INFO(self) ?
        RP_SSL_CONNECTION_INFO_GET_IFACE(self)->subject_local_certificate(self) :
        NULL;
}
static inline const char*
rp_ssl_connection_info_sni(RpSslConnectionInfo* self)
{
    return RP_IS_SSL_CONNECTION_INFO(self) ?
        RP_SSL_CONNECTION_INFO_GET_IFACE(self)->sni(self) : NULL;
}

G_END_DECLS
