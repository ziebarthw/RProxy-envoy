/*
 * rp-http-conn-manager-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-cluster-manager.h"
#include "rp-dispatcher.h"
#include "rp-http-conn-manager-config.h"
#include "rp-net-filter.h"
#include "rp-http-conn-mgr-impl-active-stream.h"

G_BEGIN_DECLS

typedef enum {
    RpDrainState_NotDraining,
    RpDrainState_Draining,
    RpDrainState_Closing
} RpDrainState_e;

/**
 * Implementation of both ConnectionManager and ServerConnectionCallbacks. This is a
 * Network::Filter that can be installed on a connection that will perform HTTP protocol agnostic
 * handling of a connection and all requests/pushes that occur on a connection.
 */
#define RP_TYPE_HTTP_CONNECTION_MANAGER_IMPL rp_http_connection_manager_impl_get_type()
G_DECLARE_FINAL_TYPE(RpHttpConnectionManagerImpl, rp_http_connection_manager_impl, RP, HTTP_CONNECTION_MANAGER_IMPL, GObject)

RpHttpConnectionManagerImpl* rp_http_connection_manager_impl_new(RpConnectionManagerConfig* config,
                                                                    RpLocalInfo* local_info,
                                                                    RpClusterManager* cluster_manager);
void rp_http_connection_manager_impl_do_end_stream(RpHttpConnectionManagerImpl* self,
                                                RpHttpConnMgrImplActiveStream* stream,
                                                bool check_for_deferred_close);
evhtp_proto rp_http_connection_manager_impl_get_protocol(RpHttpConnectionManagerImpl* self);
RpDrainState_e rp_http_connection_manager_impl_get_drain_state(RpHttpConnectionManagerImpl* self);
void rp_http_connection_manager_impl_set_drain_state(RpHttpConnectionManagerImpl* self,
                                                        RpDrainState_e state);
RpConnectionManagerConfig* rp_http_connection_manager_impl_connection_manager_config_(RpHttpConnectionManagerImpl* self);
bool rp_http_connection_manager_impl_should_defer_request_proxying_to_next_io_cycle(RpHttpConnectionManagerImpl* self);
bool rp_http_connection_manager_impl_should_keep_alive_(RpHttpConnectionManagerImpl* self);
RpNetworkReadFilterCallbacks* rp_http_connection_manager_impl_read_callbacks_(RpHttpConnectionManagerImpl* self);
void rp_http_connection_manager_impl_create_codec(RpHttpConnectionManagerImpl* self,
                                                    evbuf_t* data);
RpNetworkConnection* rp_http_connection_manager_impl_connection_(RpHttpConnectionManagerImpl* self);
RpDispatcher* rp_http_connection_manager_impl_dispatcher_(RpHttpConnectionManagerImpl* self);
RpHttpServerConnection* rp_http_connection_manager_impl_codec_(RpHttpConnectionManagerImpl* self);
RpClusterManager* rp_http_connection_manager_impl_cluster_manager_(RpHttpConnectionManagerImpl* self);
RpLocalInfo* rp_http_connection_manager_impl_local_info_(RpHttpConnectionManagerImpl* self);
void rp_http_connection_manager_impl_do_deferred_stream_destroy(RpHttpConnectionManagerImpl* self,
                                                                RpHttpConnMgrImplActiveStream* stream);

G_END_DECLS
