/*
 * rp-host-description-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_host_description_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_host_description_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "upstream/rp-upstream-impl.h"

#define PARENT_HOST_DESCRIPTION_IFACE(s) \
    ((RpHostDescriptionInterface*)g_type_interface_peek_parent(RP_HOST_DESCRIPTION_GET_IFACE(s)))

typedef struct _RpHostDescriptionImplPrivate RpHostDescriptionImplPrivate;
struct _RpHostDescriptionImplPrivate {

    RpNetworkAddressInstanceConstSharedPtr m_address;

    //TODO...struct sockaddr* m_health_check_address;
};

enum
{
    PROP_0, // Reserved.
    PROP_ADDRESS,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void host_description_iface_init(RpHostDescriptionInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpHostDescriptionImpl, rp_host_description_impl, RP_TYPE_HOST_DESCRIPTION_IMPL_BASE,
    G_ADD_PRIVATE(RpHostDescriptionImpl)
    G_IMPLEMENT_INTERFACE(RP_TYPE_HOST_DESCRIPTION, host_description_iface_init)
)

#define PRIV(obj) \
    ((RpHostDescriptionImplPrivate*) rp_host_description_impl_get_instance_private(RP_HOST_DESCRIPTION_IMPL(obj)))

static RpNetworkAddressInstanceConstSharedPtr
address_i(RpHostDescription* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_address;
}

static bool
can_create_connection_i(RpHostDescription* self, RpResourcePriority_e priority)
{
    NOISY_MSG_("(%p, %d)", self, priority);
    return PARENT_HOST_DESCRIPTION_IFACE(self)->can_create_connection(self, priority);
}

static RpClusterInfoConstSharedPtr
cluster_i(RpHostDescription* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_HOST_DESCRIPTION_IFACE(self)->cluster(self);
}

static const char*
hostname_i(RpHostDescription* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_HOST_DESCRIPTION_IFACE(self)->hostname(self);
}

static RpMetadataConstSharedPtr
metadata_i(RpHostDescription* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_HOST_DESCRIPTION_IFACE(self)->metadata(self);
}

static guint32
priority_i(RpHostDescription* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_HOST_DESCRIPTION_IFACE(self)->priority(self);
}

static void
set_metadata_i(RpHostDescription* self, RpMetadataConstSharedPtr new_metadata)
{
    NOISY_MSG_("(%p, %p)", self, new_metadata);
    PARENT_HOST_DESCRIPTION_IFACE(self)->set_metadata(self, new_metadata);
}

static void
set_priority_i(RpHostDescription* self, guint32 priority)
{
    NOISY_MSG_("(%p, %d)", self, priority);
    PARENT_HOST_DESCRIPTION_IFACE(self)->set_priority(self, priority);
}

static RpUpstreamTransportSocketFactory*
transport_socket_factory_i(RpHostDescription* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_HOST_DESCRIPTION_IFACE(self)->transport_socket_factory(self);
}

static void
host_description_iface_init(RpHostDescriptionInterface* iface)
{
    LOGD("(%p)", iface);
    iface->address = address_i;
    iface->can_create_connection = can_create_connection_i;
    iface->cluster = cluster_i;
    iface->hostname = hostname_i;
    iface->metadata = metadata_i;
    iface->priority = priority_i;
    iface->set_metadata = set_metadata_i;
    iface->set_priority = set_priority_i;
    iface->transport_socket_factory = transport_socket_factory_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_ADDRESS:
            g_value_set_object(value, PRIV(obj)->m_address);
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
        case PROP_ADDRESS:
            PRIV(obj)->m_address = g_object_ref(g_value_get_object(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

#if 0
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
#endif//0

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpHostDescriptionImplPrivate* me = PRIV(obj);
    g_clear_object(&me->m_address);
    
    G_OBJECT_CLASS(rp_host_description_impl_parent_class)->dispose(obj);
}

static void
rp_host_description_impl_class_init(RpHostDescriptionImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_ADDRESS] = g_param_spec_object("address",
                                                        "Address",
                                                        "Destination Socket Address (sockaddr)",
                                                        RP_TYPE_NETWORK_ADDRESS_INSTANCE,
                                                        G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_host_description_impl_init(RpHostDescriptionImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}
