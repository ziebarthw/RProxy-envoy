/*
 * rp-route-matcher.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_route_matcher_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_route_matcher_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "router/rp-virtual-host-impl.h"
#include "router/rp-route-matcher.h"

typedef struct _RpRouteMatcher RpRouteMatcher;
struct _RpRouteMatcher {
    GObject parent_instance;

    RpRouteConfiguration* m_config;
    RpRouteCommonConfig* m_global_route_config;
    RpServerFactoryContext* m_factory_context;
    RpStatusCode_e* m_creation_status;
    RpVirtualHost* m_default_virtual_host;
    GHashTable* m_virtual_hosts_;
    bool m_ignore_port_in_host_matching;
};

enum
{
    PROP_0, // Reserved.
    PROP_CONFIG,
    PROP_GLOBAL_ROUTE_CONFIG,
    PROP_FACTORY_CONTEXT,
    PROP_CREATION_STATUS,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_FINAL_TYPE(RpRouteMatcher, rp_route_matcher, G_TYPE_OBJECT)

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    LOGD("(%p, %u, %p, %p)", obj, prop_id, value, pspec);
    switch (prop_id)
    {
        case PROP_CONFIG:
            g_value_set_pointer(value, RP_ROUTE_MATCHER(obj)->m_config);
            break;
        case PROP_GLOBAL_ROUTE_CONFIG:
            g_value_set_object(value, RP_ROUTE_MATCHER(obj)->m_global_route_config);
            break;
        case PROP_FACTORY_CONTEXT:
            g_value_set_object(value, RP_ROUTE_MATCHER(obj)->m_factory_context);
            break;
        case PROP_CREATION_STATUS:
            g_value_set_pointer(value, RP_ROUTE_MATCHER(obj)->m_creation_status);
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
        case PROP_CONFIG:
            RP_ROUTE_MATCHER(obj)->m_config = g_value_get_pointer(value);
            break;
        case PROP_GLOBAL_ROUTE_CONFIG:
            RP_ROUTE_MATCHER(obj)->m_global_route_config = g_value_get_object(value);
            break;
        case PROP_FACTORY_CONTEXT:
            RP_ROUTE_MATCHER(obj)->m_factory_context = g_value_get_object(value);
            break;
        case PROP_CREATION_STATUS:
            RP_ROUTE_MATCHER(obj)->m_creation_status = g_value_get_pointer(value);
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

    GObject* obj = G_OBJECT_CLASS(rp_route_matcher_parent_class)->constructor(type, n_construct_properties, construct_properties);
    NOISY_MSG_("obj %p", obj);

    RpRouteMatcher* self = RP_ROUTE_MATCHER(obj);
    for (lztq_elem* elem = lztq_first(self->m_config->virtual_hosts); elem; elem = lztq_next(elem))
    {
        RpVirtualHostCfg* virtual_host_config = lztq_elem_data(elem);
        RpVirtuaHostImpl* virtual_host = rp_virtual_host_impl_new(virtual_host_config,
                                                                    self->m_global_route_config,
                                                                    self->m_factory_context,
                                                                    self->m_creation_status);
    }

    return obj;
}

OVERRIDE void
dispose(GObject* object)
{
    LOGD("(%p)", object);
    G_OBJECT_CLASS(rp_route_matcher_parent_class)->dispose(object);
}

static void
rp_route_matcher_class_init(RpRouteMatcherClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructor = constructor;
    object_class->dispose = dispose;

    obj_properties[PROP_CONFIG] = g_param_spec_pointer("config",
                                                    "Config",
                                                    "Config",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_GLOBAL_ROUTE_CONFIG] = g_param_spec_object("global-route-config",
                                                    "Global route config",
                                                    "Global Route Config",
                                                    RP_TYPE_ROUTE_COMMON_CONFIG,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_FACTORY_CONTEXT] = g_param_spec_object("factory-context",
                                                    "Server Factory Context",
                                                    "Server Factory Context",
                                                    RP_TYPE_SERVER_FACTORY_CONTEXT,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_route_matcher_init(RpRouteMatcher* self)
{
    LOGD("(%p)", self);
    self->m_ignore_port_in_host_matching = false;
}

static RpRouteMatcher*
rp_route_matcher_new(RpRouteConfiguration* config, RpRouteCommonConfig* global_route_config,
                        RpServerFactoryContext* factory_context, RpStatusCode_e* creation_status)
{
    LOGD("(%p, %p, %p, %p)", config, global_route_config, factory_context, creation_status);
    return g_object_new(RP_TYPE_ROUTE_MATCHER,
                        "config", config,
                        "global-route-config", global_route_config,
                        "factory-context", factory_context,
                        "creation-status", creation_status,
                        NULL);
}

RpRouteMatcher*
rp_route_matcher_create(RpRouteConfiguration* config, RpRouteCommonConfig* global_route_config, RpServerFactoryContext* factory_context)
{
    LOGD("(%p, %p, %p,)", config, global_route_config, factory_context);
    RpStatusCode_e creation_status;
    RpRouteMatcher* ret = rp_route_matcher_new(config, global_route_config, factory_context, &creation_status);
    if (creation_status != RpStatusCode_Ok)
    {
        return NULL;
    }
    return ret;
}

RpRoute*
rp_route_matcher_route(RpRouteMatcher* self, RpRouteCallback cb, evhtp_headers_t* request_headers, RpStreamInfo* stream_info, guint64 random_value)
{
    LOGD("(%p, %p, %p, %p, %zu)", self, cb, request_headers, stream_info, random_value);
    g_return_val_if_fail(RP_IS_ROUTE_MATCHER(self), NULL);
    
}
