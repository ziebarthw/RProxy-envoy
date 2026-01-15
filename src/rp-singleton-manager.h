/*
 * rp-singleton-manager.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-singleton-instance.h"

G_BEGIN_DECLS

#define SINGLETON_MANAGER_REGISTRATION(NAME) \
    static const char NAME##_singleton_name[] = #NAME "_singleton";

#define SINGLETON_MANAGER_REGISTERED_NAME(NAME) NAME##_singleton_name

/**
 * Callback function used to create a singleton.
 */
typedef RpSingletonInstanceSharedPtr (*RpSingletonFactoryCb)(void);

/**
 * A manager for all server-side singletons.
 */
#define RP_TYPE_SINGLETON_MANAGER rp_singleton_manager_get_type()
G_DECLARE_INTERFACE(RpSingletonManager, rp_singleton_manager, RP, SINGLETON_MANAGER, GObject)

struct _RpSingletonManagerInterface {
    GTypeInterface parent_iface;

    RpSingletonInstanceSharedPtr (*get)(RpSingletonManager*,
                                        const char*,
                                        RpSingletonFactoryCb,
                                        bool);
};

typedef UNIQUE_PTR(RpSingletonManager) RpSingletonManagerPtr;

static inline RpSingletonInstanceSharedPtr
rp_singleton_manager_get(RpSingletonManager* self, const char* name, RpSingletonFactoryCb cb, bool pin)
{
    return RP_IS_SINGLETON_MANAGER(self) ?
        RP_SINGLETON_MANAGER_GET_IFACE(self)->get(self, name, cb, pin) : NULL;
}

G_END_DECLS
