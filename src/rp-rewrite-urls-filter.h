/*
 * rp-rewrite-urls-filter.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-factory-context.h"
#include "rp-pass-through-filter.h"
#include "rp-server-filter-config.h"
#include "rproxy.h"

G_BEGIN_DECLS

typedef struct _RpRewriteUrlsCfg RpRewriteUrlsCfg;
struct _RpRewriteUrlsCfg {
    RpFilterConfigBase config;
};

static inline RpRewriteUrlsCfg
rp_rewrite_urls_cfg_ctor(void)
{
    RpRewriteUrlsCfg self = {
        .config = rp_filter_config_base_ctor("rewrite-urls-filter")
    };
    return self;
}

#define RP_TYPE_REWRITE_URLS_FILTER (rp_rewrite_urls_filter_get_type())
G_DECLARE_FINAL_TYPE(RpRewriteUrlsFilter, rp_rewrite_urls_filter, RP, REWRITE_URLS_FILTER, RpPassThroughFilter)

RpFilterFactoryCb* rp_rewrite_urls_filter_create_filter_factory(RpRewriteUrlsCfg* config,
                                                                RpFactoryContext* context);

G_END_DECLS
