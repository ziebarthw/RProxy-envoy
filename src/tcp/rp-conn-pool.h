/*
 * rp-conn-pool.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "common/conn_pool/rp-conn-pool-base-active-client.h"
#include "common/conn_pool/rp-pending-stream.h"
#include "rp-net-connection.h"
#include "rp-net-filter.h"
#include "rp-tcp-conn-pool.h"
#include "rp-upstream.h"
#include "rp-http-conn-pool-base.h"

G_BEGIN_DECLS

typedef struct _RpTcpConnPoolImpl RpTcpConnPoolImpl;

typedef struct _RpTcpAttachContext* RpTcpAttachContextPtr;
typedef struct _RpTcpAttachContext RpTcpAttachContext;
struct _RpTcpAttachContext {
    RpConnectionPoolAttachContext s;
    RpTcpConnPoolCallbacks* m_callbacks;
};

static inline RpTcpAttachContext
rp_tcp_attach_context_ctor(RpTcpConnPoolCallbacks* callbacks)
{
    RpTcpAttachContext self = {
        .s = rp_connection_pool_attach_context_ctor(NULL),
        .m_callbacks = callbacks
    };
    return self;
}


#define RP_TYPE_TCP_PENDING_STREAM rp_tcp_pending_stream_get_type()
G_DECLARE_FINAL_TYPE(RpTcpPendingStream, rp_tcp_pending_stream, RP, TCP_PENDING_STREAM, RpPendingStream)

RpTcpPendingStream* rp_tcp_pending_stream_new(RpConnPoolImplBase* parent,
                                                bool can_send_early_data,
                                                RpTcpAttachContext* context);


#define RP_TYPE_ACTIVE_TCP_CLIENT rp_active_tcp_client_get_type()
G_DECLARE_FINAL_TYPE(RpActiveTcpClient, rp_active_tcp_client, RP, ACTIVE_TCP_CLIENT, RpConnectionPoolActiveClient)

RpActiveTcpClient* rp_active_tcp_client_new(RpConnPoolImplBase* parent, RpHost* host, guint64 concurrent_stream_limit/*TODO...idle_timeout*/);


#define RP_TYPE_TCP_CONN_POOL_IMPL rp_tcp_conn_pool_impl_get_type()
G_DECLARE_FINAL_TYPE(RpTcpConnPoolImpl, rp_tcp_conn_pool_impl, RP, TCP_CONN_POOL_IMPL, RpConnPoolImplBase)

RpTcpConnPoolImpl* rp_tcp_conn_pool_impl_new(RpDispatcher* dispatcher, RpHost* host, RpResourcePriority_e priority);

G_END_DECLS
