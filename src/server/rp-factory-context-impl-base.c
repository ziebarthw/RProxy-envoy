/*
 * rp-factory-context-impl-base.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_factory_context_impl_base_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_factory_context_impl_base_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-factory-context.h"
#include "rp-server-instance.h"
#include "server/rp-factory-context-impl-base.h"

typedef struct _RpFactoryContextImplBasePrivate RpFactoryContextImplBasePrivate;
struct _RpFactoryContextImplBasePrivate {
    RpServerInstance* m_server;
};

enum
{
    PROP_0, // Reserved.
    PROP_SERVER,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void factory_context_iface_init(RpFactoryContextInterface* iface);

static void generic_factory_context_iface_init(RpGenericFactoryContextInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpFactoryContextImplBase, rp_factory_context_impl_base, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpFactoryContextImplBase)
    G_IMPLEMENT_INTERFACE(RP_TYPE_GENERIC_FACTORY_CONTEXT, generic_factory_context_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_FACTORY_CONTEXT, factory_context_iface_init)
)

#define PRIV(obj) \
    ((RpFactoryContextImplBasePrivate*) rp_factory_context_impl_base_get_instance_private(RP_FACTORY_CONTEXT_IMPL_BASE(obj)))

static RpServerFactoryContext*
server_factory_context_i(RpGenericFactoryContext* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_server_instance_server_factory_context(PRIV(self)->m_server);
}

static void
generic_factory_context_iface_init(RpGenericFactoryContextInterface* iface)
{
    LOGD("(%p)", iface);
    iface->server_factory_context = server_factory_context_i;
}

static RpTransportSocketFactoryContext*
get_transport_socket_factory_context_i(RpFactoryContext* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_server_instance_transport_socket_factory_context(PRIV(self)->m_server);
}

static void
factory_context_iface_init(RpFactoryContextInterface* iface)
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
            g_value_set_object(value, PRIV(obj)->m_server);
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
            PRIV(obj)->m_server = g_value_get_object(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_factory_context_impl_base_parent_class)->dispose(obj);
}

static void
rp_factory_context_impl_base_class_init(RpFactoryContextImplBaseClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_SERVER] = g_param_spec_object("server",
                                                    "Server",
                                                    "Server Instance",
                                                    RP_TYPE_SERVER_INSTANCE,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_factory_context_impl_base_init(RpFactoryContextImplBase* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}
