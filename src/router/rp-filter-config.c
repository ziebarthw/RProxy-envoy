/*
 * rp-filter-config.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_filter_config_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_filter_config_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "router/rp-router-filter.h"
#include "router/rp-filter-config.h"

struct _RpRouterFilterConfig {
    GObjectClass parent_instance;

    RpRouterCfg* m_config;
    RpCommonFactoryContext* m_factory_context;

    bool m_suppress_envoy_headers;
};

enum
{
    PROP_0, // Reserved.
    PROP_PROTO_CONFIG,
    PROP_FACTORY_CONTEXT,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void filter_chain_factory_iface_init(RpFilterChainFactoryInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpRouterFilterConfig, rp_router_filter_config, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_FILTER_CHAIN_FACTORY, filter_chain_factory_iface_init)
)

static bool
create_filter_chain_i(RpFilterChainFactory* self, RpFilterChainManager* manager)
{
    NOISY_MSG_("(%p, %p)", self, manager);
//TODO...
//if (upstream_http_filter_factories_.empty())...
    return false;
}

static void
filter_chain_factory_iface_init(RpFilterChainFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_filter_chain = create_filter_chain_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_PROTO_CONFIG:
            g_value_set_pointer(value, RP_ROUTER_FILTER_CONFIG(obj)->m_config);
            break;
        case PROP_FACTORY_CONTEXT:
            g_value_set_object(value, RP_ROUTER_FILTER_CONFIG(obj)->m_factory_context);
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
        case PROP_PROTO_CONFIG:
            RP_ROUTER_FILTER_CONFIG(obj)->m_config = g_value_get_pointer(value);
            break;
        case PROP_FACTORY_CONTEXT:
            RP_ROUTER_FILTER_CONFIG(obj)->m_factory_context = g_value_get_object(value);
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

    G_OBJECT_CLASS(rp_router_filter_config_parent_class)->constructed(obj);

    RpRouterFilterConfig* self = RP_ROUTER_FILTER_CONFIG(obj);
    RpRouterCfg* config = self->m_config;
    self->m_suppress_envoy_headers = config->suppress_rproxy_headers;
    self->m_config = NULL;
}

OVERRIDE void
dispose(GObject* object)
{
    NOISY_MSG_("(%p)", object);
    G_OBJECT_CLASS(rp_router_filter_config_parent_class)->dispose(object);
}

static void
rp_router_filter_config_class_init(RpRouterFilterConfigClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    obj_properties[PROP_PROTO_CONFIG] = g_param_spec_pointer("proto-config",
                                                    "Proto config",
                                                    "RouterCfg Proto Config",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_FACTORY_CONTEXT] = g_param_spec_object("factory-context",
                                                    "Factory context",
                                                    "Factory Context Instance",
                                                    RP_TYPE_FACTORY_CONTEXT,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_router_filter_config_init(RpRouterFilterConfig* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpRouterFilterConfig*
rp_router_filter_config_new(RpRouterCfg* proto_config, RpFactoryContext* factory_context)
{
    LOGD("(%p, %p)", proto_config, factory_context);
    g_return_val_if_fail(proto_config != NULL, NULL);
    g_return_val_if_fail(RP_IS_FACTORY_CONTEXT(factory_context), NULL);
    return g_object_new(RP_TYPE_ROUTER_FILTER_CONFIG,
                        "proto-config", proto_config,
                        "factory-context", factory_context,
                        NULL);
}
