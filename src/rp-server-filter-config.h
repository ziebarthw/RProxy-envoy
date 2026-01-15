/*
 * rp-server-filter-config.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-factory-context.h"

G_BEGIN_DECLS

typedef struct _RpFilterConfigBase RpFilterConfigBase;
struct _RpFilterConfigBase {
    const char* name;
    RpFactoryContext* context;
};

static inline RpFilterConfigBase
rp_filter_config_base_ctor(const char* name, RpFactoryContext* context)
{
    RpFilterConfigBase self = {
        .name = name,
        .context = context
    };
    return self;
}

G_END_DECLS
