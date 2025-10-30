/*
 * rp-instance-base.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-instance.h"

G_BEGIN_DECLS

/**
 * This is the base class for the standalone server which stitches together various common
 * components. Some components are optional (so PURE) and can be created or not by subclasses.
 */
#define RP_TYPE_INSTANCE_BASE rp_instance_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpInstanceBase, rp_instance_base, RP, INSTANCE_BASE, GObject)

struct _RpInstanceBaseClass {
    GObjectClass parent_class;

};

void rp_instance_base_initialize(RpInstanceBase* self);

G_END_DECLS
