/*
 * rp-route-config-update-receiver-impl.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-rds-config-traits.h"

G_BEGIN_DECLS

#define RP_TYPE_RDS_CONFIG_TRAITS_IMPL rp_rds_config_traits_impl_get_type()
G_DECLARE_FINAL_TYPE(RpRdsConfigTraitsImpl, rp_rds_config_traits_impl, RP, RDS_CONFIG_TRAITS_IMPL, GObject)

RpRdsConfigTraitsImpl* rp_rds_config_traits_impl_new(void);

G_END_DECLS
