/*
 * rp-route-common-config-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_route_common_config_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_route_common_config_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-rds.h"
#include "router/rp-route-common-config-impl.h"

typedef struct _RpRouteCommonConfigImpl RpRouteCommonConfigImpl;
struct _RpRouteCommonConfigImpl {
    GObject parent_instance;

    RpRouteConfiguration* m_config;
    RpServerFactoryContext* m_factory_context;
    RpStatusCode_e* m_creation_status;

    char* m_name;

    guint32 m_max_direct_reponse_body_size_bytes;

    bool m_ignore_path_parameters_in_path_matching : 1;
};

enum
{
    PROP_0, // Reserved.
    PROP_CONFIG,
    PROP_FACTORY_CONTEXT,
    PROP_CREATION_STATUS,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void route_common_config_iface_init(RpRouteCommonConfigInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpRouteCommonConfigImpl, rp_route_common_config_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_ROUTE_COMMON_CONFIG, route_common_config_iface_init)
)

static guint32
max_direct_response_body_size_bytes_i(RpRouteCommonConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_ROUTE_COMMON_CONFIG_IMPL(self)->m_max_direct_reponse_body_size_bytes;
}

static const char*
name_i(RpRouteCommonConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_ROUTE_COMMON_CONFIG_IMPL(self)->m_name;
}

static void
route_common_config_iface_init(RpRouteCommonConfigInterface* iface)
{
    LOGD("(%p)", iface);
    iface->max_direct_response_body_size_bytes = max_direct_response_body_size_bytes_i;
    iface->name = name_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_CONFIG:
            g_value_set_pointer(value, RP_ROUTE_COMMON_CONFIG_IMPL(obj)->m_config);
            break;
        case PROP_FACTORY_CONTEXT:
            g_value_set_object(value, RP_ROUTE_COMMON_CONFIG_IMPL(obj)->m_factory_context);
            break;
        case PROP_CREATION_STATUS:
            g_value_set_pointer(value, RP_ROUTE_COMMON_CONFIG_IMPL(obj)->m_creation_status);
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
        case PROP_CONFIG:
            RP_ROUTE_COMMON_CONFIG_IMPL(obj)->m_config = g_value_get_pointer(value);
            break;
        case PROP_FACTORY_CONTEXT:
            RP_ROUTE_COMMON_CONFIG_IMPL(obj)->m_factory_context = g_value_get_object(value);
            break;
        case PROP_CREATION_STATUS:
            RP_ROUTE_COMMON_CONFIG_IMPL(obj)->m_creation_status = g_value_get_pointer(value);
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

    G_OBJECT_CLASS(rp_route_common_config_impl_parent_class)->constructed(obj);

    RpRouteCommonConfigImpl* self = RP_ROUTE_COMMON_CONFIG_IMPL(obj);
    RpRouteConfiguration* config = self->m_config;
    self->m_name = g_strdup(config->name);
    self->m_max_direct_reponse_body_size_bytes = config->max_direct_response_body_size_bytes;
    self->m_ignore_path_parameters_in_path_matching = config->ignore_path_paramaters_in_path_matching;

    self->m_config = NULL;
}

OVERRIDE void
dispose(GObject* object)
{
    NOISY_MSG_("(%p)", object);

    RpRouteCommonConfigImpl* self = RP_ROUTE_COMMON_CONFIG_IMPL(object);
    g_clear_pointer(&self->m_name, g_free);

    G_OBJECT_CLASS(rp_route_common_config_impl_parent_class)->dispose(object);
}

static void
rp_route_common_config_impl_class_init(RpRouteCommonConfigImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    obj_properties[PROP_CONFIG] = g_param_spec_pointer("config",
                                                    "Config",
                                                    "RouteConfiguration",
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
rp_route_common_config_impl_init(RpRouteCommonConfigImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static RpRouteCommonConfigImpl*
route_common_config_impl_new(RpRouteConfiguration* config, RpServerFactoryContext* factory_context, RpStatusCode_e* creation_status)
{
    NOISY_MSG_("(%p, %p, %p)", config, factory_context, creation_status);
    return g_object_new(RP_TYPE_ROUTE_COMMON_CONFIG_IMPL,
                        "config", config,
                        "factory-context", factory_context,
                        "creation-status", creation_status,
                        NULL);
}

RpRouteCommonConfigImpl*
rp_route_common_config_impl_create(RpRouteConfiguration* config, RpServerFactoryContext* factory_context)
{
    LOGD("(%p, %p)", config, factory_context);
    g_return_val_if_fail(config != NULL, NULL);
    g_return_val_if_fail(RP_IS_SERVER_FACTORY_CONTEXT(factory_context), NULL);
    RpStatusCode_e creation_status = RpStatusCode_Ok;
    RpRouteCommonConfigImpl* ret = route_common_config_impl_new(config, factory_context, &creation_status);
    return creation_status == RpStatusCode_Ok ? ret : NULL;
}

const char*
rp_route_common_config_impl_name(RpRouteCommonConfigImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_ROUTE_COMMON_CONFIG_IMPL(self), NULL);
    return self->m_name;
}

guint32
rp_route_common_config_impl_max_direct_response_body_size_bytes(RpRouteCommonConfigImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_ROUTE_COMMON_CONFIG_IMPL(self), 0);
    return self->m_max_direct_reponse_body_size_bytes;
}
