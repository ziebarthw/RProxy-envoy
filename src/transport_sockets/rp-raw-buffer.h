/*
 * rp-raw-buffer.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-transport-socket-config.h"

G_BEGIN_DECLS

#define RP_TYPE_UPSTREAM_RAW_BUFFER_SOCKET_FACTORY rp_upstream_raw_buffer_socket_factory_get_type()
G_DECLARE_FINAL_TYPE(RpUpstreamRawBufferSocketFactory, rp_upstream_raw_buffer_socket_factory, RP, UPSTREAM_RAW_BUFFER_SOCKET_FACTORY, GObject)

RpUpstreamRawBufferSocketFactory* rp_upstream_raw_buffer_socket_factory_new(void);


#define RP_TYPE_DOWNSTREAM_RAW_BUFFER_SOCKET_FACTORY rp_downstream_raw_buffer_socket_factory_get_type()
G_DECLARE_FINAL_TYPE(RpDownstreamRawBufferSocketFactory, rp_downstream_raw_buffer_socket_factory, RP, DOWNSTREAM_RAW_BUFFER_SOCKET_FACTORY, GObject)

RpDownstreamRawBufferSocketFactory* rp_downstream_raw_buffer_socket_factory_new(void);

G_END_DECLS
