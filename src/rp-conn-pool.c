/*
 * rp-conn-pool.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_conn_pool_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_conn_pool_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-conn-pool.h"

G_DEFINE_INTERFACE(RpCancellable, rp_cancellable, G_TYPE_OBJECT)
static void
rp_cancellable_default_init(RpCancellableInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}


G_DEFINE_INTERFACE(RpConnectionPoolInstance, rp_connection_pool_instance, G_TYPE_OBJECT)
static void
rp_connection_pool_instance_default_init(RpConnectionPoolInstanceInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
