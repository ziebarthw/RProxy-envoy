/*
 * rp-transport-socket-config.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_transport_socket_config_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_transport_socket_config_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-transport-socket-config.h"

G_DEFINE_INTERFACE(RpTransportSocketFactoryContext, rp_transport_socket_factory_context, G_TYPE_OBJECT)
static void
rp_transport_socket_factory_context_default_init(RpTransportSocketFactoryContextInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
