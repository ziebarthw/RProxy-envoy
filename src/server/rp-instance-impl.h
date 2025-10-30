/*
 * rp-instance-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-instance.h"
#include "server/rp-instance-base.h"

G_BEGIN_DECLS

/*
 * The production server instance, which creates all of the required components.
 */
#define RP_TYPE_INSTANCE_IMPL rp_instance_impl_get_type()
G_DECLARE_FINAL_TYPE(RpInstanceImpl, rp_instance_impl, RP, INSTANCE_IMPL, RpInstanceBase)

RpInstanceImpl* rp_instance_impl_new(const rproxy_hooks_t* hooks);

G_END_DECLS
