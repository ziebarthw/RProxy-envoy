/*
 * rp-singleton-instance.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"

G_BEGIN_DECLS

/**
 * All singletons must derive from this type.
 */
#define RP_TYPE_SINGLETON_INSTANCE rp_singleton_instance_get_type()
G_DECLARE_INTERFACE(RpSingletonInstance, rp_singleton_instance, RP, SINGLETON_INSTANCE, GObject)

struct _RpSingletonInstanceInterface {
    GTypeInterface parent_iface;

};

typedef SHARED_PTR(RpSingletonInstance) RpSingletonInstanceSharedPtr;

G_END_DECLS
