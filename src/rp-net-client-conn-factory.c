/*
 * rp-net-client-conn-factory.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_net_client_conn_factory_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_net_client_conn_factory_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-net-client-conn-factory.h"

G_DEFINE_INTERFACE(RpClientConnectionFactory, rp_client_connection_factory, G_TYPE_OBJECT)
static void
rp_client_connection_factory_default_init(RpClientConnectionFactoryInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
