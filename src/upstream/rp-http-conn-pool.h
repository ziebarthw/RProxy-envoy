/*
 * rp-http-conn-pool.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-router.h"

G_BEGIN_DECLS

#define RP_TYPE_HTTP_CONN_POOL rp_http_conn_pool_get_type()
G_DECLARE_DERIVABLE_TYPE(RpHttpConnPool, rp_http_conn_pool, RP, HTTP_CONN_POOL, GObject)

struct _RpHttpConnPoolClass {
    GObjectClass parent_class;

};

RpCancellable** rp_http_conn_pool_conn_pool_stream_handle_(RpHttpConnPool* self);
RpGenericConnectionPoolCallbacks* rp_http_conn_pool_callbacks_(RpHttpConnPool* self);

G_END_DECLS
