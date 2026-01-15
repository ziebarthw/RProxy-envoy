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
#include "router/rp-route-config-impl.h"
#include "router/rp-static-route-config-provider-impl.h"

struct _RpStaticRouteConfigProviderImpl {
    GObject parent_instance;

    gint64 m_last_update;
    RpRouteConfigImpl* m_config;
    RpRdsConfigInfo m_config_info;
    RpRouteConfiguration m_config_proto;
    RpThreadLocalInstance* m_tls;
};

static void rds_route_config_provider_iface_int(RpRdsRouteConfigProviderInterface* iface);
static void route_config_provider_iface_int(RpRouteConfigProviderInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpStaticRouteConfigProviderImpl, rp_static_route_config_provider_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_RDS_ROUTE_CONFIG_PROVIDER, rds_route_config_provider_iface_int)
    G_IMPLEMENT_INTERFACE(RP_TYPE_ROUTE_CONFIG_PROVIDER, route_config_provider_iface_int)
)

static RpRouteConfig*
config_i(RpRdsRouteConfigProvider* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_ROUTE_CONFIG(RP_STATIC_ROUTE_CONFIG_PROVIDER_IMPL(self)->m_config);
}

static RpRdsConfigInfo*
config_info_i(RpRdsRouteConfigProvider* self)
{
    NOISY_MSG_("(%p)", self);
    return &RP_STATIC_ROUTE_CONFIG_PROVIDER_IMPL(self)->m_config_info;
}

static gint64
last_updated_i(RpRdsRouteConfigProvider* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_STATIC_ROUTE_CONFIG_PROVIDER_IMPL(self)->m_last_update;
}

static RpStatusCode_e
on_config_update_i(RpRdsRouteConfigProvider* self G_GNUC_UNUSED)
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

static RpRouteConfig*
config_cast_i(RpRouteConfigProvider* self)
{
    NOISY_MSG_("(%p)", self);
    return g_object_ref(RP_ROUTE_CONFIG(RP_STATIC_ROUTE_CONFIG_PROVIDER_IMPL(self)->m_config));
}

static void
route_config_provider_iface_int(RpRouteConfigProviderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->config_cast = config_cast_i;
}

OVERRIDE void
dispose(GObject* object)
{
    NOISY_MSG_("(%p)", object);

    RpStaticRouteConfigProviderImpl* self = RP_STATIC_ROUTE_CONFIG_PROVIDER_IMPL(object);
    g_clear_object(&self->m_config);

    G_OBJECT_CLASS(rp_static_route_config_provider_impl_parent_class)->dispose(object);
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
rp_static_route_config_provider_impl_new(RpRouteConfiguration* route_config_proto, RpServerFactoryContext* factory_context, RpThreadLocalInstance* tls)
{
    LOGD("(%p, %p, %p)", route_config_proto, factory_context, tls);

    g_return_val_if_fail(route_config_proto != NULL, NULL);
    g_return_val_if_fail(RP_IS_SERVER_FACTORY_CONTEXT(factory_context), NULL);
    g_return_val_if_fail(RP_IS_THREAD_LOCAL_INSTANCE(tls), NULL);

    RpStaticRouteConfigProviderImpl* self = g_object_new(RP_TYPE_STATIC_ROUTE_CONFIG_PROVIDER_IMPL, NULL);
    self->m_last_update = g_get_real_time();
    self->m_config_proto = *route_config_proto;
    self->m_config_info = rp_rds_config_info_ctor(self->m_config_proto, "");
    self->m_config = rp_route_config_impl_create(&self->m_config_proto, factory_context, tls);
    return self;
}
