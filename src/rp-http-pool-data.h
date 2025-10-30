/*
 * rp-http-pool-data.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-http-conn-pool.h"

G_BEGIN_DECLS

typedef void (*RpOnNewStreamFn)(void*);

/*
 * HttpPoolData returns information about a given pool as well as a function
 * to create streams on that pool.
 */
#define RP_TYPE_HTTP_POOL_DATA rp_http_pool_data_get_type()
G_DECLARE_FINAL_TYPE(RpHttpPoolData, rp_http_pool_data, RP, HTTP_POOL_DATA, GObject)

RpHttpPoolData* rp_http_pool_data_new(RpOnNewStreamFn on_new_stream,
                                        void* arg,
                                        RpHttpConnectionPoolInstance* pool);
RpCancellable* rp_http_pool_data_new_stream(RpHttpPoolData* self,
                                            RpResponseDecoder* response_decoder,
                                            RpHttpConnPoolCallbacks* callbacks,
                                            RpHttpConnPoolInstStreamOptionsPtr stream_options);
bool rp_http_pool_data_has_active_connections(RpHttpPoolData* self);
void rp_http_pool_data_add_idle_callbacks(RpHttpPoolData* self, idle_cb cb);
void rp_http_pool_data_drain_connections(RpHttpPoolData* self,
                                            RpDrainBehavior_e drain_behavior);
RpHostDescription* rp_http_pool_data_host(RpHttpPoolData* self);

G_END_DECLS
