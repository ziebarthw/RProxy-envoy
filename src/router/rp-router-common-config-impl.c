/*
 * rp-router-common-config-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_router_common_config_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_router_common_config_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-router.h"
#include "router/rp-router-config-impl.h"

#define ROUTER_COMMON_CONFIG_IMPL(s) RP_ROUTER_COMMON_CONFIG_IMPL((GObject*)s)

struct _RpRouterCommonConfigImpl {
    GObject parent_instance;

    GPtrArray* m_internal_only_headers;
    RpRouteConfiguration m_config;
    char* m_name;

    guint32 m_max_direct_reponse_body_size_bytes;

    bool m_uses_vhds : 1;
    bool m_most_specific_header_mutations_wins : 1;
    bool m_ignore_path_parameters_in_path_matching : 1;
};

static void router_common_config_iface_init(RpRouterCommonConfigInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpRouterCommonConfigImpl, rp_router_common_config_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_ROUTER_COMMON_CONFIG, router_common_config_iface_init)
)

static guint32
max_direct_response_body_size_bytes_i(const RpRouterCommonConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return ROUTER_COMMON_CONFIG_IMPL(self)->m_max_direct_reponse_body_size_bytes;
}

static const char*
name_i(const RpRouterCommonConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return ROUTER_COMMON_CONFIG_IMPL(self)->m_name;
}

static void
router_common_config_iface_init(RpRouterCommonConfigInterface* iface)
{
    LOGD("(%p)", iface);
    iface->max_direct_response_body_size_bytes = max_direct_response_body_size_bytes_i;
    iface->name = name_i;
}

OVERRIDE void
dispose(GObject* object)
{
    NOISY_MSG_("(%p)", object);

    RpRouterCommonConfigImpl* self = RP_ROUTER_COMMON_CONFIG_IMPL(object);
    g_clear_pointer(&self->m_name, g_free);

    G_OBJECT_CLASS(rp_router_common_config_impl_parent_class)->dispose(object);
}

static void
rp_router_common_config_impl_class_init(RpRouterCommonConfigImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_router_common_config_impl_init(RpRouterCommonConfigImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpRouterCommonConfigImpl*
constructed(RpRouterCommonConfigImpl* self)
{
    NOISY_MSG_("(%p)", self);

    RpRouteConfiguration* config = &self->m_config;
    self->m_name = g_strdup(config->name);
    if (!self->m_name)
    {
        LOGE("failed");
        return  NULL;
    }
    self->m_max_direct_reponse_body_size_bytes = config->max_direct_response_body_size_bytes;
    self->m_ignore_path_parameters_in_path_matching = config->ignore_path_paramaters_in_path_matching;
    //TODO...self->m_metadata = ...
    return self;
}

static RpRouterCommonConfigImpl*
router_common_config_impl_new(const RpRouteConfiguration* config, RpServerFactoryContext* factory_context G_GNUC_UNUSED, RpStatusCode_e* creation_status)
{
    NOISY_MSG_("(%p, %p, %p)", config, factory_context, creation_status);
    *creation_status = RpStatusCode_Ok;
    g_autoptr(RpRouterCommonConfigImpl) self = g_object_new(RP_TYPE_ROUTER_COMMON_CONFIG_IMPL, NULL);
    self->m_config = *config;
    if (!constructed(self))
    {
        LOGE("error");
        *creation_status = RpStatusCode_EnvoyOverloadError;
        return NULL;
    }
    return g_steal_pointer(&self);
}

RpRouterCommonConfig*
rp_router_common_config_impl_create(const RpRouteConfiguration* config, RpServerFactoryContext* factory_context)
{
    LOGD("(%p, %p)", config, factory_context);

    g_return_val_if_fail(config != NULL, NULL);
    g_return_val_if_fail(RP_IS_SERVER_FACTORY_CONTEXT(factory_context), NULL);

    RpStatusCode_e creation_status = RpStatusCode_Ok;
    g_autoptr(RpRouterCommonConfigImpl) ret = router_common_config_impl_new(config, factory_context, &creation_status);
    if (creation_status != RpStatusCode_Ok)
    {
        LOGE("failed %d", creation_status);
        return NULL;
    }
    return RP_ROUTER_COMMON_CONFIG(g_steal_pointer(&ret));
}
