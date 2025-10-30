/*
 * rp-http-conn-pool.c
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

#include "rp-http-conn-pool.h"

G_DEFINE_INTERFACE(RpHttpConnPoolCallbacks, rp_http_conn_pool_callbacks, G_TYPE_OBJECT)
static void
rp_http_conn_pool_callbacks_default_init(RpHttpConnPoolCallbacksInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpHttpConnectionPoolInstance, rp_http_connection_pool_instance, RP_TYPE_CONNECTION_POOL_INSTANCE)
static void
rp_http_connection_pool_instance_default_init(RpHttpConnectionPoolInstanceInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpHttpConnectionLifetimeCallbacks, rp_http_connection_lifetime_callbacks, G_TYPE_OBJECT)
static void
rp_http_connection_lifetime_callbacks_default_init(RpHttpConnectionLifetimeCallbacksInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
