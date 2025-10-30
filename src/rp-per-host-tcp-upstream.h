/*
 * rp-per-host-tcp-upstream.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-router.h"
#include "rp-tcp-conn-pool.h"
#include "upstream/rp-tcp-upstream.h"

G_BEGIN_DECLS

#define RP_TYPE_PER_HOST_TCP_UPSTREAM rp_per_host_tcp_upstream_get_type()
G_DECLARE_FINAL_TYPE(RpPerHostTcpUpstream, rp_per_host_tcp_upstream, RP, PER_HOST_TCP_UPSTREAM, RpTcpUpstream)

RpPerHostTcpUpstream* rp_per_host_tcp_upstream_new(RpUpstreamToDownstream* upstream_request,
                                                    RpTcpConnPoolConnectionData* upstream);

G_END_DECLS
