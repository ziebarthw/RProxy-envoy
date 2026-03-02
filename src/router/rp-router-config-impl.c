/*
 * rp-router-config-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_router_config_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_router_config_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rproxy.h"
#include "rp-headers.h"
#include "rp-rds-config.h"
#include "event/rp-dispatcher-impl.h"
#include "thread_local/rp-thread-local-impl.h"
#include "router/rp-route-impl.h"
#include "router/rp-router-config-impl.h"

#define ROUTER_CONFIG_IMPL(s) RP_ROUTER_CONFIG_IMPL((GObject*)s)

struct _RpRouterConfigImpl {
    GObject parent_instance;

    RpRouterCommonConfig* m_shared_config;
    //TODO...std::unique_ptr<RouteMatcher> route_matcher_;
};

static void router_config_iface_init(RpRouterConfigInterface* iface);
static void router_common_config_iface_init(RpRouterCommonConfigInterface* iface);
static void rds_config_iface_init(RpRdsConfigInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpRouterConfigImpl, rp_router_config_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_RDS_CONFIG, rds_config_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_ROUTER_COMMON_CONFIG, router_common_config_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_ROUTER_CONFIG, router_config_iface_init)
)

static const GPtrArray*
internal_only_headers_i(const RpRouterCommonConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_router_common_config_internal_only_headers(ROUTER_CONFIG_IMPL(self)->m_shared_config);
}

static const char*
name_i(const RpRouterCommonConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_router_common_config_name(ROUTER_CONFIG_IMPL(self)->m_shared_config);
}

static bool
uses_vhds_i(const RpRouterCommonConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_router_common_config_uses_vhds(ROUTER_CONFIG_IMPL(self)->m_shared_config);
}

static bool
most_specific_header_mutation_wins_i(const RpRouterCommonConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_router_common_config_most_specific_header_mutation_wins(ROUTER_CONFIG_IMPL(self)->m_shared_config);
}

static guint32
max_direct_response_body_size_bytes_i(const RpRouterCommonConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_router_common_config_max_direct_response_body_size_bytes(ROUTER_CONFIG_IMPL(self)->m_shared_config);
}

static RpMetadataConstSharedPtr
metadata_i(const RpRouterCommonConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_router_common_config_metadata(ROUTER_CONFIG_IMPL(self)->m_shared_config);
}

static void
router_common_config_iface_init(RpRouterCommonConfigInterface* iface)
{
    LOGD("(%p)", iface);
    iface->internal_only_headers = internal_only_headers_i;
    iface->name = name_i;
    iface->uses_vhds = uses_vhds_i;
    iface->most_specific_header_mutation_wins = most_specific_header_mutation_wins_i;
    iface->max_direct_response_body_size_bytes = max_direct_response_body_size_bytes_i;
    iface->metadata = metadata_i;
}

static void
rds_config_iface_init(RpRdsConfigInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

static RpRouteConstSharedPtr
route_i(const RpRouterConfig* self, RpRouteCallback cb, evhtp_headers_t* request_headers, const RpStreamInfo* stream_info, guint64 random_value)
{
    NOISY_MSG_("(%p, %p, %p, %p, %zu)", self, cb, request_headers, stream_info, random_value);

    const char* hostname = evhtp_header_find(request_headers, RpHeaderValues.HostLegacy);
    const char* path = evhtp_header_find(request_headers, RpHeaderValues.Path);
    RpDispatcher* dispatcher = rp_thread_local_instance_impl_get_dispatcher();
    evthr_t* thr = rp_dispatcher_thr(dispatcher);
    rproxy_t* rproxy = evthr_get_aux(thr);
    // |path| may be nullptr if it is a CONNECT request.
    rule_cfg_t* rule_cfg = evhtp_path_match(rproxy->htp, hostname, path ? path : "/");
    return rule_cfg ? RP_ROUTE(rp_route_impl_new(self, rule_cfg)) : NULL;
}

static void
router_config_iface_init(RpRouterConfigInterface* iface)
{
    LOGD("(%p)", iface);
    iface->route = route_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpRouterConfigImpl* self = ROUTER_CONFIG_IMPL(obj);
NOISY_MSG_("%p, clearing route common config %p(%u)", self, self->m_shared_config, G_OBJECT(self->m_shared_config)->ref_count);
    g_clear_object(&self->m_shared_config);

    G_OBJECT_CLASS(rp_router_config_impl_parent_class)->dispose(obj);
}

static void
rp_router_config_impl_class_init(RpRouterConfigImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_router_config_impl_init(RpRouterConfigImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpRouterConfigImpl*
constructed(RpRouterConfigImpl* self, const RpRouteConfiguration* config, RpServerFactoryContext* factory_context)
{
    NOISY_MSG_("(%p, %p, %p)", self, config, factory_context);

    self->m_shared_config = RP_ROUTER_COMMON_CONFIG(rp_router_common_config_impl_create(config, factory_context));
    if (!self->m_shared_config)
    {
        LOGE("failed");
        return NULL;
    }
    return self;
}

static inline RpRouterConfigImpl*
router_config_impl_new(const RpRouteConfiguration* config, RpServerFactoryContext* factory_context, RpStatusCode_e* creation_status)
{
    NOISY_MSG_("(%p, %p, %p)", config, factory_context, creation_status);

    *creation_status = RpStatusCode_Ok;
    g_autoptr(RpRouterConfigImpl) self = g_object_new(RP_TYPE_ROUTER_CONFIG_IMPL, NULL);
    if (!self)
    {
        LOGE("failed");
        *creation_status = RpStatusCode_CodecProtocolError;
        return NULL;
    }
    if (!constructed(self, config, factory_context))
    {
        LOGE("failed");
        *creation_status = RpStatusCode_CodecProtocolError;
        return NULL;
    }
    return g_steal_pointer(&self);
}

RpRouterConfigImpl*
rp_router_config_impl_create(const RpRouteConfiguration* config, RpServerFactoryContext* factory_context)
{
    LOGD("(%p, %p)", config, factory_context);

    g_return_val_if_fail(config != NULL, NULL);
    g_return_val_if_fail(RP_IS_SERVER_FACTORY_CONTEXT(factory_context), NULL);

    RpStatusCode_e creation_status = RpStatusCode_Ok;
    g_autoptr(RpRouterConfigImpl) ret = router_config_impl_new(config, factory_context, &creation_status);
    if (creation_status != RpStatusCode_Ok)
    {
        LOGE("failed");
        return NULL;
    }
    return g_steal_pointer(&ret);
}
