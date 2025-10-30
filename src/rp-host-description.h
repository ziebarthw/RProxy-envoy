/*
 * rp-host-description.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-resource-manager.h"
#include "rp-net-transport-socket.h"

G_BEGIN_DECLS

typedef struct _RpClusterInfo RpClusterInfo;

/**
 * A description of an upstream host.
 */
#define RP_TYPE_HOST_DESCRIPTION rp_host_description_get_type()
G_DECLARE_INTERFACE(RpHostDescription, rp_host_description, RP, HOST_DESCRIPTION, GObject)

struct _RpHostDescriptionInterface {
    GTypeInterface parent_iface;

    //TODO...
    RpClusterInfo* (*cluster)(RpHostDescription*);
    bool (*can_create_connection)(RpHostDescription*, RpResourcePriority_e);
    //TODO...
    const char* (*hostname)(RpHostDescription*);
    RpUpstreamTransportSocketFactory* (*transport_socket_factory)(RpHostDescription*);
    //TODO...
    struct sockaddr* (*address)(RpHostDescription*);
    //TODO...
    guint32 (*priority)(RpHostDescription*);
    void (*set_priority)(RpHostDescription*, guint32);
    //TODO...
};

static inline RpClusterInfo*
rp_host_description_cluster(RpHostDescription* self)
{
    return RP_IS_HOST_DESCRIPTION(self) ?
        RP_HOST_DESCRIPTION_GET_IFACE(self)->cluster(self) : NULL;
}
static inline bool
rp_host_description_can_create_connection(RpHostDescription* self, RpResourcePriority_e priority)
{
    return RP_IS_HOST_DESCRIPTION(self) ?
        RP_HOST_DESCRIPTION_GET_IFACE(self)->can_create_connection(self, priority) :
        false;
}
static inline const char*
rp_host_description_hostname(RpHostDescription* self)
{
    return RP_IS_HOST_DESCRIPTION(self) ?
        RP_HOST_DESCRIPTION_GET_IFACE(self)->hostname(self) : NULL;
}
static inline RpUpstreamTransportSocketFactory*
rp_host_description_transport_socket_factory(RpHostDescription* self)
{
    return RP_IS_HOST_DESCRIPTION(self) ?
        RP_HOST_DESCRIPTION_GET_IFACE(self)->transport_socket_factory(self) :
        NULL;
}
static inline struct sockaddr*
rp_host_description_address(RpHostDescription* self)
{
    return RP_IS_HOST_DESCRIPTION(self) ?
        RP_HOST_DESCRIPTION_GET_IFACE(self)->address(self) : NULL;
}
static inline guint32
rp_host_description_priority(RpHostDescription* self)
{
    return RP_IS_HOST_DESCRIPTION(self) ?
        RP_HOST_DESCRIPTION_GET_IFACE(self)->priority(self) : 0;
}
static inline void
rp_host_description_set_priority(RpHostDescription* self, guint32 priority)
{
    if (RP_IS_HOST_DESCRIPTION(self))
    {
        RP_HOST_DESCRIPTION_GET_IFACE(self)->set_priority(self, priority);
    }
}

G_END_DECLS
