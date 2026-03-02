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
#include "rproxy.h"
#include "rp-api-os-sys-calls.h"
#include "rp-io-handle.h"
#include "rp-net-address.h"

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

    RpNetworkAddressInstanceConstSharedPtr (*local_address)(const RpConnectionInfoProvider*);
    RpNetworkAddressInstanceConstSharedPtr (*direct_local_address)(const RpConnectionInfoProvider*);
    bool (*local_address_restored)(const RpConnectionInfoProvider*);
    RpNetworkAddressInstanceConstSharedPtr (*remote_address)(const RpConnectionInfoProvider*);
    RpNetworkAddressInstanceConstSharedPtr (*direct_remote_address)(const RpConnectionInfoProvider*);
    const char* (*requested_server_name)(const RpConnectionInfoProvider*);
    guint64 (*connection_id)(const RpConnectionInfoProvider*);
    const char* (*interface_name)(const RpConnectionInfoProvider*);
    RpSslConnectionInfo* (*ssl_connection)(const RpConnectionInfoProvider*);
    //TODO...
};

typedef SHARED_PTR(RpConnectionInfoProvider) RpConnectionInfoProviderSharedPtr;
typedef const SHARED_PTR(RpConnectionInfoProvider) RpConnectionInfoProviderConstSharedPtr;

static inline gboolean
rp_connection_info_provider_is_a(RpConnectionInfoProviderConstSharedPtr self)
{
    g_return_val_if_fail(self != NULL, false);
    return RP_IS_CONNECTION_INFO_PROVIDER((GObject*)self);
}
static inline void
rp_connection_info_provider_set_object(RpConnectionInfoProviderSharedPtr* dst, RpConnectionInfoProviderConstSharedPtr src)
{
    g_return_if_fail(dst != NULL);
    g_set_object((GObject**)dst, (GObject*)src);
}
static inline RpConnectionInfoProviderInterface*
rp_connection_info_provider_iface(RpConnectionInfoProviderConstSharedPtr self)
{
    g_return_val_if_fail(rp_connection_info_provider_is_a(self), NULL);
    return RP_CONNECTION_INFO_PROVIDER_GET_IFACE((GObject*)self);
}
static inline RpNetworkAddressInstanceConstSharedPtr
rp_connection_info_provider_local_address(RpConnectionInfoProviderConstSharedPtr self)
{
    g_return_val_if_fail(rp_connection_info_provider_is_a(self), NULL);
    RpConnectionInfoProviderInterface* iface = rp_connection_info_provider_iface(self);
    return iface->local_address ? iface->local_address(self) : NULL;
}
static inline RpNetworkAddressInstanceConstSharedPtr
rp_connection_info_provider_direct_local_address(RpConnectionInfoProviderConstSharedPtr self)
{
    g_return_val_if_fail(rp_connection_info_provider_is_a(self), NULL);
    RpConnectionInfoProviderInterface* iface = rp_connection_info_provider_iface(self);
    return iface->direct_local_address ? iface->direct_local_address(self) : NULL;
}
static inline bool
rp_connection_info_provider_local_address_restored(RpConnectionInfoProviderConstSharedPtr self)
{
    g_return_val_if_fail(rp_connection_info_provider_is_a(self), false);
    RpConnectionInfoProviderInterface* iface = rp_connection_info_provider_iface(self);
    return iface->local_address_restored ?
        iface->local_address_restored(self) : false;
}
static inline RpNetworkAddressInstanceConstSharedPtr
rp_connection_info_provider_remote_address(RpConnectionInfoProviderConstSharedPtr self)
{
    g_return_val_if_fail(rp_connection_info_provider_is_a(self), NULL);
    RpConnectionInfoProviderInterface* iface = rp_connection_info_provider_iface(self);
    return iface->remote_address ? iface->remote_address(self) : NULL;
}
static inline RpNetworkAddressInstanceConstSharedPtr
rp_connection_info_provider_direct_remote_address(RpConnectionInfoProviderConstSharedPtr self)
{
    g_return_val_if_fail(rp_connection_info_provider_is_a(self), NULL);
    RpConnectionInfoProviderInterface* iface = rp_connection_info_provider_iface(self);
    return iface->direct_remote_address ? iface->direct_remote_address(self) : NULL;
}
static inline const char*
rp_connection_info_provider_requested_server_name(RpConnectionInfoProviderConstSharedPtr self)
{
    g_return_val_if_fail(rp_connection_info_provider_is_a(self), NULL);
    RpConnectionInfoProviderInterface* iface = rp_connection_info_provider_iface(self);
    return iface->requested_server_name ?
        iface->requested_server_name(self) : NULL;
}
static inline guint64
rp_connection_info_provider_connection_id(RpConnectionInfoProviderConstSharedPtr self)
{
    g_return_val_if_fail(rp_connection_info_provider_is_a(self), 0);
    RpConnectionInfoProviderInterface* iface = rp_connection_info_provider_iface(self);
    return iface->connection_id ? iface->connection_id(self) : 0;
}
static inline const char*
rp_connection_info_provider_interface_name(RpConnectionInfoProviderConstSharedPtr self)
{
    g_return_val_if_fail(rp_connection_info_provider_is_a(self), NULL);
    RpConnectionInfoProviderInterface* iface = rp_connection_info_provider_iface(self);
    return iface->interface_name ? iface->interface_name(self) : NULL;
}
static inline RpSslConnectionInfo*
rp_connection_info_provider_ssl_connection(RpConnectionInfoProviderConstSharedPtr self)
{
    g_return_val_if_fail(rp_connection_info_provider_is_a(self), NULL);
    RpConnectionInfoProviderInterface* iface = rp_connection_info_provider_iface(self);
    return iface->ssl_connection ? iface->ssl_connection(self) : NULL;
}


