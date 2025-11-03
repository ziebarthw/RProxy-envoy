/*
 * rp-host-description-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_host_description_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_host_description_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "upstream/rp-host-description-impl.h"

typedef struct _RpHostDescriptionImplPrivate RpHostDescriptionImplPrivate;
struct _RpHostDescriptionImplPrivate {

    RpUpstreamTransportSocketFactory* m_socket_factory;

    struct sockaddr* m_address;
    struct sockaddr_storage m_sockaddr_storage;

};

enum
{
    PROP_0, // Reserved.
    PROP_ADDRESS,
    PROP_SOCKET_FACTORY,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpHostDescriptionImpl, rp_host_description_impl, RP_TYPE_HOST_DESCRIPTION_IMPL_BASE,
    G_ADD_PRIVATE(RpHostDescriptionImpl)
)

#define PRIV(obj) \
    ((RpHostDescriptionImplPrivate*) rp_host_description_impl_get_instance_private(RP_HOST_DESCRIPTION_IMPL(obj)))

static struct sockaddr*
address_i(RpHostDescription* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_address;
}

static RpUpstreamTransportSocketFactory*
transport_socket_factory_i(RpHostDescription* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_socket_factory;
}

static void
host_description_iface_init(RpHostDescriptionInterface* iface)
{
    LOGD("(%p)", iface);
    iface->address = address_i;
    iface->transport_socket_factory = transport_socket_factory_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_ADDRESS:
            g_value_set_pointer(value, PRIV(obj)->m_address);
            break;
        case PROP_SOCKET_FACTORY:
            g_value_set_object(value, PRIV(obj)->m_socket_factory);
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
        case PROP_SOCKET_FACTORY:
            PRIV(obj)->m_socket_factory = g_value_get_object(value);
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

    G_OBJECT_CLASS(rp_host_description_impl_parent_class)->constructed(obj);

    RpHostDescriptionImpl* self = RP_HOST_DESCRIPTION_IMPL(obj);
    RpHostDescriptionImplPrivate* me = PRIV(self);
    struct sockaddr* dest_address = rp_host_description_impl_base_dest_address_(RP_HOST_DESCRIPTION_IMPL_BASE(obj));
    if (dest_address->sa_family == AF_INET)
    {
        *((struct sockaddr_in*)&me->m_sockaddr_storage) = *((struct sockaddr_in*)dest_address);
    }
    else
    {
        *((struct sockaddr_in6*)&me->m_sockaddr_storage) = *((struct sockaddr_in6*)dest_address);
    }
    me->m_address = (struct sockaddr*)&me->m_sockaddr_storage;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_host_description_impl_parent_class)->dispose(obj);
}

static void
host_description_impl_base_class_init(RpHostDescriptionImplBaseClass* klass)
{
    LOGD("(%p)", klass);
    host_description_iface_init(g_type_interface_peek(klass, RP_TYPE_HOST_DESCRIPTION));
}

static void
rp_host_description_impl_class_init(RpHostDescriptionImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->constructed = constructed;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    host_description_impl_base_class_init(RP_HOST_DESCRIPTION_IMPL_BASE_CLASS(klass));

    obj_properties[PROP_ADDRESS] = g_param_spec_pointer("address",
                                                    "Address",
                                                    "Destination Socket Address (sockaddr)",
                                                    G_PARAM_READABLE|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_SOCKET_FACTORY] = g_param_spec_object("socket-factory",
                                                    "Socket factory",
                                                    "UpstreamTransportSocketFactory Instance",
                                                    RP_TYPE_UPSTREAM_TRANSPORT_SOCKET_FACTORY,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_host_description_impl_init(RpHostDescriptionImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}
