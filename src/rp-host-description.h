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
typedef const SHARED_PTR(RpClusterInfo) RpClusterInfoConstSharedPtr;
typedef SHARED_PTR(RpClusterInfo) RpClusterInfoSharedPtr;
typedef gpointer RpMetadataConstSharedPtr;

/**
 * A description of an upstream host.
 */
#define RP_TYPE_HOST_DESCRIPTION rp_host_description_get_type()
G_DECLARE_INTERFACE(RpHostDescription, rp_host_description, RP, HOST_DESCRIPTION, GObject)

struct _RpHostDescriptionInterface {
    GTypeInterface parent_iface;

    //TODO...
    RpMetadataConstSharedPtr (*metadata)(const RpHostDescription*);
    void (*set_metadata)(RpHostDescription*, RpMetadataConstSharedPtr);
    RpClusterInfoConstSharedPtr (*cluster)(const RpHostDescription*);
    bool (*can_create_connection)(const RpHostDescription*, RpResourcePriority_e);
    //TODO...
    const char* (*hostname)(const RpHostDescription*);
    RpUpstreamTransportSocketFactory* (*transport_socket_factory)(const RpHostDescription*);
    //TODO...
    RpNetworkAddressInstanceConstSharedPtr (*address)(const RpHostDescription*);
    //TODO...
    guint32 (*priority)(const RpHostDescription*);
    void (*set_priority)(RpHostDescription*, guint32);
    //TODO...
};

typedef const SHARED_PTR(RpHostDescription) RpHostDescriptionConstSharedPtr;
typedef SHARED_PTR(RpHostDescription) RpHostDescriptionSharedPtr;

static inline gboolean
rp_host_description_is_a(RpHostDescriptionConstSharedPtr self)
{
    g_return_val_if_fail(self != NULL, FALSE);
    return RP_IS_HOST_DESCRIPTION((GObject*)self);
}
static inline RpHostDescriptionInterface*
rp_host_description_iface(RpHostDescriptionConstSharedPtr self)
{
    g_return_val_if_fail(rp_host_description_is_a(self), NULL);
    return RP_HOST_DESCRIPTION_GET_IFACE((GObject*)self);
}
static inline void
rp_host_description_set_object(RpHostDescriptionSharedPtr* dst, RpHostDescriptionConstSharedPtr src)
{
    g_return_if_fail(dst != NULL);
    g_set_object((GObject**)dst, (GObject*)src);
}
static inline RpMetadataConstSharedPtr
rp_host_description_metadata(RpHostDescriptionConstSharedPtr self)
{
    g_return_val_if_fail(rp_host_description_is_a(self), NULL);
    RpHostDescriptionInterface* iface = rp_host_description_iface(self);
    return iface->metadata ? iface->metadata(self) : NULL;
}
static inline void
rp_host_description_set_metadata(RpHostDescription* self, RpMetadataConstSharedPtr metadata)
{
    g_return_if_fail(rp_host_description_is_a(self));
    RpHostDescriptionInterface* iface = rp_host_description_iface(self);
    if (iface->set_metadata) \
        iface->set_metadata(self, metadata);
}
static inline RpClusterInfoConstSharedPtr
rp_host_description_cluster(RpHostDescriptionConstSharedPtr self)
{
    g_return_val_if_fail(rp_host_description_is_a(self), NULL);
    RpHostDescriptionInterface* iface = rp_host_description_iface(self);
    return iface->cluster ? iface->cluster(self) : NULL;
}
static inline bool
rp_host_description_can_create_connection(RpHostDescriptionConstSharedPtr self, RpResourcePriority_e priority)
{
    g_return_val_if_fail(rp_host_description_is_a(self), false);
    RpHostDescriptionInterface* iface = rp_host_description_iface(self);
    return iface->can_create_connection ?
        iface->can_create_connection(self, priority) : false;
}
static inline const char*
rp_host_description_hostname(RpHostDescriptionConstSharedPtr self)
{
    g_return_val_if_fail(rp_host_description_is_a(self), NULL);
    RpHostDescriptionInterface* iface = rp_host_description_iface(self);
    return iface->hostname ? iface->hostname(self) : NULL;
}
static inline RpUpstreamTransportSocketFactory*
rp_host_description_transport_socket_factory(RpHostDescriptionConstSharedPtr self)
{
    g_return_val_if_fail(rp_host_description_is_a(self), NULL);
    RpHostDescriptionInterface* iface = rp_host_description_iface(self);
    return iface->transport_socket_factory ?
        iface->transport_socket_factory(self) : NULL;
}
static inline RpNetworkAddressInstanceConstSharedPtr
rp_host_description_address(RpHostDescriptionConstSharedPtr self)
{
    g_return_val_if_fail(rp_host_description_is_a(self), NULL);
    RpHostDescriptionInterface* iface = rp_host_description_iface(self);
    return iface->address ? iface->address(self) : NULL;
}
static inline guint32
rp_host_description_priority(RpHostDescriptionConstSharedPtr self)
{
    g_return_val_if_fail(rp_host_description_is_a(self), 0);
    RpHostDescriptionInterface* iface = rp_host_description_iface(self);
    return iface->priority ? iface->priority(self) : 0;
}
static inline void
rp_host_description_set_priority(RpHostDescription* self, guint32 priority)
{
    g_return_if_fail(rp_host_description_is_a(self));
    RpHostDescriptionInterface* iface = rp_host_description_iface(self);
    if (iface->set_priority) \
        iface->set_priority(self, priority);
}

G_END_DECLS
