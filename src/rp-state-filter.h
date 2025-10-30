/*
 * rp-state-filter.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-factory-context.h"
#include "rp-pass-through-filter.h"

G_BEGIN_DECLS

extern const char* rewrite_urls_key;
extern const char* rule_key;
extern const char* original_uri_key;
extern const char* soup_request_headers_key;
extern const char* soup_response_headers_key;
extern const char* passthrough_key;
extern const char* dynamic_host_key;
extern const char* dynamic_port_key;

#define RP_TYPE_STATE_FILTER rp_state_filter_get_type()
G_DECLARE_FINAL_TYPE(RpStateFilter, rp_state_filter, RP, STATE_FILTER, RpPassThroughFilter)

RpFilterFactoryCb* rp_state_filter_create_filter_factory(RpFactoryContext* context);

G_END_DECLS
