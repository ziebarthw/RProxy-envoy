/*
 * rp-connection-socket-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_connection_socket_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_connection_socket_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include "network/rp-connection-socket-impl.h"

typedef struct _RpConnectionSocketImplPrivate RpConnectionSocketImplPrivate;
struct _RpConnectionSocketImplPrivate {
    char* m_transport_protocol;
    GArray* m_application_protocols;
};

static void connection_socket_iface_init(RpConnectionSocketInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpConnectionSocketImpl, rp_connection_socket_impl, RP_TYPE_SOCKET_IMPL,
    G_ADD_PRIVATE(RpConnectionSocketImpl)
    G_IMPLEMENT_INTERFACE(RP_TYPE_CONNECTION_SOCKET, connection_socket_iface_init)
)

#define PRIV(obj) \
    ((RpConnectionSocketImplPrivate*) rp_connection_socket_impl_get_instance_private(RP_CONNECTION_SOCKET_IMPL(obj)))

static void
set_detected_transport_protocol_i(RpConnectionSocket* self, const char* protocol)
{
    NOISY_MSG_("(%p, %p(%s))", self, protocol, protocol);
    RpConnectionSocketImplPrivate* me = PRIV(self);
    g_clear_pointer(&me->m_transport_protocol, g_free);
    me->m_transport_protocol = g_strdup(protocol);
}

static inline GArray*
ensure_application_protocols(RpConnectionSocketImplPrivate* me, guint length)
{
    NOISY_MSG_("(%p, %u)", me, length);
    if (me->m_application_protocols)
    {
        NOISY_MSG_("pre-allocated application protocols %p", me->m_application_protocols);
        me->m_application_protocols = g_array_set_size(me->m_application_protocols, 0);
        return me->m_application_protocols;
    }
    me->m_application_protocols = g_array_sized_new(false, false, sizeof(char*), length);
    NOISY_MSG_("allocated application protocols %p", me->m_application_protocols);
    return me->m_application_protocols;
}

static void
set_requested_application_protocols_i(RpConnectionSocket* self, GArray* protocols)
{
    NOISY_MSG_("(%p, %p)", self, protocols);
    GArray* dest = ensure_application_protocols(PRIV(self), protocols->len);
    for (guint i = 0; i < protocols->len; ++i)
    {
        char* entry = g_array_index(protocols, char*, i);
        g_array_append_val(dest, entry);
    }
}

static GArray*
requested_application_protocols_i(RpConnectionSocket* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_application_protocols;
}

static void
set_requested_server_name_i(RpConnectionSocket* self, const char* server_name)
{
    NOISY_MSG_("(%p, %p(%s))", self, server_name, server_name);
    rp_connection_info_setter_set_requested_server_name(
        rp_socket_connection_info_provider(RP_SOCKET(self)), server_name);
}

static const char*
requested_server_name_i(RpConnectionSocket* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_connection_info_provider_requested_server_name(RP_CONNECTION_INFO_PROVIDER(
        rp_socket_connection_info_provider(RP_SOCKET(self))
    ));
}

static void
connection_socket_iface_init(RpConnectionSocketInterface* iface)
{
    LOGD("(%p)", iface);
    iface->set_detected_transport_protocol = set_detected_transport_protocol_i;
    iface->set_requested_application_protocols = set_requested_application_protocols_i;
    iface->requested_application_protocols = requested_application_protocols_i;
    iface->set_requested_server_name = set_requested_server_name_i;
    iface->requested_server_name = requested_server_name_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    LOGD("(%p)", obj);

    RpConnectionSocketImplPrivate* me = PRIV(obj);
    g_clear_pointer(&me->m_transport_protocol, g_free);
    if (me->m_application_protocols)
    {
        g_array_free(me->m_application_protocols, true);
        me->m_application_protocols = NULL;
    }

    G_OBJECT_CLASS(rp_connection_socket_impl_parent_class)->dispose(obj);
}

static void
rp_connection_socket_impl_class_init(RpConnectionSocketImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_connection_socket_impl_init(RpConnectionSocketImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}
