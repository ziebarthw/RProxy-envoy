/*
 * rp-socket.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_socket_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_socket_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <stdio.h>
#include "rp-socket.h"

G_DEFINE_INTERFACE(RpSocket, rp_socket, G_TYPE_OBJECT)
static void
rp_socket_default_init(RpSocketInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpConnectionInfoProvider, rp_connection_info_provider, G_TYPE_OBJECT)
static void
rp_connection_info_provider_default_init(RpConnectionInfoProviderInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpConnectionInfoSetter, rp_connection_info_setter, RP_TYPE_CONNECTION_INFO_PROVIDER)
static void
rp_connection_info_setter_default_init(RpConnectionInfoSetterInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpSslConnectionInfo, rp_ssl_connection_info, G_TYPE_OBJECT)
static void
rp_ssl_connection_info_default_init(RpSslConnectionInfoInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
