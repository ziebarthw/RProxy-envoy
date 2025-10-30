/*
 * rp-managed-resource-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "common/rp-basic-resource-impl.h"

G_BEGIN_DECLS

#define RP_TYPE_MANAGED_RESOURCE_IMPL rp_managed_resource_impl_get_type()
G_DECLARE_FINAL_TYPE(RpManagedResourceImpl, rp_managed_resource_impl, RP, MANAGED_RESOURCE_IMPL, RpBasicResourceLimitImpl)

RpManagedResourceImpl* rp_manage_resource_impl_new(guint64 max,
                                                    const char* runtime_key);

G_END_DECLS
