/*
 * rp-transport-socket-config-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_transport_socket_config_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_transport_socket_config_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "server/rp-transport-socket-config-impl.h"

struct _RpTransportSocketFactoryContextImpl {
    GObject parent_instance;

    RpServerFactoryContext* m_server_context;
    RpClusterManager* m_cluster_manager;
};

static void transport_socket_factory_context_iface_init(RpTransportSocketFactoryContextInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpTransportSocketFactoryContextImpl, rp_transport_socket_factory_context_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_TRANSPORT_SOCKET_FACTORY_CONTEXT, transport_socket_factory_context_iface_init)
)

static RpServerFactoryContext*
server_factory_context_i(RpTransportSocketFactoryContext* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_TRANSPORT_SOCKET_FACTORY_CONTEXT_IMPL(self)->m_server_context;
}

static RpClusterManager*
cluster_manager_i(RpTransportSocketFactoryContext* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_TRANSPORT_SOCKET_FACTORY_CONTEXT_IMPL(self)->m_cluster_manager;
}

static void
transport_socket_factory_context_iface_init(RpTransportSocketFactoryContextInterface* iface)
{
    LOGD("(%p)", iface);
    iface->server_factory_context = server_factory_context_i;
    iface->cluster_manager = cluster_manager_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_transport_socket_factory_context_impl_parent_class)->dispose(obj);
}

static void
rp_transport_socket_factory_context_impl_class_init(RpTransportSocketFactoryContextImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_transport_socket_factory_context_impl_init(RpTransportSocketFactoryContextImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpTransportSocketFactoryContextImpl*
rp_transport_socket_factory_context_impl_new(RpServerFactoryContext* server_context, RpClusterManager* cm)
{
    LOGD("(%p, %p)", server_context, cm);

    g_return_val_if_fail(RP_IS_SERVER_FACTORY_CONTEXT(server_context), NULL);
    g_return_val_if_fail(RP_IS_CLUSTER_MANAGER(cm), NULL);

    RpTransportSocketFactoryContextImpl* self = g_object_new(RP_TYPE_TRANSPORT_SOCKET_FACTORY_CONTEXT_IMPL, NULL);
    self->m_server_context = server_context;
    self->m_cluster_manager = cm;
    return self;
}
