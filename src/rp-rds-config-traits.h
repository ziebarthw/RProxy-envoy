/*
 * rp-rds-config-traits.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-route-configuration.h"
#include "rp-rds-config.h"
#include "rp-factory-context.h"

G_BEGIN_DECLS

#define RP_TYPE_RDS_CONFIG_TRAITS rp_rds_config_traits_get_type()
G_DECLARE_INTERFACE(RpRdsConfigTraits, rp_rds_config_traits, RP, RDS_CONFIG_TRAITS, GObject)

struct _RpRdsConfigTraitsInterface {
    GTypeInterface parent_iface;

    RpRdsConfigConstSharedPtr (*create_null_config)(RpRdsConfigTraits*);
    RpRdsConfigConstSharedPtr (*create_config)(RpRdsConfigTraits*,
                                                const RpRouteConfiguration*,
                                                RpServerFactoryContext*);
};

static inline RpRdsConfigConstSharedPtr
rp_rds_config_traits_create_null_config(RpRdsConfigTraits* self)
{
    return RP_IS_RDS_CONFIG_TRAITS(self) ?
        RP_RDS_CONFIG_TRAITS_GET_IFACE(self)->create_null_config(self) : NULL;
}
static inline RpRdsConfigConstSharedPtr
rp_rds_config_traits_create_config(RpRdsConfigTraits* self, const RpRouteConfiguration* rc, RpServerFactoryContext* factory_context)
{
    return RP_IS_RDS_CONFIG_TRAITS(self) ?
        RP_RDS_CONFIG_TRAITS_GET_IFACE(self)->create_config(self, rc, factory_context) :
        NULL;
}

G_END_DECLS
