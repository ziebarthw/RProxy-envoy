/*
 * rp-http1-client-connection-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-codec.h"
#include "rp-http1-connection-impl.h"

G_BEGIN_DECLS

/**
 * Implementation of Http::ClientConnection for HTTP/1.1.
 */
// https://github.com/envoyproxy/envoy/blob/main/source/common/http/http1/codec_impl.h#L588
#define RP_TYPE_HTTP1_CLIENT_CONNECTION_IMPL rp_http1_client_connection_impl_get_type()
G_DECLARE_FINAL_TYPE(RpHttp1ClientConnectionImpl, rp_http1_client_connection_impl, RP, HTTP1_CLIENT_CONNECTION_IMPL, RpHttp1ConnectionImpl)

RpHttp1ClientConnectionImpl* rp_http1_client_connection_impl_new(RpNetworkConnection* connection,
                                            RpHttpConnectionCallbacks* callbacks,
                                            const struct RpHttp1Settings_s* settings,
                                            guint16 max_response_headers_kb,
                                            guint32 max_response_headers_count,
                                            bool passing_through_proxy);
bool rp_http1_client_connection_impl_cannot_have_body(RpHttp1ClientConnectionImpl* self);

G_END_DECLS
