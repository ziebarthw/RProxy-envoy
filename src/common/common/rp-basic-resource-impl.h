/*
 * rp-basic-resource-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-resource.h"

G_BEGIN_DECLS

/**
 * A handle to track some limited resource.
 *
 * NOTE:
 * This implementation makes some assumptions which favor simplicity over correctness. Though
 * atomics are used, it is possible for resources to temporarily go above the supplied maximums.
 * This should not effect overall behavior.
 */
#define RP_TYPE_BASIC_RESOURCE_LIMIT_IMPL rp_basic_resource_limit_impl_get_type()
G_DECLARE_DERIVABLE_TYPE(RpBasicResourceLimitImpl, rp_basic_resource_limit_impl, RP, BASIC_RESOURCE_LIMIT_IMPL, GObject)

struct _RpBasicResourceLimitImplClass {
    GObjectClass parent_class;


};

void rp_basic_resource_limit_impl_set_max(RpBasicResourceLimitImpl* self,
                                            guint64 new_max);

G_END_DECLS
