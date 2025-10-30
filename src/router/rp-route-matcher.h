/*
 * rp-route-matcher.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-codec.h"
#include "rp-factory-context.h"
#include "rp-route-configuration.h"
#include "rp-router.h"

G_BEGIN_DECLS

/**
 * Wraps the route configuration which matches an incoming request headers to a backend cluster.
 * This is split out mainly to help with unit testing.
 */
#define RP_TYPE_ROUTE_MATCHER rp_route_matcher_get_type()
G_DECLARE_FINAL_TYPE(RpRouteMatcher, rp_route_matcher, RP, ROUTE_MATCHER, GObject)

RpRouteMatcher* rp_route_matcher_create(RpRouteConfiguration* config,
                                        RpRouteCommonConfig* global_route_config,
                                        RpServerFactoryContext* factory_context);
RpRoute* rp_route_matcher_route(RpRouteMatcher* self,
                                RpRouteCallback cb,
                                evhtp_headers_t* request_headers,
                                RpStreamInfo* stream_info,
                                guint64 random_value);

G_END_DECLS
