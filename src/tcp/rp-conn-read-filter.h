/*
 * rp-conn-read-filter.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "tcp/rp-active-tcp-client.h"
#include "rp-net-filter-impl.h"

G_BEGIN_DECLS

#define RP_TYPE_CONN_READ_FILTER rp_conn_read_filter_get_type()
G_DECLARE_FINAL_TYPE(RpConnReadFilter, rp_conn_read_filter, RP, CONN_READ_FILTER, RpNetworkReadFilterBaseImpl)

RpConnReadFilter* rp_conn_read_filter_new(RpActiveTcpClient* parent);

G_END_DECLS
