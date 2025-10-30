/*
 * rp-default-upstream-http-filter-chain-factory.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "router/rp-upstream-codec-filter.h"

G_BEGIN_DECLS

#define RP_TYPE_DEFAULT_UPSTREAM_HTTP_FILTER_CHAIN_FACTORY rp_default_upstream_http_filter_chain_factory_get_type()
G_DECLARE_FINAL_TYPE(RpDefaultUpstreamHttpFilterChainFactory, rp_default_upstream_http_filter_chain_factory, RP, DEFAULT_UPSTREAM_HTTP_FILTER_CHAIN_FACTORY, GObject)

RpDefaultUpstreamHttpFilterChainFactory* rp_default_upstream_http_filter_chain_factory_new(void);
void rp_default_upstream_http_filter_chain_factory_add_filter_factory(RpDefaultUpstreamHttpFilterChainFactory* self,
                                                                        RpFilterFactoryCb* cb,
                                                                        bool disabled);

G_END_DECLS
