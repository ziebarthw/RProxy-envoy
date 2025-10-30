/*
 * rp-http-conn-mgr-impl-active-stream.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-codec.h"
#include "rp-downstream-filter-manager.h"

G_BEGIN_DECLS

typedef struct _RpHttpConnectionManagerImpl RpHttpConnectionManagerImpl;

#define RP_TYPE_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM rp_http_conn_mgr_impl_active_stream_get_type()
G_DECLARE_FINAL_TYPE(RpHttpConnMgrImplActiveStream, rp_http_conn_mgr_impl_active_stream, RP, HTTP_CONN_MGR_IMPL_ACTIVE_STREAM, GObject)

RpHttpConnMgrImplActiveStream* rp_http_conn_mgr_impl_active_stream_new(RpHttpConnectionManagerImpl* connection_manager,
                                                        guint32 buffer_limit);
RpResponseEncoder* rp_http_conn_mgr_impl_active_stream_response_encoder(RpHttpConnMgrImplActiveStream* self);
RpDownstreamFilterManager* rp_http_conn_mgr_impl_active_stream_filter_manager_(RpHttpConnMgrImplActiveStream* self);
bool rp_http_conn_mgr_impl_active_stream_get_codec_saw_local_complete(RpHttpConnMgrImplActiveStream* self);
void rp_http_conn_mgr_impl_active_stream_set_codec_saw_local_complete(RpHttpConnMgrImplActiveStream* self, bool v);
void rp_http_conn_mgr_impl_active_stream_set_is_internally_created(RpHttpConnMgrImplActiveStream* self, bool v);
void rp_http_conn_mgr_impl_active_stream_set_response_encoder(RpHttpConnMgrImplActiveStream* self,
                                                            RpResponseEncoder* response_encoder);
guint32 rp_http_conn_mgr_impl_active_stream_get_idle_timeout(RpHttpConnMgrImplActiveStream* self);
bool rp_http_conn_mgr_impl_active_stream_can_destroy_stream(RpHttpConnMgrImplActiveStream* self);
bool rp_http_conn_mgr_impl_active_stream_is_internally_destroyed(RpHttpConnMgrImplActiveStream* self);
void rp_http_conn_mgr_impl_active_stream_set_is_zombie_stream(RpHttpConnMgrImplActiveStream* self, bool v);
void rp_http_conn_mgr_impl_active_stream_complete_request(RpHttpConnMgrImplActiveStream* self);
evhtp_headers_t* rp_http_conn_mgr_impl_active_stream_response_headers_(RpHttpConnMgrImplActiveStream* self);
evhtp_headers_t* rp_http_conn_mgr_impl_active_stream_request_headers_(RpHttpConnMgrImplActiveStream* self);
RpHttpConnectionManagerImpl* rp_http_conn_mgr_impl_active_stream_get_connection_manager(RpHttpConnMgrImplActiveStream* self);
RpClusterManager* rp_http_conn_mgr_impl_active_stream_get_cluster_manager(RpHttpConnMgrImplActiveStream* self);

G_END_DECLS
