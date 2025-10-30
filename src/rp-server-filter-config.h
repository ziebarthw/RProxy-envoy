/*
 * rp-server-filter-config.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _RpFilterConfigBase RpFilterConfigBase;
struct _RpFilterConfigBase {
    const char* name;
};

static inline RpFilterConfigBase
rp_filter_config_base_ctor(const char* name)
{
    RpFilterConfigBase self = {
        .name = name
    };
    return self;
}

G_END_DECLS
