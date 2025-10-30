/*
 * rp-http-conn-manager-config.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-conn-manager-config.h"
#include "rp-factory-context.h"
#include "rp-route-configuration.h"
#include "rp-rds.h"
#include "lzq.h"

G_BEGIN_DECLS

// Ultra-simplistic version of protobuf patterned after RProxy cfg structs.
typedef struct _RpHttpConnectionManagerCfg RpHttpConnectionManagerCfg;
struct _RpHttpConnectionManagerCfg {

    char* codec_type; // "AUTO"(def), "HTTP1", "HTTP2", "HTTP3"
    guint32 max_request_headers_kb;

    struct {
        guint64 idle_timeout;
        guint64 max_connection_duration;
        guint32 max_headers_count;
        guint32 max_response_headers_kb;
        guint64 max_stream_duration;
        char* headers_with_undercores_action; // "ALLOW"(def), "REJECT_REQUEST", "DROP_HEADER"
        guint32 max_requests_per_connection;
    } http_protocol_options;

    RpRouteConfiguration route_config;

    lztq* rules;
};

/**
 * Maps proto config to runtime config for an HTTP connection manager network filter.
 */
#define RP_TYPE_HTTP_CONNECTION_MANAGER_CONFIG rp_http_connection_manager_config_get_type()
G_DECLARE_FINAL_TYPE(RpHttpConnectionManagerConfig, rp_http_connection_manager_config, RP, HTTP_CONNECTION_MANAGER_CONFIG, GObject)

RpHttpConnectionManagerConfig* rp_http_connection_manager_config_new(RpHttpConnectionManagerCfg* config,
                                                                        RpFactoryContext* context);
void rp_http_connection_manager_config_add_filter_factory(RpHttpConnectionManagerConfig* self,
                                                            RpFilterFactoryCb* cb,
                                                            bool disabled);
RpFactoryContext* rp_http_connection_manager_config_factory_context(RpHttpConnectionManagerConfig* self);

G_END_DECLS
