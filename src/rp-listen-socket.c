/*
 * rp-listen-socket.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_listen_socket_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_listen_socket_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <stdio.h>
#include "rp-listen-socket.h"

G_DEFINE_INTERFACE(RpConnectionSocket, rp_connection_socket, G_TYPE_OBJECT)
static void
rp_connection_socket_default_init(RpConnectionSocketInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
