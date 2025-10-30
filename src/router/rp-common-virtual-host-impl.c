/*
 * rp-common-virtual-host-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_common_virtual_host_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_common_virtual_host_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "router/rp-common-virtual-host-impl.h"

typedef struct _RpCommonVirtualHostImpl RpCommonVirtualHostImpl;
struct _RpCommonVirtualHostImpl {
    GObject parent_instance;

    RpVirtualHostCfg* m_virtual_host;
    RpServerFactoryContext* m_factory_context;
    RpStatusCode_e* m_creation_status;

    const char* m_name;
    RpRouteCommonConfig* m_global_route_config;
};

enum
{
    PROP_0, // Reserved.
    PROP_VIRTUAL_HOST,
    PROP_GLOBAL_ROUTE_CONFIG,
    PROP_FACTORY_CONTEXT,
    PROP_CREATION_STATUS,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void virtual_host_iface_init(RpVirtualHostInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpCommonVirtualHostImpl, rp_common_virtual_host_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_VIRTUAL_HOST, virtual_host_iface_init)
)

static const char*
name_i(RpVirtualHost* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_COMMON_VIRTUAL_HOST_IMPL(self)->m_name;
}

static RpRouteCommonConfig*
route_config_i(RpVirtualHost* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_COMMON_VIRTUAL_HOST_IMPL(self)->m_global_route_config;
}

static void
virtual_host_iface_init(RpVirtualHostInterface* iface)
{
    LOGD("(%p)", iface);
    iface->name = name_i;
    iface->route_config = route_config_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    LOGD("(%p, %u, %p, %p)", obj, prop_id, value, pspec);
    switch (prop_id)
    {
        case PROP_VIRTUAL_HOST:
            g_value_set_pointer(value, RP_COMMON_VIRTUAL_HOST_IMPL(obj)->m_virtual_host);
            break;
        case PROP_GLOBAL_ROUTE_CONFIG:
            g_value_set_object(value, RP_COMMON_VIRTUAL_HOST_IMPL(obj)->m_global_route_config);
            break;
        case PROP_FACTORY_CONTEXT:
            g_value_set_object(value, RP_COMMON_VIRTUAL_HOST_IMPL(obj)->m_factory_context);
            break;
        case PROP_CREATION_STATUS:
            g_value_set_pointer(value, RP_COMMON_VIRTUAL_HOST_IMPL(obj)->m_creation_status);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    LOGD("(%p, %u, %p, %p)", obj, prop_id, value, pspec);
    switch (prop_id)
    {
        case PROP_VIRTUAL_HOST:
            RP_COMMON_VIRTUAL_HOST_IMPL(obj)->m_virtual_host = g_value_get_pointer(value);
            break;
        case PROP_GLOBAL_ROUTE_CONFIG:
            RP_COMMON_VIRTUAL_HOST_IMPL(obj)->m_global_route_config = g_value_get_pointer(value);
            break;
        case PROP_FACTORY_CONTEXT:
            RP_COMMON_VIRTUAL_HOST_IMPL(obj)->m_factory_context = g_value_get_object(value);
            break;
        case PROP_CREATION_STATUS:
            RP_COMMON_VIRTUAL_HOST_IMPL(obj)->m_creation_status = g_value_get_pointer(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE GObject*
constructor(GType type, guint n_construct_properties, GObjectConstructParam *construct_properties)
{
    LOGD("(%lu, %u, %p)", type, n_construct_properties, construct_properties);

    GObject* obj = G_OBJECT_CLASS(rp_common_virtual_host_impl_parent_class)->constructor(type, n_construct_properties, construct_properties);
    NOISY_MSG_("obj %p", obj);

    RpCommonVirtualHostImpl* self = RP_COMMON_VIRTUAL_HOST_IMPL(obj);
    RpVirtualHostCfg* virtual_host = self->m_virtual_host;
    self->m_name = virtual_host->name;

    return obj;
}

OVERRIDE void
dispose(GObject* object)
{
    LOGD("(%p)", object);
    G_OBJECT_CLASS(rp_common_virtual_host_impl_parent_class)->dispose(object);
}

static void
rp_common_virtual_host_impl_class_init(RpCommonVirtualHostImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructor = constructor;
    object_class->dispose = dispose;

    obj_properties[PROP_VIRTUAL_HOST] = g_param_spec_pointer("virtual-host",
                                                    "Virtual host",
                                                    "Virtual Host Config",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_GLOBAL_ROUTE_CONFIG] = g_param_spec_object("global-route-config",
                                                    "Global route config",
                                                    "Global Route Config",
                                                    RP_TYPE_ROUTE_COMMON_CONFIG,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_FACTORY_CONTEXT] = g_param_spec_object("factory-context",
                                                    "Factory context",
                                                    "Factory Context",
                                                    RP_TYPE_SERVER_FACTORY_CONTEXT,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_CREATION_STATUS] = g_param_spec_pointer("creation-status",
                                                    "Creation status",
                                                    "Creation Status",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_common_virtual_host_impl_init(RpCommonVirtualHostImpl* self G_GNUC_UNUSED)
{
    LOGD("(%p)", self);
}

static RpCommonVirtualHostImpl*
common_virtual_host_impl_new(RpVirtualHostCfg* virtual_host, RpRouteCommonConfig* global_route_config, RpServerFactoryContext* factory_context, RpStatusCode_e* creation_status)
{
    LOGD("(%p, %p, %p, %p)", virtual_host, global_route_config, factory_context, creation_status);
    return g_object_new(RP_TYPE_COMMON_VIRTUAL_HOST_IMPL,
                        "virtual-host", virtual_host,
                        "global-route-config", global_route_config,
                        "factory-context", factory_context,
                        "creation-status", creation_status,
                        NULL);
}

RpCommonVirtualHostImpl*
rp_common_virtual_host_impl_create(RpVirtualHostCfg* virtual_host, RpRouteCommonConfig* global_route_config, RpServerFactoryContext* factory_context)
{
    LOGD("(%p, %p, %p)", virtual_host, global_route_config, factory_context);
    g_return_val_if_fail(virtual_host != NULL, NULL);
    g_return_val_if_fail(RP_IS_ROUTE_COMMON_CONFIG(global_route_config), NULL);
    g_return_val_if_fail(RP_IS_SERVER_FACTORY_CONTEXT(factory_context), NULL);
    RpStatusCode_e creation_status = RpStatusCode_Ok;
    RpCommonVirtualHostImpl* ret = common_virtual_host_impl_new(virtual_host, global_route_config, factory_context, &creation_status);
    if (creation_status != RpStatusCode_Ok)
    {
        return NULL;
    }
    return ret;
}
