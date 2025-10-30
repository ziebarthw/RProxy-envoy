/*
 * rp-tcp-conn-pool.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_http_conn_pool_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_http_conn_pool_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-tcp-conn-pool.h"

G_DEFINE_INTERFACE(RpTcpConnPoolUpstreamCallbacks, rp_tcp_conn_pool_upstream_callbacks, G_TYPE_OBJECT)
static void
rp_tcp_conn_pool_upstream_callbacks_default_init(RpTcpConnPoolUpstreamCallbacksInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpTcpConnPoolConnectionData, rp_tcp_conn_pool_connection_data, G_TYPE_OBJECT)
static void
rp_tcp_conn_pool_connection_data_default_init(RpTcpConnPoolConnectionDataInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpTcpConnPoolCallbacks, rp_tcp_conn_pool_callbacks, G_TYPE_OBJECT)
static void
rp_tcp_conn_pool_callbacks_default_init(RpTcpConnPoolCallbacksInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpTcpConnPoolInstance, rp_tcp_conn_pool_instance, RP_TYPE_CONNECTION_POOL_INSTANCE)
static void
rp_tcp_conn_pool_instance_default_init(RpTcpConnPoolInstanceInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
