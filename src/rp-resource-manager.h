/*
 * rp-resource-manager.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "common/rp-resource.h"

G_BEGIN_DECLS

/**
 * Resource priority classes. The parallel NumResourcePriorities constant allows defining fixed
 * arrays for each priority, but does not pollute the enum.
 */
typedef enum {
    RpResourcePriority_Default,
    RpResourcePriority_High
} RpResourcePriority_e;


/**
 * Global resource manager that loosely synchronizes maximum connections, pending requests, etc.
 * NOTE: Currently this is used on a per cluster basis. In the future we may consider also chaining
 *       this with a global resource manager.
 */
#define RP_TYPE_RESOURCE_MANAGER rp_resource_manager_get_type()
G_DECLARE_INTERFACE(RpResourceManager, rp_resource_manager, RP, RESOURCE_MANAGER, GObject)

struct _RpResourceManagerInterface {
    GTypeInterface parent_iface;

    RpResourceLimit* (*connections)(RpResourceManager*);
    RpResourceLimit* (*pending_requests)(RpResourceManager*);
    RpResourceLimit* (*requests)(RpResourceManager*);
    RpResourceLimit* (*retries)(RpResourceManager*);
    RpResourceLimit* (*connection_pools)(RpResourceManager*);
    guint64 (*max_connections_pre_host)(RpResourceManager*);
};

static inline RpResourceLimit*
rp_resource_manager_connections(RpResourceManager* self)
{
    return RP_IS_RESOURCE_MANAGER(self) ?
        RP_RESOURCE_MANAGER_GET_IFACE(self)->connections(self) : NULL;
}
static inline RpResourceLimit*
rp_resource_manager_pending_requests(RpResourceManager* self)
{
    return RP_IS_RESOURCE_MANAGER(self) ?
        RP_RESOURCE_MANAGER_GET_IFACE(self)->pending_requests(self) : NULL;
}
static inline RpResourceLimit*
rp_resource_manager_requests(RpResourceManager* self)
{
    return RP_IS_RESOURCE_MANAGER(self) ?
        RP_RESOURCE_MANAGER_GET_IFACE(self)->requests(self) : NULL;
}
static inline RpResourceLimit*
rp_resource_manager_retries(RpResourceManager* self)
{
    return RP_IS_RESOURCE_MANAGER(self) ?
        RP_RESOURCE_MANAGER_GET_IFACE(self)->retries(self) : NULL;
}
static inline RpResourceLimit*
rp_resource_manager_connection_pools(RpResourceManager* self)
{
    return RP_IS_RESOURCE_MANAGER(self) ?
        RP_RESOURCE_MANAGER_GET_IFACE(self)->connection_pools(self) : NULL;
}
static inline guint64
rp_resource_manager_max_connections_pre_host(RpResourceManager* self)
{
    return RP_IS_RESOURCE_MANAGER(self) ?
        RP_RESOURCE_MANAGER_GET_IFACE(self)->max_connections_pre_host(self) : 0;
}

G_END_DECLS
