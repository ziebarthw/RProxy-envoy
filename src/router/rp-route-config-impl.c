/*
 * rp-route-config-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_route_config_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_route_config_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rproxy.h"
#include "rp-headers.h"
#include "event/rp-dispatcher-impl.h"
#include "router/rp-route-common-config-impl.h"
#include "router/rp-route-impl.h"
#include "router/rp-route-config-impl.h"

typedef struct _RpRouteConfigImpl RpRouteConfigImpl;
struct _RpRouteConfigImpl {
    GObject parent_instance;

    RpRouteConfiguration* m_config;
    RpServerFactoryContext* m_factory_context;
    RpStatusCode_e* m_creation_status;

    RpDispatcher* m_dispatcher;

    RpRouteCommonConfigImpl* m_shared_config;
};

enum
{
    PROP_0, // Reserved.
    PROP_CONFIG,
    PROP_FACTORY_CONTEXT,
    PROP_CREATION_STATUS,
    PROP_DISPATCHER,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void route_common_config_iface_init(RpRouteCommonConfigInterface* iface);
static void route_config_iface_init(RpRouteConfigInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpRouteConfigImpl, rp_route_config_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_ROUTE_COMMON_CONFIG, route_common_config_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_ROUTE_CONFIG, route_config_iface_init)
)

static const char*
name_i(RpRouteCommonConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_route_common_config_impl_name(RP_ROUTE_CONFIG_IMPL(self)->m_shared_config);
}

static guint32
max_direct_response_body_size_bytes_i(RpRouteCommonConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_route_common_config_max_direct_response_body_size_bytes(RP_ROUTE_COMMON_CONFIG(RP_ROUTE_CONFIG_IMPL(self)->m_shared_config));
}

static void
route_common_config_iface_init(RpRouteCommonConfigInterface* iface)
{
    LOGD("(%p)", iface);
    iface->name = name_i;
    iface->max_direct_response_body_size_bytes = max_direct_response_body_size_bytes_i;
}

static RpRoute*
route_i(RpRouteConfig* self, RpRouteCallback cb, evhtp_headers_t* request_headers, RpStreamInfo* stream_info, guint64 random_value)
{
    NOISY_MSG_("(%p, %p, %p, %p, %zu)", self, cb, request_headers, stream_info, random_value);

    RpRouteConfigImpl* me = RP_ROUTE_CONFIG_IMPL(self);
    const char* hostname = evhtp_header_find(request_headers, RpHeaderValues.HostLegacy);
    const char* path = evhtp_header_find(request_headers, RpHeaderValues.Path);
    evthr_t* thr = rp_dispatcher_thr(RP_DISPATCHER_IMPL(me->m_dispatcher));
    rproxy_t* rproxy = evthr_get_aux(thr);
    // |path| may be nullptr if it is a CONNECT request.
    rule_cfg_t* rule_cfg = evhtp_path_match(rproxy->htp, hostname, path ? path : "/");
    return rule_cfg ? RP_ROUTE(rp_route_impl_new(self, rule_cfg)) : NULL;
}

static void
route_config_iface_init(RpRouteConfigInterface* iface)
{
    LOGD("(%p)", iface);
    iface->route = route_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_CONFIG:
            g_value_set_pointer(value, RP_ROUTE_CONFIG_IMPL(obj)->m_config);
            break;
        case PROP_FACTORY_CONTEXT:
            g_value_set_object(value, RP_ROUTE_CONFIG_IMPL(obj)->m_factory_context);
            break;
        case PROP_CREATION_STATUS:
            g_value_set_pointer(value, RP_ROUTE_CONFIG_IMPL(obj)->m_creation_status);
            break;
        case PROP_DISPATCHER:
            g_value_set_pointer(value, RP_ROUTE_CONFIG_IMPL(obj)->m_dispatcher);
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
            RP_ROUTE_CONFIG_IMPL(obj)->m_config = g_value_get_pointer(value);
            break;
        case PROP_FACTORY_CONTEXT:
            RP_ROUTE_CONFIG_IMPL(obj)->m_factory_context = g_value_get_object(value);
            break;
        case PROP_CREATION_STATUS:
            RP_ROUTE_CONFIG_IMPL(obj)->m_creation_status = g_value_get_pointer(value);
            break;
        case PROP_DISPATCHER:
            RP_ROUTE_CONFIG_IMPL(obj)->m_dispatcher = g_value_get_pointer(value);
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

    G_OBJECT_CLASS(rp_route_config_impl_parent_class)->constructed(obj);

    RpRouteConfigImpl* self = RP_ROUTE_CONFIG_IMPL(obj);
    self->m_shared_config = rp_route_common_config_impl_create(self->m_config,
                                                                self->m_factory_context);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpRouteConfigImpl* self = RP_ROUTE_CONFIG_IMPL(obj);
    g_clear_object(&self->m_shared_config);

    G_OBJECT_CLASS(rp_route_config_impl_parent_class)->dispose(obj);
}

static void
rp_route_config_impl_class_init(RpRouteConfigImplClass* klass)
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
                                                    "Creatsion Status",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_DISPATCHER] = g_param_spec_pointer("dispatcher",
                                                    "Dispatcher",
                                                    "Dispatcher",
                                                    G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_route_config_impl_init(RpRouteConfigImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpRouteConfigImpl*
route_config_impl_new(RpRouteConfiguration* config, RpServerFactoryContext* factory_context, RpStatusCode_e* creation_status)
{
    NOISY_MSG_("(%p, %p, %p)", config, factory_context, creation_status);
    return g_object_new(RP_TYPE_ROUTE_CONFIG_IMPL,
                        "config", config,
                        "factory-context", factory_context,
                        "creation-status", creation_status,
                        NULL);
}

RpRouteConfigImpl*
rp_route_config_impl_create(RpRouteConfiguration* config, RpServerFactoryContext* factory_context)
{
    LOGD("(%p, %p)", config, factory_context);
    g_return_val_if_fail(config != NULL, NULL);
    g_return_val_if_fail(RP_IS_SERVER_FACTORY_CONTEXT(factory_context), NULL);
    RpStatusCode_e creation_status = RpStatusCode_Ok;
    RpRouteConfigImpl* ret = route_config_impl_new(config, factory_context, &creation_status);
    if (creation_status != RpStatusCode_Ok)
    {
        return NULL;
    }
    return ret;
}
