/*
 * rp-conn-info-setter-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-socket.h"

G_BEGIN_DECLS

#define RP_TYPE_CONNECTION_INFO_SETTER_IMPL rp_connection_info_setter_impl_get_type()
G_DECLARE_FINAL_TYPE(RpConnectionInfoSetterImpl, rp_connection_info_setter_impl, RP, CONNECTION_INFO_SETTER_IMPL, GObject)

RpConnectionInfoSetterImpl* rp_connection_info_setter_impl_new(struct sockaddr* local_address, struct sockaddr* remote_address);

G_END_DECLS
