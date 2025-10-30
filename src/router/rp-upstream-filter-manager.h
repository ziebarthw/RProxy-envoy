/*
 * rp-upstream-filter-manager.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-dispatcher.h"
#include "rp-filter-manager.h"
#include "router/rp-upstream-request.h"

G_BEGIN_DECLS

/*
 * The upstream HTTP filter manager class.
 */
#define RP_TYPE_UPSTREAM_FILTER_MANAGER rp_upstream_filter_manager_get_type()
G_DECLARE_FINAL_TYPE(RpUpstreamFilterManager, rp_upstream_filter_manager, RP, UPSTREAM_FILTER_MANAGER, RpFilterManager)

RpUpstreamFilterManager* rp_upstream_filter_manager_new(RpFilterManagerCallbacks* filter_manager_callbacks,
                                                        RpDispatcher* dispatcher,
                                                        RpNetworkConnection* connection,
                                                        guint64 stream_id,
                                                        bool proxy_100_continue,
                                                        guint32 buffer_limit,
                                                        RpUpstreamRequest* request);

G_END_DECLS
