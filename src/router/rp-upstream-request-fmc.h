/*
 * rp-upstream-request-fmc.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "router/rp-upstream-request.h"

G_BEGIN_DECLS

// UpstreamRequestFilterManagerCallbacks
#define RP_TYPE_UPSTREAM_REQUEST_FMC rp_upstream_request_fmc_get_type()
G_DECLARE_FINAL_TYPE(RpUpstreamRequestFmc, rp_upstream_request_fmc, RP, UPSTREAM_REQUEST_FMC, GObject)

RpUpstreamRequestFmc* rp_upstream_request_fmc_new(RpUpstreamRequest* upstream_request);

G_END_DECLS
