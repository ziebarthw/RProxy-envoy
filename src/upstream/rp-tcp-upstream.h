/*
 * rp-tcp-upstream.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define RP_TYPE_TCP_UPSTREAM rp_tcp_upstream_get_type()
G_DECLARE_DERIVABLE_TYPE(RpTcpUpstream, rp_tcp_upstream, RP, TCP_UPSTREAM, GObject)

struct _RpTcpUpstreamClass {
    GObjectClass parent_class;
    
};

G_END_DECLS
