/*
 * rp-route-provider-manager.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_route_provider_manager_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_route_provider_manager_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "router/rp-router-config-impl.h"
#include "router/rp-route-config-update-receiver-impl.h"
#include "router/rp-static-route-config-provider-impl.h"
#include "router/rp-route-provider-manager.h"

struct _RpRouteConfigProviderManagerImpl {
    GObject parent_instance;

    RpRdsRouteConfigProviderManager* m_manager;
};

static void route_config_provider_manager_iface_init(RpRouteConfigProviderManagerInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpRouteConfigProviderManagerImpl, rp_route_config_provider_manager_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_ROUTE_CONFIG_PROVIDER_MANAGER, route_config_provider_manager_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_SINGLETON_INSTANCE, NULL)
)

typedef struct _RpCreateStaticProviderCtx RpCreateStaticProviderCtx;
struct _RpCreateStaticProviderCtx {
    RpServerFactoryContext* factory_context;
    const RpRouteConfiguration* route_config;
    RpRouteConfigProviderManagerImpl* this_;
};

static RpRdsRouteConfigProviderPtr
create_static_provider(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    RpCreateStaticProviderCtx* ctx = arg;
    g_autoptr(RpRdsConfigTraitsImpl) config_traits = rp_rds_config_traits_impl_new();
    return (RpRdsRouteConfigProviderPtr)rp_static_route_config_provider_impl_new(ctx->route_config,
                                                                                (RpRdsConfigTraits*)config_traits,
                                                                                ctx->factory_context,
                                                                                ctx->this_->m_manager);
}

static RpRouteConfigProviderPtr
create_static_route_config_provider_i(RpRouteConfigProviderManager* self, const RpRouteConfiguration* route_config, RpServerFactoryContext* factory_context)
{
    NOISY_MSG_("(%p, %p, %p)", self, route_config, factory_context);
    RpRouteConfigProviderManagerImpl* me = RP_ROUTE_CONFIG_PROVIDER_MANAGER_IMPL(self);
    RpCreateStaticProviderCtx ctx = {
        .factory_context = factory_context,
        .route_config = route_config,
        .this_ = me
    };
    return (RpRouteConfigProviderPtr)rp_rds_route_config_provider_manager_add_static_provider(me->m_manager, create_static_provider, &ctx);
}

static void
route_config_provider_manager_iface_init(RpRouteConfigProviderManagerInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_static_route_config_provider = create_static_route_config_provider_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpRouteConfigProviderManagerImpl* self = RP_ROUTE_CONFIG_PROVIDER_MANAGER_IMPL(obj);
    g_clear_object(&self->m_manager);

    G_OBJECT_CLASS(rp_route_config_provider_manager_impl_parent_class)->dispose(obj);
}

static void
rp_route_config_provider_manager_impl_class_init(RpRouteConfigProviderManagerImplClass* klass)
{
    LOGD("(%p)", klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_route_config_provider_manager_impl_init(RpRouteConfigProviderManagerImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpRouteConfigProviderManager*
rp_route_config_provider_manager_impl_new(void)
{
    LOGD("()");

    g_autoptr(RpRouteConfigProviderManagerImpl) self = g_object_new(RP_TYPE_ROUTE_CONFIG_PROVIDER_MANAGER_IMPL, NULL);
    if (!self)
    {
        LOGE("failed");
        return NULL;
    }

    self->m_manager = rp_rds_route_config_provider_manager_new();
    if (!self->m_manager)
    {
        LOGE("failed");
        return NULL;
    }

    return RP_ROUTE_CONFIG_PROVIDER_MANAGER(g_steal_pointer(&self));
}
