/*
 * rp-socket-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_socket_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_socket_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "rproxy.h"
#include "rp-io-handle.h"
#include "network/rp-socket-impl.h"

typedef struct _RpSocketImplPrivate RpSocketImplPrivate;
struct _RpSocketImplPrivate {
    UNIQUE_PTR(RpIoHandle) m_io_handle;
    struct sockaddr* m_local_address;
    struct sockaddr* m_remote_address;
    RpConnectionInfoSetterImpl* m_connection_info_provider;
};

enum
{
    PROP_0, // Reserved.
    PROP_IO_HANDLE,
    PROP_LOCAL_ADDRESS,
    PROP_REMOTE_ADDRESS,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void socket_iface_init(RpSocketInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpSocketImpl, rp_socket_impl, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpSocketImpl)
    G_IMPLEMENT_INTERFACE(RP_TYPE_SOCKET, socket_iface_init)
)

#define PRIV(obj) \
    ((RpSocketImplPrivate*) rp_socket_impl_get_instance_private(RP_SOCKET_IMPL(obj)))

static RpConnectionInfoSetter*
connection_info_provider_i(RpSocket* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CONNECTION_INFO_SETTER(PRIV(self)->m_connection_info_provider);
}

static void
close_i(RpSocket* self)
{
    NOISY_MSG_("(%p)", self);
    RpSocketImplPrivate* me = PRIV(self);
    if (me->m_io_handle && rp_io_handle_is_open(me->m_io_handle))
    {
        NOISY_MSG_("closing");
        rp_io_handle_close(me->m_io_handle);
    }
}

static bool
is_open_i(RpSocket* self)
{
    NOISY_MSG_("(%p)", self);
    RpSocketImplPrivate* me = PRIV(self);
    return me->m_io_handle && rp_io_handle_is_open(me->m_io_handle);
}

static RpIoHandle*
io_handle_i(RpSocket* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_io_handle;
}

static int
connect_i(RpSocket* self, struct sockaddr* address, const char* requested_server_name)
{
    NOISY_MSG_("(%p, %p, %p(%s))", self, address, requested_server_name, requested_server_name);
    RpSocketImplPrivate* me = PRIV(self);
    int result = rp_io_handle_connect(me->m_io_handle, address, requested_server_name);
    RpConnectionInfoSetter* info_provider = RP_CONNECTION_INFO_SETTER(me->m_connection_info_provider);
    rp_connection_info_setter_set_local_address(info_provider, rp_io_handle_local_address(me->m_io_handle));
    return result;
}

static int
sockfd_i(RpSocket* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_io_handle_sockfd(PRIV(self)->m_io_handle);
}

static void
socket_iface_init(RpSocketInterface* iface)
{
    LOGD("(%p)", iface);
    iface->connection_info_provider = connection_info_provider_i;
    iface->close = close_i;
    iface->is_open = is_open_i;
    iface->io_handle = io_handle_i;
    iface->connect = connect_i;
    iface->sockfd = sockfd_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_IO_HANDLE:
            g_value_set_object(value, PRIV(obj)->m_io_handle);
            break;
        case PROP_LOCAL_ADDRESS:
            g_value_set_pointer(value, PRIV(obj)->m_local_address);
            break;
        case PROP_REMOTE_ADDRESS:
            g_value_set_pointer(value, PRIV(obj)->m_remote_address);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_IO_HANDLE:
            PRIV(obj)->m_io_handle = g_value_get_object(value);
            break;
        case PROP_LOCAL_ADDRESS:
            PRIV(obj)->m_local_address = g_value_get_pointer(value);
            break;
        case PROP_REMOTE_ADDRESS:
            PRIV(obj)->m_remote_address = g_value_get_pointer(value);
NOISY_MSG_("remote address %p", PRIV(obj)->m_remote_address);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_socket_impl_parent_class)->constructed(obj);

    RpSocketImplPrivate* me = PRIV(obj);
    me->m_connection_info_provider = rp_connection_info_setter_impl_new(g_steal_pointer(&me->m_local_address),
                                                                        g_steal_pointer(&me->m_remote_address));
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpSocketImplPrivate* me = PRIV(obj);
    g_clear_object(&me->m_connection_info_provider);
    g_clear_object(&me->m_io_handle);

    G_OBJECT_CLASS(rp_socket_impl_parent_class)->dispose(obj);
}

static void
rp_socket_impl_class_init(RpSocketImplClass* klass)
{
    NOISY_MSG_("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    obj_properties[PROP_IO_HANDLE] = g_param_spec_object("io-handle",
                                                    "IO handle",
                                                    "I/O Handle Instance",
                                                    RP_TYPE_IO_HANDLE,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_LOCAL_ADDRESS] = g_param_spec_pointer("local-address",
                                                    "Local address",
                                                    "Local Socket Address (sockaddr)",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_REMOTE_ADDRESS] = g_param_spec_pointer("remote-address",
                                                    "Remote address",
                                                    "Remote Socket Address (sockaddr)",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_socket_impl_init(RpSocketImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}
