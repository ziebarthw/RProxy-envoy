/*
 * rp-conn-info-setter-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_connection_info_setter_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_connection_info_setter_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "network/rp-conn-info-setter-impl.h"

#define CONNECTION_INFO_SETTER_IMPL(s) RP_CONNECTION_INFO_SETTER_IMPL((GObject*)s)

struct _RpConnectionInfoSetterImpl {
    GObject parent_instance;

    RpSslConnectionInfo* m_ssl_info;

    RpNetworkAddressInstanceSharedPtr m_local_address;
    RpNetworkAddressInstanceSharedPtr m_direct_local_address;
    RpNetworkAddressInstanceSharedPtr m_remote_address;
    RpNetworkAddressInstanceSharedPtr m_direct_remote_address;

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

static RpNetworkAddressInstanceConstSharedPtr
remote_address_i(RpConnectionInfoProviderConstSharedPtr self)
{
    NOISY_MSG_("(%p)", self);
    return CONNECTION_INFO_SETTER_IMPL(self)->m_remote_address;
}

static RpNetworkAddressInstanceConstSharedPtr
local_address_i(RpConnectionInfoProviderConstSharedPtr self)
{
    NOISY_MSG_("(%p)", self);
RpConnectionInfoSetterImpl* me = CONNECTION_INFO_SETTER_IMPL(self);
NOISY_MSG_("%p, local address %p", self, me->m_local_address);
if (me->m_local_address) NOISY_MSG_("%p, local address %p(%u)", self, me->m_local_address, G_OBJECT(me->m_local_address)->ref_count);
    return CONNECTION_INFO_SETTER_IMPL(self)->m_local_address;
}

static RpSslConnectionInfo*
ssl_connection_i(RpConnectionInfoProviderConstSharedPtr self)
{
    NOISY_MSG_("(%p)", self);
    return CONNECTION_INFO_SETTER_IMPL(self)->m_ssl_info;
}

static const char*
requested_server_name_i(RpConnectionInfoProviderConstSharedPtr self)
{
    NOISY_MSG_("(%p)", self);
    return CONNECTION_INFO_SETTER_IMPL(self)->m_server_name;
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
set_ssl_connection_i(RpConnectionInfoSetterSharedPtr self, RpSslConnectionInfo* ssl_info)
{
    NOISY_MSG_("(%p, %p)", self, ssl_info);
    CONNECTION_INFO_SETTER_IMPL(self)->m_ssl_info = ssl_info;
}

static void
set_local_address_i(RpConnectionInfoSetterSharedPtr self, RpNetworkAddressInstanceConstSharedPtr address)
{
    NOISY_MSG_("(%p, %p)", self, address);
    RpConnectionInfoSetterImpl* me = CONNECTION_INFO_SETTER_IMPL(self);
if (me->m_local_address) NOISY_MSG_("%p, local address %p(%u)", self, me->m_local_address, G_OBJECT(me->m_local_address)->ref_count);
    rp_network_address_instance_set_object(&me->m_local_address, address);
if (me->m_local_address) NOISY_MSG_("%p, local address %p(%u)", self, me->m_local_address, G_OBJECT(me->m_local_address)->ref_count);
}

static void
set_remote_address_i(RpConnectionInfoSetterSharedPtr self, RpNetworkAddressInstanceConstSharedPtr address)
{
    NOISY_MSG_("(%p, %p)", self, address);
    RpConnectionInfoSetterImpl* me = CONNECTION_INFO_SETTER_IMPL(self);
    rp_network_address_instance_set_object(&me->m_remote_address, address);
}

static void
set_requested_server_name_i(RpConnectionInfoSetterSharedPtr self, const char* requested_server_name)
{
    NOISY_MSG_("(%p, %p(%s))", self, requested_server_name, requested_server_name);
    RpConnectionInfoSetterImpl* me = CONNECTION_INFO_SETTER_IMPL(self);
    g_clear_pointer(&me->m_server_name, g_free);
    me->m_server_name = g_ascii_strdown(requested_server_name, -1);
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

    RpConnectionInfoSetterImpl* self = CONNECTION_INFO_SETTER_IMPL(obj);
NOISY_MSG_("%p, clearing server name %p(%s)", self, self->m_server_name, self->m_server_name);
    g_clear_pointer(&self->m_server_name, g_free);
if (self->m_local_address) NOISY_MSG_("%p, local address %p(%u)", self, self->m_local_address, G_OBJECT(self->m_local_address)->ref_count);
    g_clear_object(&self->m_local_address);
if (self->m_remote_address) NOISY_MSG_("%p, remote address %p(%u)", self, self->m_remote_address, G_OBJECT(self->m_remote_address)->ref_count);
    g_clear_object(&self->m_remote_address);

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
rp_connection_info_setter_impl_new(RpNetworkAddressInstanceConstSharedPtr local_address,
                                    RpNetworkAddressInstanceConstSharedPtr remote_address)
{
    LOGD("(%p, %p)", local_address, remote_address);
    RpConnectionInfoSetterImpl* self = g_object_new(RP_TYPE_CONNECTION_INFO_SETTER_IMPL, NULL);
    rp_network_address_instance_set_object(&self->m_local_address, local_address);
    rp_network_address_instance_set_object(&self->m_remote_address, remote_address);
    return constructed(self);
}
