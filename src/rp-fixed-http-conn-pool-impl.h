/*
 * rp-fixed-http-conn-pool-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-dispatcher.h"
#include "rp-http-conn-pool-base.h"

G_BEGIN_DECLS

typedef RpCodecClient* RpCodecClientPtr;
typedef RpConnectionPoolActiveClientPtr (*RpCreateClientFn)(RpHttpConnPoolImplBase*);
typedef RpCodecClientPtr (*RpCreateCodecFn)(RpCreateConnectionDataPtr, RpHttpConnPoolImplBase*);

/* An implementation of Envoy::ConnectionPool::ConnPoolImplBase for HTTP/1 and HTTP/2
 */
#define RP_TYPE_FIXED_HTTP_CONN_POOL_IMPL rp_fixed_http_conn_pool_impl_get_type()
G_DECLARE_FINAL_TYPE(RpFixedHttpConnPoolImpl, rp_fixed_http_conn_pool_impl, RP, FIXED_HTTP_CONN_POOL_IMPL, RpHttpConnPoolImplBase)

RpFixedHttpConnPoolImpl* rp_fixed_http_conn_pool_impl_new(RpHost* host,
                                                            RpResourcePriority_e priority,
                                                            RpDispatcher* dispatcher,
                                                            RpCreateClientFn client_fn,
                                                            RpCreateCodecFn codec_fn,
                                                            evhtp_proto* protocols,
                                                            void* origin);
void rp_fixed_http_conn_pool_impl_set_origin(RpFixedHttpConnPoolImpl* self,
                                                void* origin);

G_END_DECLS
