/*
 * rp-http-upstream.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define RP_TYPE_HTTP_UPSTREAM rp_http_upstream_get_type()
G_DECLARE_DERIVABLE_TYPE(RpHttpUpstream, rp_http_upstream, RP, HTTP_UPSTREAM, GObject)

struct _RpHttpUpstreamClass {
    GObjectClass parent_class;

};

RpUpstreamToDownstream* rp_http_upstream_upstream_request_(RpHttpUpstream* self);

G_END_DECLS
