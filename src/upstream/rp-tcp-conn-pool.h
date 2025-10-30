/*
 * rp-tcp-conn-pool.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-router.h"

G_BEGIN_DECLS

#define RP_TYPE_TCP_CONN_POOL rp_tcp_conn_pool_get_type()
G_DECLARE_DERIVABLE_TYPE(RpTcpConnPool, rp_tcp_conn_pool, RP, TCP_CONN_POOL, GObject)

struct _RpTcpConnPoolClass {
    GObjectClass parent_class;

};

RpCancellable** rp_tcp_conn_pool_conn_pool_upstream_handle_(RpTcpConnPool* self);
RpGenericConnectionPoolCallbacks* rp_tcp_conn_pool_callbacks_(RpTcpConnPool* self);

G_END_DECLS
