/*
 * rp-server-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_server_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_server_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-factory-context.h"
#include "rp-transport-socket-config.h"
#include "server/rp-server-impl.h"

struct _RpServerFactoryContextImpl {
    GObject parent_instance;

    RpInstance* m_server;
};

enum
{
    PROP_0, // Reserved.
    PROP_SERVER,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

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
    return rp_instance_cluster_manager(RP_INSTANCE(RP_SERVER_FACTORY_CONTEXT_IMPL(self)->m_server));
}

static RpLocalInfo*
local_info_i(RpCommonFactoryContext* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_instance_local_info(RP_INSTANCE(RP_SERVER_FACTORY_CONTEXT_IMPL(self)->m_server));
}

static void
common_factory_iface_init(RpCommonFactoryContextInterface* iface)
{
    LOGD("(%p)", iface);
    iface->cluster_manager = cluster_manager_i;
    iface->local_info = local_info_i;
}

static RpTransportSocketFactoryContext*
get_transport_socket_factory_context_i(RpServerFactoryContext* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_instance_transport_socket_factory_context(RP_INSTANCE(RP_SERVER_FACTORY_CONTEXT_IMPL(self)->m_server));
}

static void
server_factory_context_iface_init(RpServerFactoryContextInterface* iface)
{
    LOGD("(%p)", iface);
    iface->get_transport_socket_factory_context = get_transport_socket_factory_context_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_SERVER:
            g_value_set_object(value, RP_SERVER_FACTORY_CONTEXT_IMPL(obj)->m_server);
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
        case PROP_SERVER:
            RP_SERVER_FACTORY_CONTEXT_IMPL(obj)->m_server = g_value_get_object(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
dispose(GObject* object)
{
    NOISY_MSG_("(%p)", object);
    G_OBJECT_CLASS(rp_server_factory_context_impl_parent_class)->dispose(object);
}

static void
rp_server_factory_context_impl_class_init(RpServerFactoryContextImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_SERVER] = g_param_spec_object("server",
                                                    "Server",
                                                    "Server Instance",
                                                    RP_TYPE_INSTANCE,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_server_factory_context_impl_init(RpServerFactoryContextImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpServerFactoryContextImpl*
rp_server_factory_context_impl_new(RpInstance* server)
{
    LOGD("(%p)", server);
    g_return_val_if_fail(RP_IS_INSTANCE(server), NULL);
    return g_object_new(RP_TYPE_SERVER_FACTORY_CONTEXT_IMPL,
                        "server", server,
                        NULL);
}
