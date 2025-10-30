/*
 * rp-http1-server-connection-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-net-connection.h"
#include "rp-http1-connection-impl.h"

G_BEGIN_DECLS

/**
 * Implementation of Http::ServerConnection for HTTP/1.1.
 */
// https://github.com/envoyproxy/envoy/blob/main/source/common/http/http1/codec_impl.h#L465
#define RP_TYPE_HTTP1_SERVER_CONNECTION_IMPL rp_http1_server_connection_impl_get_type()
G_DECLARE_FINAL_TYPE(RpHttp1ServerConnectionImpl, rp_http1_server_connection_impl, RP, HTTP1_SERVER_CONNECTION_IMPL, RpHttp1ConnectionImpl)

RpHttp1ServerConnectionImpl* rp_http1_server_connection_impl_new(RpNetworkConnection* connection,
                                                                    RpHttpServerConnectionCallbacks* callbacks,
                                                                    const struct RpHttp1Settings_s* settings,
                                                                    guint32 max_request_headers_kb,
                                                                    guint32 max_request_headers_count);

G_END_DECLS
