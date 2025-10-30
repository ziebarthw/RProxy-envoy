/*
 * rp-upstream-request.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-codec.h"
#include "rp-http-conn-pool.h"
#include "rp-http-filter.h"
#include "rp-router.h"

G_BEGIN_DECLS

typedef struct _RpGenericUpstream RpGenericUpstream;
typedef struct _RpRouterFilterInterface RpRouterFilterInterface;

/* The Upstream request is the base class for forwarding HTTP upstream.
 *
 * On the new request path, payload (headers/body/metadata/data) still arrives via
 * the accept[X]fromRouter functions. Said data is immediately passed off to the
 * UpstreamFilterManager, which passes each item through the filter chain until
 * it arrives at the last filter in the chain, the UpstreamCodecFilter. If the upstream
 * stream is not established, the UpstreamCodecFilter returns StopAllIteration, and the
 * FilterManager will buffer data, using watermarks to push back to the router
 * filter if buffers become overrun. When an upstream connection is established,
 * the UpstreamCodecFilter will send data upstream.
 *
 * On the new response path, payload arrives from upstream via the UpstreamCodecFilter's
 * CodecBridge. It is passed off directly to the FilterManager, traverses the
 * filter chain, and completion is signaled via the
 * UpstreamRequestFilterManagerCallbacks's encode[X] functions. These somewhat
 * confusingly pass through the UpstreamRequest's legacy decode[X] functions
 * (required due to the UpstreamToDownstream interface, but will be renamed once
 * the classic mode is deprecated), and are finally passed to the router via the
 * RouterFilterInterface onUpstream[X] functions.
 *
 * There is some required communication between the UpstreamRequest and
 * UpstreamCodecFilter. This is accomplished via the UpstreamStreamFilterCallbacks
 * interface, with the UpstreamFilterManager acting as intermediary.
 *
 */
#define RP_TYPE_UPSTREAM_REQUEST rp_upstream_request_get_type()
G_DECLARE_FINAL_TYPE(RpUpstreamRequest, rp_upstream_request, RP, UPSTREAM_REQUEST, GObject)

RpUpstreamRequest* rp_upstream_request_new(RpRouterFilterInterface* parent, RpGenericConnPool* conn_pool, bool can_send_early_data, bool can_use_http3, bool enable_half_close);
void rp_upstream_request_reset_stream(RpUpstreamRequest* self);
void rp_upstream_request_accept_headers_from_router(RpUpstreamRequest* self,
                                                    bool end_stream);
void rp_upstream_request_accept_data_from_router(RpUpstreamRequest* self,
                                                    evbuf_t* data,
                                                    bool end_stream);
RpRouterFilterInterface* rp_upstream_request_parent_(RpUpstreamRequest* self);
bool rp_upstream_request_paused_for_connect_(RpUpstreamRequest* self);
void rp_upstream_request_set_paused_for_connect_(RpUpstreamRequest* self,
                                                    bool value);
bool rp_upstream_request_paused_for_websocket_upgrade_(RpUpstreamRequest* self);
void rp_upstream_request_set_paused_for_websocket_upgrade_(RpUpstreamRequest* self,
                                                            bool value);
RpGenericUpstream* rp_upstream_request_upstream_(RpUpstreamRequest* self);
GSList** rp_upstream_request_upstream_callbacks_(RpUpstreamRequest* self);
RpUpstreamToDownstream** rp_upstream_request_upstream_interface_(RpUpstreamRequest* self);
bool rp_upstream_request_enable_half_close_(RpUpstreamRequest* self);
void rp_upstream_request_accept_trailers_from_router(RpUpstreamRequest* self,
                                                        evhtp_headers_t* trailers);
RpStreamInfo* rp_upstream_request_stream_info(RpUpstreamRequest* self);
void rp_upstream_request_on_upstream_host_selected(RpUpstreamRequest* self,
                                                    RpHostDescription* host,
                                                    bool pool_success);
void rp_upstream_request_clear_request_encoder(RpUpstreamRequest* self);
bool rp_upstream_request_encode_complete(RpUpstreamRequest* self);
bool rp_upstream_request_awaiting_headers(RpUpstreamRequest* self);

G_END_DECLS
