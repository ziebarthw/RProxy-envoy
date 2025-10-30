/*
 * rp-local-info.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * Information about the local environment.
 */
#define RP_TYPE_LOCAL_INFO rp_local_info_get_type()
G_DECLARE_INTERFACE(RpLocalInfo, rp_local_info, RP, LOCAL_INFO, GObject)

struct _RpLocalInfoInterface {
    GTypeInterface parent_iface;

    struct sockaddr* (*address)(RpLocalInfo*);
    const char* (*zone_name)(RpLocalInfo*);
    //TODO...
    const char* (*cluster_name)(RpLocalInfo*);
    const char* (*node_name)(RpLocalInfo*);
    //TODO...
};

static inline struct sockaddr*
rp_local_info_address(RpLocalInfo* self)
{
    return RP_IS_LOCAL_INFO(self) ?
        RP_LOCAL_INFO_GET_IFACE(self)->address(self) : NULL;
}
static inline const char*
rp_local_info_zone_name(RpLocalInfo* self)
{
    return RP_IS_LOCAL_INFO(self) ?
        RP_LOCAL_INFO_GET_IFACE(self)->zone_name(self) : NULL;
}
static inline const char*
rp_local_info_cluster_name(RpLocalInfo* self)
{
    return RP_IS_LOCAL_INFO(self) ?
        RP_LOCAL_INFO_GET_IFACE(self)->cluster_name(self) : NULL;
}
static inline const char*
rp_local_info_node_name(RpLocalInfo* self)
{
    return RP_IS_LOCAL_INFO(self) ?
        RP_LOCAL_INFO_GET_IFACE(self)->node_name(self) : NULL;
}

G_END_DECLS
