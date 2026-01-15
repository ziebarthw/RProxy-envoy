/*
 * rp-server-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_server_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_server_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-factory-context.h"
#include "rp-transport-socket-config.h"
#include "server/rp-server-impl.h"

#define SERVER(s) RP_SERVER_FACTORY_CONTEXT_IMPL(s)->m_server

struct _RpServerFactoryContextImpl {
    GObject parent_instance;

    RpServerInstance* m_server;
};

static void common_factory_iface_init(RpCommonFactoryContextInterface* iface);
static void server_factory_context_iface_init(RpServerFactoryContextInterface* iface);
static void transport_socket_factory_context_iface_init(RpTransportSocketFactoryContextInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpServerFactoryContextImpl, rp_server_factory_context_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_COMMON_FACTORY_CONTEXT, common_factory_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_SERVER_FACTORY_CONTEXT, server_factory_context_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_TRANSPORT_SOCKET_FACTORY_CONTEXT, transport_socket_factory_context_iface_init)
)

static RpServerFactoryContext*
server_factory_context_i(RpTransportSocketFactoryContext* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_SERVER_FACTORY_CONTEXT(self);
}

static void
transport_socket_factory_context_iface_init(RpTransportSocketFactoryContextInterface* iface)
{
    LOGD("(%p)", iface);
    iface->server_factory_context = server_factory_context_i;
}

static RpClusterManager*
cluster_manager_i(RpCommonFactoryContext* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_server_instance_cluster_manager(SERVER(self));
}

static RpLocalInfo*
local_info_i(RpCommonFactoryContext* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_server_instance_local_info(SERVER(self));
}

static RpDispatcher*
main_thread_dispatcher_i(RpCommonFactoryContext* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_server_instance_dispatcher(SERVER(self));
}

static RpSlotAllocator*
thread_local_i(RpCommonFactoryContext* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_SLOT_ALLOCATOR(rp_server_instance_thread_local(SERVER(self)));
}

static RpSingletonManager*
singleton_manager_i(RpCommonFactoryContext* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_server_instance_singleton_manager(SERVER(self));
}

static void
common_factory_iface_init(RpCommonFactoryContextInterface* iface)
{
    LOGD("(%p)", iface);
    iface->cluster_manager = cluster_manager_i;
    iface->local_info = local_info_i;
    iface->main_thread_dispatcher = main_thread_dispatcher_i;
    iface->thread_local = thread_local_i;
    iface->singleton_manager = singleton_manager_i;
}

static RpTransportSocketFactoryContext*
get_transport_socket_factory_context_i(RpServerFactoryContext* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_server_instance_transport_socket_factory_context(SERVER(self));
}

static rproxy_t*
bootstrap_i(RpServerFactoryContext* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_server_instance_bootstrap(SERVER(self));
}

static GMutex*
lock_i(RpServerFactoryContext* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_server_instance_lock(SERVER(self));
}

static void
server_factory_context_iface_init(RpServerFactoryContextInterface* iface)
{
    LOGD("(%p)", iface);
    iface->get_transport_socket_factory_context = get_transport_socket_factory_context_i;
    iface->bootstrap = bootstrap_i;
    iface->lock = lock_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_server_factory_context_impl_parent_class)->dispose(obj);
}

static void
rp_server_factory_context_impl_class_init(RpServerFactoryContextImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_server_factory_context_impl_init(RpServerFactoryContextImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpServerFactoryContextImpl*
rp_server_factory_context_impl_new(RpServerInstance* server)
{
    LOGD("(%p)", server);
    g_return_val_if_fail(RP_IS_SERVER_INSTANCE(server), NULL);
    RpServerFactoryContextImpl* self = g_object_new(RP_TYPE_SERVER_FACTORY_CONTEXT_IMPL, NULL);
    self->m_server = server;
    return self;
}
