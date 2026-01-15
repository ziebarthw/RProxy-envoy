/*
 * rp-per-host-http-conn-pool.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-http-conn-pool.h"
#include "rp-thread-local-cluster.h"
#include "rp-upstream.h"
#include "upstream/rp-http-conn-pool.h"

G_BEGIN_DECLS

#define RP_TYPE_PER_HOST_HTTP_CONN_POOL rp_per_host_http_conn_pool_get_type()
G_DECLARE_FINAL_TYPE(RpPerHostHttpConnPool, rp_per_host_http_conn_pool, RP, PER_HOST_HTTP_CONN_POOL, RpHttpConnPool)

RpPerHostHttpConnPool* rp_per_host_http_conn_pool_new(RpHostConstSharedPtr host,
                                                        RpThreadLocalCluster* thread_local_cluster,
                                                        RpResourcePriority_e priority,
                                                        evhtp_proto downstream_protocol,
                                                        RpLoadBalancerContext* context);

G_END_DECLS
