/*
 * rp-per-host-http-upstream.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-router.h"
#include "upstream/rp-http-upstream.h"

G_BEGIN_DECLS

/*
 * The http upstream to remember the host and encode the host metadata and cluster metadata into
 * upstream http request.
 */
#define RP_TYPE_PER_HOST_HTTP_UPSTREAM rp_per_host_http_upstream_get_type()
G_DECLARE_FINAL_TYPE(RpPerHostHttpUpstream, rp_per_host_http_upstream, RP, PER_HOST_HTTP_UPSTREAM, RpHttpUpstream)

RpPerHostHttpUpstream* rp_per_host_http_upstream_new(RpUpstreamToDownstream* upstream_request, RpRequestEncoder* encoder, RpHostDescription* host);

G_END_DECLS
