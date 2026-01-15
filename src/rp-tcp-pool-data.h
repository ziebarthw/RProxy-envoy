/*
 * rp-tcp-pool-data.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-tcp-conn-pool.h"
#include "rp-tcp-pool-data.h"

G_BEGIN_DECLS

typedef void (*RpOnNewConnectionFn)(RpTcpConnPoolInstancePtr, gpointer);

/*
 * Tcp pool returns information about a given pool, as well as a function to
 * create connections on that pool.
 */
#define RP_TYPE_TCP_POOL_DATA rp_tcp_pool_data_get_type()
G_DECLARE_FINAL_TYPE(RpTcpPoolData, rp_tcp_pool_data, RP, TCP_POOL_DATA, GObject)

RpTcpPoolData* rp_tcp_pool_data_new(RpOnNewConnectionFn on_new_connection,
                                    gpointer arg,
                                    RpTcpConnPoolInstance* pool);
RpCancellable* rp_tcp_pool_data_new_connection(RpTcpPoolData* self,
                                                RpTcpConnPoolCallbacks* callbacks);
RpHostDescription* rp_tcp_pool_data_host(RpTcpPoolData* self);

G_END_DECLS
