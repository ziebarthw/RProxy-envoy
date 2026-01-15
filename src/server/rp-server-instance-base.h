/*
 * rp-server-instance-base.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "server/rp-server-configuration-impl.h"
#include "rp-server-instance.h"

G_BEGIN_DECLS

/**
 * This is the base class for the standalone server which stitches together various common
 * components. Some components are optional (so PURE) and can be created or not by subclasses.
 */
#define RP_TYPE_SERVER_INSTANCE_BASE rp_server_instance_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpServerInstanceBase, rp_server_instance_base, RP, SERVER_INSTANCE_BASE, GObject)

struct _RpServerInstanceBaseClass {
    GObjectClass parent_class;

};

void rp_server_instance_base_initialize(RpServerInstanceBase* self);
RpServerConfigurationMainImpl* rp_server_instance_base_config(RpServerInstanceBase* self);

G_END_DECLS
