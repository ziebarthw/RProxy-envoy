/*
 * rp-thread-local-object.h
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
 * All objects that are stored via the ThreadLocal interface must derive from this type.
 */
#define RP_TYPE_THREAD_LOCAL_OBJECT rp_thread_local_object_get_type()
G_DECLARE_INTERFACE(RpThreadLocalObject, rp_thread_local_object, RP, THREAD_LOCAL_OBJECT, GObject)

struct _RpThreadLocalObjectInterface {
    GTypeInterface parent_iface;

};

typedef SHARED_PTR(RpThreadLocalObject) RpThreadLocalObjectSharedPtr;

G_END_DECLS
