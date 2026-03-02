/*
 * rp-route-config-update-receiver-impl.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_route_config_update_receiver_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_route_config_update_receiver_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "router/rp-router-config-impl.h"
#include "router/rp-route-config-update-receiver-impl.h"

struct _RpRdsConfigTraitsImpl {
    GObject parent_instance;

};

static void rds_config_traits_iface_init(RpRdsConfigTraitsInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpRdsConfigTraitsImpl, rp_rds_config_traits_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_RDS_CONFIG_TRAITS, rds_config_traits_iface_init)
)

static RpRdsConfigConstSharedPtr
create_config_i(RpRdsConfigTraits* self, const RpRouteConfiguration* rc, RpServerFactoryContext* factory_context)
{
    NOISY_MSG_("(%p, %p, %p)", self, rc, factory_context);
    return (RpRdsConfigConstSharedPtr)rp_router_config_impl_create(rc, factory_context);
}

static RpRdsConfigConstSharedPtr
create_null_config_i(RpRdsConfigTraits* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return NULL;
}

static void
rds_config_traits_iface_init(RpRdsConfigTraitsInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_config = create_config_i;
    iface->create_null_config = create_null_config_i;
}

OVERRIDE void
dispose(GObject* object)
{
    NOISY_MSG_("(%p)", object);
    G_OBJECT_CLASS(rp_rds_config_traits_impl_parent_class)->dispose(object);
}

static void
rp_rds_config_traits_impl_class_init(RpRdsConfigTraitsImplClass* klass)
{
    LOGD("(%p)", klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_rds_config_traits_impl_init(RpRdsConfigTraitsImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpRdsConfigTraitsImpl*
rp_rds_config_traits_impl_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_RDS_CONFIG_TRAITS_IMPL, NULL);
}
