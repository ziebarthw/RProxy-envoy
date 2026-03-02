/*
 * rp-static-route-config-provider-impl.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_static_route_config_provider_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_static_route_config_provider_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rds/rp-static-route-config-provider-impl.h"

#define RDS_STATIC_ROUTE_CONFIG_PROVIDER_IMPL(s) RP_RDS_STATIC_ROUTE_CONFIG_PROVIDER_IMPL((GObject*)s)

struct _RpRdsStaticRouteConfigProviderImpl {
    GObject parent_instance;

    RpRdsRouteConfigProviderManager* m_route_config_provider_manager;
    RpServerFactoryContext* m_factory_context;
    RpRouteConfiguration m_route_config_proto;
    RpRdsConfig* m_config;
    RpSystemTime m_last_updated;
    RpRdsConfigInfo m_config_info;
};

static void rds_route_config_provider_iface_init(RpRdsRouteConfigProviderInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpRdsStaticRouteConfigProviderImpl, rp_rds_static_route_config_provider_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_RDS_ROUTE_CONFIG_PROVIDER, rds_route_config_provider_iface_init)
)

static RpRdsConfigConstSharedPtr
config_i(const RpRdsRouteConfigProvider* self)
{
    NOISY_MSG_("(%p)", self);
    return RDS_STATIC_ROUTE_CONFIG_PROVIDER_IMPL(self)->m_config;
}

static const RpRdsConfigInfo*
config_info_i(const RpRdsRouteConfigProvider* self)
{
    NOISY_MSG_("(%p)", self);
    return &RDS_STATIC_ROUTE_CONFIG_PROVIDER_IMPL(self)->m_config_info;
}

static RpSystemTime
last_updated_i(const RpRdsRouteConfigProvider* self)
{
    NOISY_MSG_("(%p)", self);
    return RDS_STATIC_ROUTE_CONFIG_PROVIDER_IMPL(self)->m_last_updated;
}

static void
rds_route_config_provider_iface_init(RpRdsRouteConfigProviderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->config = config_i;
    iface->config_info = config_info_i;
    iface->last_updated = last_updated_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpRdsStaticRouteConfigProviderImpl* self = RP_RDS_STATIC_ROUTE_CONFIG_PROVIDER_IMPL(obj);
NOISY_MSG_("%p, clearing config %p(%u)", obj, self->m_config, G_OBJECT(self->m_config)->ref_count);
    g_clear_object(&self->m_config);

    G_OBJECT_CLASS(rp_rds_static_route_config_provider_impl_parent_class)->dispose(obj);
}

static void
rp_rds_static_route_config_provider_impl_class_init(RpRdsStaticRouteConfigProviderImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_rds_static_route_config_provider_impl_init(RpRdsStaticRouteConfigProviderImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpRdsStaticRouteConfigProviderImpl*
rp_rds_static_route_config_provider_impl_new(const RpRouteConfiguration* route_config_proto, RpRdsConfigTraits* config_traits,
                                                RpServerFactoryContext* factory_context, RpRdsRouteConfigProviderManager* route_config_provider_manager)
{
    LOGD("(%p, %p, %p, %p)",
        route_config_proto, config_traits, factory_context, route_config_provider_manager);

    g_return_val_if_fail(route_config_proto != NULL, NULL);
    g_return_val_if_fail(RP_IS_RDS_CONFIG_TRAITS(config_traits), NULL);
    g_return_val_if_fail(RP_IS_SERVER_FACTORY_CONTEXT(factory_context), NULL);
    g_return_val_if_fail(RP_IS_RDS_ROUTE_CONFIG_PROVIDER_MANAGER(route_config_provider_manager), NULL);

    g_autoptr(RpRdsStaticRouteConfigProviderImpl) self = g_object_new(RP_TYPE_RDS_STATIC_ROUTE_CONFIG_PROVIDER_IMPL, NULL);
    if (!self)
    {
        LOGE("failed");
        return NULL;
    }
    self->m_route_config_provider_manager = route_config_provider_manager;
    self->m_route_config_proto = *route_config_proto;
    g_autoptr(RpRdsConfig) config = (RpRdsConfig*)rp_rds_config_traits_create_config(config_traits, &self->m_route_config_proto, factory_context);
    if (!config)
    {
        LOGE("failed");
        return NULL;
    }
    self->m_config = g_steal_pointer(&config);
    self->m_last_updated = rp_time_source_system_time(
                            rp_common_factory_context_time_source(
                                RP_COMMON_FACTORY_CONTEXT(factory_context)));
    return g_steal_pointer(&self);
}