#define RP_TYPE_CONNECTION_INFO_SETTER rp_connection_info_setter_get_type()
G_DECLARE_INTERFACE(RpConnectionInfoSetter, rp_connection_info_setter, RP, CONNECTION_INFO_SETTER, RpConnectionInfoProvider)

struct _RpConnectionInfoSetterInterface {
    RpConnectionInfoProviderInterface parent_iface;

    void (*set_local_address)(RpConnectionInfoSetter*, RpNetworkAddressInstanceConstSharedPtr);
    void (*restore_local_address)(RpConnectionInfoSetter*, RpNetworkAddressInstanceConstSharedPtr);
    void (*set_remote_address)(RpConnectionInfoSetter*, RpNetworkAddressInstanceConstSharedPtr);
    void (*set_requested_server_name)(RpConnectionInfoSetter*, const char*);
    void (*set_connection_id)(RpConnectionInfoSetter*, guint64);
    void (*enable_setting_interface_name)(RpConnectionInfoSetter*, bool);
    void (*set_ssl_connection)(RpConnectionInfoSetter*, RpSslConnectionInfo*);
};

typedef SHARED_PTR(RpConnectionInfoSetter) RpConnectionInfoSetterSharedPtr;

static inline gboolean
rp_connection_info_setter_is_a(RpConnectionInfoSetterSharedPtr self)
{
    g_return_val_if_fail(self != NULL, false);
    return RP_IS_CONNECTION_INFO_SETTER(self);
}
static inline RpConnectionInfoSetterInterface*
rp_connection_info_setter_iface(RpConnectionInfoSetterSharedPtr self)
{
    g_return_val_if_fail(rp_connection_info_setter_is_a(self), NULL);
    return RP_CONNECTION_INFO_SETTER_GET_IFACE((GObject*)self);
}
static inline void
rp_connection_info_setter_set_local_address(RpConnectionInfoSetterSharedPtr self, RpNetworkAddressInstanceConstSharedPtr local_address)
{
    g_return_if_fail(rp_connection_info_setter_is_a(self));
    RpConnectionInfoSetterInterface* iface = rp_connection_info_setter_iface(self);
    if (iface && iface->set_local_address) \
        iface->set_local_address(self, local_address);
}
static inline void
rp_connection_info_setter_restore_local_address(RpConnectionInfoSetterSharedPtr self, RpNetworkAddressInstanceConstSharedPtr local_address)
{
    g_return_if_fail(rp_connection_info_setter_is_a(self));
    RpConnectionInfoSetterInterface* iface = rp_connection_info_setter_iface(self);
    if (iface && iface->restore_local_address) \
        iface->restore_local_address(self, local_address);
}
static inline void
rp_connection_info_setter_set_remote_address(RpConnectionInfoSetterSharedPtr self, RpNetworkAddressInstanceConstSharedPtr remote_address)
{
    g_return_if_fail(rp_connection_info_setter_is_a(self));
    RpConnectionInfoSetterInterface* iface = rp_connection_info_setter_iface(self);
    if (iface && iface->set_remote_address) \
        iface->set_remote_address(self, remote_address);
}
static inline void
rp_connection_info_setter_set_requested_server_name(RpConnectionInfoSetterSharedPtr self, const char* requested_server_name)
{
    g_return_if_fail(rp_connection_info_setter_is_a(self));
    RpConnectionInfoSetterInterface* iface = rp_connection_info_setter_iface(self);
    if (iface && iface->set_requested_server_name) \
        iface->set_requested_server_name(self, requested_server_name);
}
static inline void rp_connection_info_setter_set_connection_id(RpConnectionInfoSetterSharedPtr self, guint64 id)
{
    g_return_if_fail(rp_connection_info_setter_is_a(self));
    RpConnectionInfoSetterInterface* iface = rp_connection_info_setter_iface(self);
    if (iface && iface->set_connection_id) \
        iface->set_connection_id(self, id);
}
static inline void
rp_connection_info_setter_enable_setting_interface_name(RpConnectionInfoSetterSharedPtr self, bool enable)
{
    g_return_if_fail(rp_connection_info_setter_is_a(self));
    RpConnectionInfoSetterInterface* iface = rp_connection_info_setter_iface(self);
    if (iface && iface->enable_setting_interface_name) \
        iface->enable_setting_interface_name(self, enable);
}
static inline void
rp_connection_info_setter_set_ssl_connection(RpConnectionInfoSetterSharedPtr self, RpSslConnectionInfo* ssl_connection_info)
{
    g_return_if_fail(rp_connection_info_setter_is_a(self));
    RpConnectionInfoSetterInterface* iface = rp_connection_info_setter_iface(self);
    if (iface && iface->set_ssl_connection) \
        iface->set_ssl_connection(self, ssl_connection_info);
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
    RpSysCallIntResult (*connect)(RpSocket*,
                                    RpNetworkAddressInstanceConstSharedPtr);
    int (*sockfd)(RpSocket*);
    //TODO...
};

typedef UNIQUE_PTR(RpSocket) RpSocketPtr;
typedef SHARED_PTR(RpSocket) RpSocketSharedPtr;

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
static inline RpSysCallIntResult
rp_socket_connect(RpSocket* self, RpNetworkAddressInstanceConstSharedPtr address)
{
    return RP_IS_SOCKET(self) ?
        RP_SOCKET_GET_IFACE(self)->connect(self, address) :
        rp_sys_call_int_ctor(-1, EINVAL);
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
