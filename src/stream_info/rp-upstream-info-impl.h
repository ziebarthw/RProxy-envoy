/*
 * template.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-stream-info.h"

G_BEGIN_DECLS

#define RP_TYPE_UPSTREAM_INFO_IMPL rp_upstream_info_impl_get_type()
G_DECLARE_FINAL_TYPE(RpUpstreamInfoImpl, rp_upstream_info_impl, RP, UPSTREAM_INFO_IMPL, GObject)

RpUpstreamInfoImpl* rp_upstream_info_impl_new(void);

G_END_DECLS
