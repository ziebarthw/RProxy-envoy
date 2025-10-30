/*
 * rp-host-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-net-transport-socket.h"
#include "upstream/rp-host-description-impl.h"

G_BEGIN_DECLS

/**
 * Implementation of Upstream::Host.
 */
#define RP_TYPE_HOST_IMPL rp_host_impl_get_type()
G_DECLARE_FINAL_TYPE(RpHostImpl, rp_host_impl, RP, HOST_IMPL, RpHostDescriptionImpl)

RpHostImpl* rp_host_impl_new(RpClusterInfo* cluster,
                                RpUpstreamTransportSocketFactory* socket_factory,
                                const char* hostname,
                                struct sockaddr* address,
                                guint32 initial_weight,
                                bool disable_health_check);

G_END_DECLS
