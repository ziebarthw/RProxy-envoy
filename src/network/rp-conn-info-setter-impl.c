/*
 * rp-conn-info-setter-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_connection_info_setter_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_connection_info_setter_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "network/rp-conn-info-setter-impl.h"

struct _RpConnectionInfoSetterImpl {
    GObject parent_instance;

    RpSslConnectionInfo* m_ssl_info;

    struct sockaddr_storage m_local_address;
    struct sockaddr_storage m_direct_local_address;
    struct sockaddr_storage m_remote_address;
    struct sockaddr_storage m_direct_remote_address;

    char* m_server_name;
    char* m_interface_name;

    guint64 m_connection_id;

    bool m_local_address_restored;
};

static void connection_info_provider_iface_init(RpConnectionInfoProviderInterface* iface);
static void connection_info_setter_iface_init(RpConnectionInfoSetterInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpConnectionInfoSetterImpl, rp_connection_info_setter_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_CONNECTION_INFO_PROVIDER, connection_info_provider_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_CONNECTION_INFO_SETTER, connection_info_setter_iface_init)
)

static inline void
set_sockaddr_storage(struct sockaddr_storage* dest, struct sockaddr* src)
{
    NOISY_MSG_("(%p, %p)", dest, src);
    if (!src) //REVISIT...
    {
        NOISY_MSG_("null");
        struct sockaddr_in* dest_ = (struct sockaddr_in*)dest;
        memset(dest_, 0, sizeof(*dest_));
        dest_->sin_family = AF_UNSPEC;
    }
    else if (src->sa_family == AF_INET)
    {
        NOISY_MSG_("IPv4");
        *((struct sockaddr_in*)dest) = *((struct sockaddr_in*)src);
    }
    else
    {
        NOISY_MSG_("IPv6");
        *((struct sockaddr_in6*)dest) = *((struct sockaddr_in6*)src);
    }
}

static struct sockaddr*
remote_address_i(RpConnectionInfoProvider* self)
{
    NOISY_MSG_("(%p)", self);
    return (struct sockaddr*)&RP_CONNECTION_INFO_SETTER_IMPL(self)->m_remote_address;
}

static struct sockaddr*
local_address_i(RpConnectionInfoProvider* self)
{
    NOISY_MSG_("(%p)", self);
    return (struct sockaddr*)&RP_CONNECTION_INFO_SETTER_IMPL(self)->m_local_address;
}

static RpSslConnectionInfo*
ssl_connection_i(RpConnectionInfoProvider* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CONNECTION_INFO_SETTER_IMPL(self)->m_ssl_info;
}

static const char*
requested_server_name_i(RpConnectionInfoProvider* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CONNECTION_INFO_SETTER_IMPL(self)->m_server_name;
}

static void
connection_info_provider_iface_init(RpConnectionInfoProviderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->remote_address = remote_address_i;
    iface->local_address = local_address_i;
    iface->ssl_connection = ssl_connection_i;
    iface->requested_server_name = requested_server_name_i;
}

static void
set_ssl_connection_i(RpConnectionInfoSetter* self, RpSslConnectionInfo* ssl_info)
{
    NOISY_MSG_("(%p, %p)", self, ssl_info);
    RP_CONNECTION_INFO_SETTER_IMPL(self)->m_ssl_info = ssl_info;
}

static void
set_local_address_i(RpConnectionInfoSetter* self, struct sockaddr* address)
{
    NOISY_MSG_("(%p, %p)", self, address);
    set_sockaddr_storage(&RP_CONNECTION_INFO_SETTER_IMPL(self)->m_local_address, address);
}

static void
set_remote_address_i(RpConnectionInfoSetter* self, struct sockaddr* address)
{
    NOISY_MSG_("(%p, %p)", self, address);
    set_sockaddr_storage(&RP_CONNECTION_INFO_SETTER_IMPL(self)->m_remote_address, address);
}

static void
set_requested_server_name_i(RpConnectionInfoSetter* self, const char* requested_server_name)
{
    NOISY_MSG_("(%p, %p(%s))", self, requested_server_name, requested_server_name);
    RpConnectionInfoSetterImpl* me = RP_CONNECTION_INFO_SETTER_IMPL(self);
    g_clear_pointer(&me->m_server_name, g_free);
    me->m_server_name = g_ascii_strdown(g_strdup(requested_server_name), -1);
}

static void
connection_info_setter_iface_init(RpConnectionInfoSetterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->set_ssl_connection = set_ssl_connection_i;
    iface->set_local_address = set_local_address_i;
    iface->set_remote_address = set_remote_address_i;
    iface->set_requested_server_name = set_requested_server_name_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpConnectionInfoSetterImpl* self = RP_CONNECTION_INFO_SETTER_IMPL(obj);
    g_clear_pointer(&self->m_server_name, g_free);

    G_OBJECT_CLASS(rp_connection_info_setter_impl_parent_class)->dispose(obj);
}

static void
rp_connection_info_setter_impl_class_init(RpConnectionInfoSetterImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_connection_info_setter_impl_init(RpConnectionInfoSetterImpl* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_local_address_restored = false;
}

static inline RpConnectionInfoSetterImpl*
constructed(RpConnectionInfoSetterImpl* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_direct_local_address = self->m_local_address;
    self->m_direct_remote_address = self->m_remote_address;
    return self;
}

RpConnectionInfoSetterImpl*
rp_connection_info_setter_impl_new(struct sockaddr* local_address, struct sockaddr* remote_address)
{
    LOGD("(%p, %p)", local_address, remote_address);
    RpConnectionInfoSetterImpl* self = g_object_new(RP_TYPE_CONNECTION_INFO_SETTER_IMPL, NULL);
    set_sockaddr_storage(&self->m_local_address, local_address);
    set_sockaddr_storage(&self->m_remote_address, remote_address);
    return constructed(self);
}
