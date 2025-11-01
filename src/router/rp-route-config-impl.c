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

    SHARED_PTR(RpRouteConfiguration) m_config;
    SHARED_PTR(RpServerFactoryContext) m_factory_context;

    SHARED_PTR(RpDispatcher) m_dispatcher;

    UNIQUE_PTR(RpRouteCommonConfigImpl) m_shared_config;
};

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
    object_class->dispose = dispose;
}

static void
rp_route_config_impl_init(RpRouteConfigImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpRouteConfigImpl*
constructed(RpRouteConfigImpl* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_shared_config = rp_route_common_config_impl_create(self->m_config,
                                                                self->m_factory_context);
    return self;
}

static inline RpRouteConfigImpl*
route_config_impl_new(RpRouteConfiguration* config, RpServerFactoryContext* factory_context, RpStatusCode_e* creation_status)
{
    NOISY_MSG_("(%p, %p, %p)", config, factory_context, creation_status);
    RpRouteConfigImpl* self = g_object_new(RP_TYPE_ROUTE_CONFIG_IMPL, NULL);
    self->m_config = config;
    self->m_factory_context = factory_context;
    *creation_status = RpStatusCode_Ok;
    return constructed(self);
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

void
rp_route_config_impl_set_dispatcher(RpRouteConfigImpl* self, SHARED_PTR(RpDispatcher) dispatcher)
{
    LOGD("(%p, %p)", self, dispatcher);
    g_return_if_fail(RP_IS_ROUTE_CONFIG_IMPL(self));
    g_return_if_fail(RP_IS_DISPATCHER(dispatcher));
    self->m_dispatcher = dispatcher;
}
