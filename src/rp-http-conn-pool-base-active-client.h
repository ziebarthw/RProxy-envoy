/*
 * rp-http-conn-pool-base-active-client.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-http-conn-pool.h"
#include "common/conn_pool/rp-conn-pool-base-active-client.h"

G_BEGIN_DECLS

/*
 * An implementation of Envoy::ConnectionPool::ActiveClient for HTTP/1.1 and HTTP/2
 */
#define RP_TYPE_HTTP_CONN_POOL_BASE_ACTIVE_CLIENT rp_http_conn_pool_base_active_client_get_type()
G_DECLARE_DERIVABLE_TYPE(RpHttpConnPoolBaseActiveClient, rp_http_conn_pool_base_active_client, RP, HTTP_CONN_POOL_BASE_ACTIVE_CLIENT, RpConnectionPoolActiveClient)

struct _RpHttpConnPoolBaseActiveClientClass {
    RpConnectionPoolActiveClientClass parent_class;

    RpRequestEncoder* (*new_stream_encoder)(RpHttpConnPoolBaseActiveClient*, RpResponseDecoder*);
};

static inline RpRequestEncoder*
rp_http_conn_pool_base_active_client_new_stream_encoder(RpHttpConnPoolBaseActiveClient* self, RpResponseDecoder* decoder)
{
    return RP_IS_HTTP_CONN_POOL_BASE_ACTIVE_CLIENT(self) ?
        RP_HTTP_CONN_POOL_BASE_ACTIVE_CLIENT_GET_CLASS(self)->new_stream_encoder(self, decoder) :
        NULL;
}

RpCodecClient* rp_http_conn_pool_base_active_client_codec_client_(RpHttpConnPoolBaseActiveClient* self);

G_END_DECLS
