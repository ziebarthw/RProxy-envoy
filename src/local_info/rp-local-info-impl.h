/*
 * rp-local-info-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-local-info.h"

G_BEGIN_DECLS

#define RP_TYPE_LOCAL_INFO_IMPL rp_local_info_impl_get_type()
G_DECLARE_FINAL_TYPE(RpLocalInfoImpl, rp_local_info_impl, RP, LOCAL_INFO_IMPL, GObject)

RpLocalInfoImpl* rp_local_info_impl_new(void);

G_END_DECLS
