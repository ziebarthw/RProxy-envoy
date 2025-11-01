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
    object_class->dispose = dispose;
}

static void
rp_router_filter_config_init(RpRouterFilterConfig* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpRouterFilterConfig*
constructed(RpRouterFilterConfig* self)
{
    NOISY_MSG_("(%p)", self);

    RpRouterCfg* config = self->m_config;
    self->m_suppress_envoy_headers = config->suppress_rproxy_headers;
    self->m_config = NULL;
    return self;
}

RpRouterFilterConfig*
rp_router_filter_config_new(RpRouterCfg* proto_config, RpFactoryContext* factory_context)
{
    LOGD("(%p, %p)", proto_config, factory_context);
    g_return_val_if_fail(proto_config != NULL, NULL);
    g_return_val_if_fail(RP_IS_FACTORY_CONTEXT(factory_context), NULL);
    RpRouterFilterConfig* self = g_object_new(RP_TYPE_ROUTER_FILTER_CONFIG, NULL);
    self->m_config = proto_config;
    self->m_factory_context = RP_COMMON_FACTORY_CONTEXT(factory_context);
    return constructed(self);
}
