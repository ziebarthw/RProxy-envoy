/*
 * rp-http1-conn-pool.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-dispatcher.h"
#include "rp-http-conn-pool-base.h"
#include "rp-http-conn-pool-base-active-client.h"

G_BEGIN_DECLS

/**
 * An active client for HTTP/1.1 connections.
 */
#define RP_TYPE_HTTP1_CP_ACTIVE_CLIENT rp_http1_cp_active_client_get_type()
G_DECLARE_FINAL_TYPE(RpHttp1CpActiveClient, rp_http1_cp_active_client, RP, HTTP1_CP_ACTIVE_CLIENT, RpHttpConnPoolBaseActiveClient)

RpHttp1CpActiveClient* rp_http1_cp_active_client_new(RpHttpConnPoolImplBase* parent,
                                                        RpCreateConnectionDataPtr data);
RpHttpConnPoolImplBase* rp_http1_cp_active_client_parent(RpHttp1CpActiveClient* self);
void rp_http1_cp_active_client_stream_wrapper_reset(RpHttp1CpActiveClient* self);

RpHttpConnectionPoolInstancePtr http1_allocate_conn_pool(RpDispatcher* dispatcher,
                                                            RpHost* host,
                                                            RpResourcePriority_e priority);

G_END_DECLS
