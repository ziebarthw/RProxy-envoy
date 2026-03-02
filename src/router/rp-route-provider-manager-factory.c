/*
 * rp-route-provider-manager-factory.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_route_provider_manager_factory_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_route_provider_manager_factory_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "router/rp-route-provider-manager.h"

SINGLETON_MANAGER_REGISTRATION(route_config_provider_manager);

struct _RpRouteConfigProviderManagerFactory {
    GObject parent_instance;

    RpSingletonManager* m_singleton_manager;
};

G_DEFINE_TYPE(RpRouteConfigProviderManagerFactory, rp_route_config_provider_manager_factory, G_TYPE_OBJECT)

static RpSingletonInstanceSharedPtr
singleton_factory_cb(void)
{
    NOISY_MSG_("()");
    return RP_SINGLETON_INSTANCE(rp_route_config_provider_manager_impl_new());
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_route_config_provider_manager_factory_parent_class)->dispose(obj);
}

static void
rp_route_config_provider_manager_factory_class_init(RpRouteConfigProviderManagerFactoryClass* klass)
{
    LOGD("(%p)", klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_route_config_provider_manager_factory_init(RpRouteConfigProviderManagerFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpRouteConfigProviderManagerFactory*
rp_route_config_provider_manager_factory_new(RpSingletonManager* singleton_manager)
{
    LOGD("(%p)", singleton_manager);

    g_return_val_if_fail(RP_IS_SINGLETON_MANAGER(singleton_manager), NULL);

    RpRouteConfigProviderManagerFactory* self = g_object_new(RP_TYPE_ROUTE_CONFIG_PROVIDER_MANAGER_FACTORY, NULL);
    self->m_singleton_manager = singleton_manager;
    return self;
}

RpRouteConfigProviderManagerSharedPtr
rp_route_config_provider_manager_factory_get(RpRouteConfigProviderManagerFactory* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_ROuTE_CONFIG_PROVIDER_MANAGER_FACTORY(self), NULL);
    return (RpRouteConfigProviderManagerSharedPtr)rp_singleton_manager_get(self->m_singleton_manager,
                                                    SINGLETON_MANAGER_REGISTERED_NAME(route_config_provider_manager),
                                                    singleton_factory_cb,
                                                    true);
}
