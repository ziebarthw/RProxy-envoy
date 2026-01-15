/*
 * rp-http-conn-pool-base.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-codec-client.h"
#include "rp-http-conn-pool.h"
#include "common/conn_pool/rp-conn-pool-base.h"

G_BEGIN_DECLS

typedef struct _RpConnectionPoolActiveClient* RpConnectionPoolActiveClientPtr;
typedef struct _RpCreateConnectionData* RpCreateConnectionDataPtr;
typedef struct _RpHttpAttachContext* RpHttpAttachContextPtr;
typedef struct _RpHttpAttachContext RpHttpAttachContext;

struct _RpHttpAttachContext {
    struct _RpConnectionPoolAttachContext s;
    RpResponseDecoder* m_decoder;
    RpHttpConnPoolCallbacks* m_callbacks;
};

static inline RpHttpAttachContext
rp_http_attach_context_ctor(RpResponseDecoder* decoder, RpHttpConnPoolCallbacks* callbacks)
{
    RpHttpAttachContext self = {
        .s = rp_connection_pool_attach_context_ctor(NULL),
        .m_decoder = decoder,
        .m_callbacks = callbacks
    };
    return self;
}

/* An implementation of Envoy::ConnectionPool::ConnPoolImplBase for shared code
 * between HTTP/1.1 and HTTP/2
 *
 * NOTE: The connection pool does NOT do DNS resolution. It assumes it is being given a numeric IP
 *       address. Higher layer code should handle resolving DNS on error and creating a new pool
 *       bound to a different IP address.
 */
#define RP_TYPE_HTTP_CONN_POOL_IMPL_BASE rp_http_conn_pool_impl_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpHttpConnPoolImplBase, rp_http_conn_pool_impl_base, RP, HTTP_CONN_POOL_IMPL_BASE, RpConnPoolImplBase)

struct _RpHttpConnPoolImplBaseClass {
    RpConnPoolImplBaseClass parent_class;

    RpCodecClient* (*create_codec_client)(RpHttpConnPoolImplBase*, RpCreateConnectionDataPtr);
};

static inline RpCodecClient*
rp_http_conn_pool_impl_base_create_codec_client(RpHttpConnPoolImplBase* self, RpCreateConnectionDataPtr data)
{
    return RP_IS_HTTP_CONN_POOL_IMPL_BASE(self) ?
        RP_HTTP_CONN_POOL_IMPL_BASE_GET_CLASS(self)->create_codec_client(self, data) :
        NULL;
}

G_END_DECLS
