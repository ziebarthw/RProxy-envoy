/*
 * rp-http-pending-stream.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "common/rp-conn-pool.h"
#include "conn_pool/rp-conn-pool-base.h"
#include "conn_pool/rp-pending-stream.h"
#include "rp-codec.h"
#include "rp-http-conn-pool.h"
#include "rp-net-connection.h"

G_BEGIN_DECLS

/*
 * An implementation of Envoy::ConnectionPool::PendingStream for HTTP/1.1 and HTTP/2
 */
#define RP_TYPE_HTTP_PENDING_STREAM rp_http_pending_stream_get_type()
G_DECLARE_FINAL_TYPE(RpHttpPendingStream, rp_http_pending_stream, RP, HTTP_PENDING_STREAM, RpPendingStream)

RpHttpPendingStream* rp_http_pending_stream_new(RpConnPoolImplBase* parent, RpResponseDecoder* decoder, RpHttpConnPoolCallbacks* callbacks, bool can_send_early_data);

G_END_DECLS
