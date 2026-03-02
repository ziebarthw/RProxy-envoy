/*
 * rp-rds-config.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"

G_BEGIN_DECLS

/**
 * Base class for router configuration classes used with Rds.
 */
#define RP_TYPE_RDS_CONFIG rp_rds_config_get_type()
G_DECLARE_INTERFACE(RpRdsConfig, rp_rds_config, RP, RDS_CONFIG, GObject)

struct _RpRdsConfigInterface {
    GTypeInterface parent_interface;

};

typedef const SHARED_PTR(RpRdsConfig) RpRdsConfigConstSharedPtr;
typedef SHARED_PTR(RpRdsConfig) RpRdsConfigSharedPtr;

G_END_DECLS
