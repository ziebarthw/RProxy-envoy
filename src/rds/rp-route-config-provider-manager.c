/*
 * rp-route-config-provider-manager.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_route_config_provider_manager_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_route_config_provider_manager_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rds/rp-route-config-provider-manager.h"

struct _RpRdsRouteConfigProviderManager {
    GObject parent_instance;

    GHashTable* m_static_route_config_providers;
};

G_DEFINE_TYPE(RpRdsRouteConfigProviderManager, rp_rds_route_config_provider_manager, G_TYPE_OBJECT)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpRdsRouteConfigProviderManager* self = RP_RDS_ROUTE_CONFIG_PROVIDER_MANAGER(obj);
    g_hash_table_destroy(g_steal_pointer(&self->m_static_route_config_providers));

    G_OBJECT_CLASS(rp_rds_route_config_provider_manager_parent_class)->dispose(obj);
}

static void
rp_rds_route_config_provider_manager_class_init(RpRdsRouteConfigProviderManagerClass* klass)
{
    LOGD("(%p)", klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_rds_route_config_provider_manager_init(RpRdsRouteConfigProviderManager* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_static_route_config_providers = g_hash_table_new_full(NULL, NULL, g_object_unref, NULL);
}

RpRdsRouteConfigProviderManager*
rp_rds_route_config_provider_manager_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_RDS_ROUTE_CONFIG_PROVIDER_MANAGER, NULL);
}

RpRdsRouteConfigProviderPtr
rp_rds_route_config_provider_manager_add_static_provider(RpRdsRouteConfigProviderManager* self,
                                                            RpRdsRouteConfigProviderPtr (*create_static_provider)(gpointer), gpointer arg)
{
    LOGD("(%p, %p, %p)", self, create_static_provider, arg);
    g_return_val_if_fail(RP_IS_RDS_ROUTE_CONFIG_PROVIDER_MANAGER(self), NULL);
    g_return_val_if_fail(create_static_provider != NULL, NULL);
    RpRdsRouteConfigProviderPtr provider = create_static_provider(arg);
    if (!g_hash_table_add(self->m_static_route_config_providers, g_object_ref(provider)))
    {
        LOGE("failed");
    }
    return provider;
}
