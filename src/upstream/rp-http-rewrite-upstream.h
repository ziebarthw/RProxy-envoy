/*
 * rp-http-rewrite-upstream.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "upstream/rp-http-upstream.h"
#include "rp-factory-context.h"

G_BEGIN_DECLS

#define RP_TYPE_HTTP_REWRITE_UPSTREAM rp_http_rewrite_upstream_get_type()
G_DECLARE_FINAL_TYPE(RpHttpRewriteUpstream, rp_http_rewrite_upstream, RP, HTTP_REWRITE_UPSTREAM, GObject)

RpFilterFactoryCb* rp_http_rewrite_filter_create_filter_factory(RpFactoryContext* context);

G_END_DECLS
