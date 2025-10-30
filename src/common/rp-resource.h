/*
 * rp-resource.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * A handle for use by any resource managers.
 */
#define RP_TYPE_RESOURCE_LIMIT rp_resource_limit_get_type()
G_DECLARE_INTERFACE(RpResourceLimit, rp_resource_limit, RP, RESOURCE_LIMIT, GObject)

struct _RpResourceLimitInterface {
    GTypeInterface parent_iface;

    bool (*can_create)(RpResourceLimit*);
    void (*inc)(RpResourceLimit*);
    void (*dec)(RpResourceLimit*);
    void (*dec_by)(RpResourceLimit*, guint64);
    guint64 (*max)(RpResourceLimit*);
    guint64 (*count)(RpResourceLimit*);
};

static inline bool
rp_resource_limit_can_create(RpResourceLimit* self)
{
    return RP_IS_RESOURCE_LIMIT(self) ?
        RP_RESOURCE_LIMIT_GET_IFACE(self)->can_create(self) : false;
}
static inline void
rp_resource_limit_inc(RpResourceLimit* self)
{
    if (RP_IS_RESOURCE_LIMIT(self))
    {
        RP_RESOURCE_LIMIT_GET_IFACE(self)->inc(self);
    }
}
static inline void
rp_resource_limit_dec(RpResourceLimit* self)
{
    if (RP_IS_RESOURCE_LIMIT(self))
    {
        RP_RESOURCE_LIMIT_GET_IFACE(self)->dec(self);
    }
}
static inline void
rp_resource_limit_dec_by(RpResourceLimit* self, guint64 amount)
{
    if (RP_IS_RESOURCE_LIMIT(self))
    {
        RP_RESOURCE_LIMIT_GET_IFACE(self)->dec_by(self, amount);
    }
}
static inline guint64
rp_resource_limit_max(RpResourceLimit* self)
{
    return RP_IS_RESOURCE_LIMIT(self) ?
        RP_RESOURCE_LIMIT_GET_IFACE(self)->max(self) : 0;
}
static inline guint64
rp_resource_limit_count(RpResourceLimit* self)
{
    return RP_IS_RESOURCE_LIMIT(self) ?
        RP_RESOURCE_LIMIT_GET_IFACE(self)->count(self) : G_MAXINT64;
}

G_END_DECLS
