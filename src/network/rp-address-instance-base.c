/*
 * rp-address-instance-base.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_instance_base_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_instance_base_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-socket-interface.h"
#include "network/rp-address-impl.h"

typedef struct _RpNetworkAddressInstanceBasePrivate RpNetworkAddressInstanceBasePrivate;
struct _RpNetworkAddressInstanceBasePrivate {
    GString* m_friendly_name;
    const RpNetworkAddressSocketInterface* m_socket_interface;
    RpAddressType_e m_type;
};

enum
{
    PROP_0, // Reserved.
    PROP_TYPE,
    PROP_SOCK_INTERFACE,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void address_instance_iface_init(RpNetworkAddressInstanceInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpNetworkAddressInstanceBase, rp_network_address_instance_base, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpNetworkAddressInstanceBase)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_ADDRESS_INSTANCE, address_instance_iface_init)
)

#define PRIV(obj) \
    ((RpNetworkAddressInstanceBasePrivate*) rp_network_address_instance_base_get_instance_private(RP_NETWORK_ADDRESS_INSTANCE_BASE(obj)))

static const char*
as_string_i(RpNetworkAddressInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_friendly_name->str;
}

static string_view
as_string_view_i(RpNetworkAddressInstance* self)
{
    NOISY_MSG_("(%p)", self);
    GString* friendly_name_ = PRIV(self)->m_friendly_name;
    return string_view_ctor(friendly_name_->str, friendly_name_->len);
}

static const char*
logical_name_i(RpNetworkAddressInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return as_string_i(self);
}

static RpAddressType_e
type_i(RpNetworkAddressInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_type;
}

static const RpNetworkAddressSocketInterface*
socket_interface_i(RpNetworkAddressInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_socket_interface;
}

static void
address_instance_iface_init(RpNetworkAddressInstanceInterface* iface)
{
    LOGD("(%p)", iface);
    iface->as_string = as_string_i;
    iface->as_string_view = as_string_view_i;
    iface->logical_name = logical_name_i;
    iface->type = type_i;
    iface->socket_interface = socket_interface_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_TYPE:
            g_value_set_int(value, PRIV(obj)->m_type);
            break;
        case PROP_SOCK_INTERFACE:
            g_value_set_object(value, (gpointer)PRIV(obj)->m_socket_interface);
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
        case PROP_TYPE:
            PRIV(obj)->m_type = g_value_get_int(value);
            break;
        case PROP_SOCK_INTERFACE:
            PRIV(obj)->m_socket_interface = g_value_get_object(value);
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

    RpNetworkAddressInstanceBasePrivate* me = PRIV(obj);
    g_string_free(g_steal_pointer(&me->m_friendly_name), true);

    G_OBJECT_CLASS(rp_network_address_instance_base_parent_class)->dispose(obj);
}

static void
rp_network_address_instance_base_class_init(RpNetworkAddressInstanceBaseClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_SOCK_INTERFACE] = g_param_spec_object("sock-interface",
                                                    "Socket interface",
                                                    "Socket interface",
                                                    RP_TYPE_NETWORK_ADDRESS_SOCKET_INTERFACE,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_TYPE] = g_param_spec_int("type",
                                                    "Address type",
                                                    "Address type",
                                                    RpAddressType_IP,
                                                    RpAddressType_RPROXY_INTERNAL,
                                                    RpAddressType_IP,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_network_address_instance_base_init(RpNetworkAddressInstanceBase* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

UNIQUE_PTR(GString)*
rp_network_address_instance_base_friendly_name_(RpNetworkAddressInstanceBase* self)
{
    LOGD("(%p)", self);
    return &PRIV(self)->m_friendly_name;
}
