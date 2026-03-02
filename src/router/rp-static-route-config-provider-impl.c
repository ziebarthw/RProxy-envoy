/*
 * rp-static-route-config-provider-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_static_route_config_provider_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_static_route_config_provider_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-rds.h"
#include "rds/rp-static-route-config-provider-impl.h"
#include "router/rp-static-route-config-provider-impl.h"

#define STATIC_ROUTE_CONFIG_PROVIDER_IMPL(s) RP_STATIC_ROUTE_CONFIG_PROVIDER_IMPL((GObject*)s)

struct _RpStaticRouteConfigProviderImpl {
    GObject parent_instance;

    RpRdsStaticRouteConfigProviderImpl* m_base;
    //TODO...Rds::RouteConfigProviderManager& route_config_provider_manager_;
};

static void route_config_provider_iface_int(RpRouteConfigProviderInterface* iface);
static void rds_route_config_provider_iface_int(RpRdsRouteConfigProviderInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpStaticRouteConfigProviderImpl, rp_static_route_config_provider_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_RDS_ROUTE_CONFIG_PROVIDER, rds_route_config_provider_iface_int)
    G_IMPLEMENT_INTERFACE(RP_TYPE_ROUTE_CONFIG_PROVIDER, route_config_provider_iface_int)
)

static RpRdsConfigConstSharedPtr
config_i(const RpRdsRouteConfigProvider* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_rds_route_config_provider_config(
            RP_RDS_ROUTE_CONFIG_PROVIDER(
                STATIC_ROUTE_CONFIG_PROVIDER_IMPL(self)->m_base));
}

static const RpRdsConfigInfo*
config_info_i(const RpRdsRouteConfigProvider* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_rds_route_config_provider_config_info(
            RP_RDS_ROUTE_CONFIG_PROVIDER(
                STATIC_ROUTE_CONFIG_PROVIDER_IMPL(self)->m_base));
}

static RpSystemTime
last_updated_i(const RpRdsRouteConfigProvider* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_rds_route_config_provider_last_updated(
            RP_RDS_ROUTE_CONFIG_PROVIDER(
                STATIC_ROUTE_CONFIG_PROVIDER_IMPL(self)->m_base));
}

static RpStatusCode_e
on_config_update_i(const RpRdsRouteConfigProvider* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return RpStatusCode_Ok;
}

static void
rds_route_config_provider_iface_int(RpRdsRouteConfigProviderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->config = config_i;
    iface->config_info = config_info_i;
    iface->last_updated = last_updated_i;
    iface->on_config_update = on_config_update_i;
}

static RpRouterConfigConstSharedPtr
config_cast_i(RpRouteConfigProvider* self)
{
    NOISY_MSG_("(%p)", self);
    return (RpRouterConfigConstSharedPtr)rp_rds_route_config_provider_config(
                                            RP_RDS_ROUTE_CONFIG_PROVIDER(
                                                STATIC_ROUTE_CONFIG_PROVIDER_IMPL(self)->m_base));
}

static void
route_config_provider_iface_int(RpRouteConfigProviderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->config_cast = config_cast_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpStaticRouteConfigProviderImpl* self = RP_STATIC_ROUTE_CONFIG_PROVIDER_IMPL(obj);
    g_clear_object(&self->m_base);

    G_OBJECT_CLASS(rp_static_route_config_provider_impl_parent_class)->dispose(obj);
}

static void
rp_static_route_config_provider_impl_class_init(RpStaticRouteConfigProviderImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_static_route_config_provider_impl_init(RpStaticRouteConfigProviderImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpStaticRouteConfigProviderImpl*
rp_static_route_config_provider_impl_new(const RpRouteConfiguration* route_config_proto, RpRdsConfigTraits* config_traits,
                                            RpServerFactoryContext* factory_context, RpRdsRouteConfigProviderManager* route_config_provider_manager)
{
    LOGD("(%p, %p, %p, %p)", route_config_proto, config_traits, factory_context, route_config_provider_manager);

    g_return_val_if_fail(route_config_proto != NULL, NULL);
    g_return_val_if_fail(RP_IS_RDS_CONFIG_TRAITS(config_traits), NULL);
    g_return_val_if_fail(RP_IS_SERVER_FACTORY_CONTEXT(factory_context), NULL);
    g_return_val_if_fail(RP_IS_RDS_ROUTE_CONFIG_PROVIDER_MANAGER(route_config_provider_manager), NULL);

    g_autoptr(RpStaticRouteConfigProviderImpl) self = g_object_new(RP_TYPE_STATIC_ROUTE_CONFIG_PROVIDER_IMPL, NULL);
    if (!self)
    {
        LOGE("failed");
        return NULL;
    }
    self->m_base = rp_rds_static_route_config_provider_impl_new(route_config_proto,
                                                                config_traits,
                                                                factory_context,
                                                                route_config_provider_manager);
    if (!self->m_base)
    {
        LOGE("failed");
        return NULL;
    }
    return g_steal_pointer(&self);
}
