/*
 * rp-instance.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-cluster-manager.h"
#include "rp-factory-context.h"
#include "rp-local-info.h"
#include "rp-transport-socket-config.h"

G_BEGIN_DECLS

/**
 * An instance of the running server.
 */
#define RP_TYPE_INSTANCE rp_instance_get_type()
G_DECLARE_INTERFACE(RpInstance, rp_instance, RP, INSTANCE, GObject)

struct _RpInstanceInterface {
    GTypeInterface parent_iface;

    void (*run)(RpInstance*);
    RpClusterManager* (*cluster_manager)(RpInstance*);
    void (*shutdown)(RpInstance*);
    bool (*is_shutdown)(RpInstance*);
//TODO...Http::Context& httpContext();
//TODO...Router::Context& routerContext();
//TODO...ProcessContextOptRef processContext();
    RpLocalInfo* (*local_info)(RpInstance*);
//TODO...Regex::Engine& regexEngine();
    RpServerFactoryContext* (*server_factory_context)(RpInstance*);
    RpTransportSocketFactoryContext* (*transport_socket_factory_context)(RpInstance*);
    bool (*enable_reuse_port_default)(RpInstance*);
    const rproxy_hooks_t* (*hooks)(RpInstance*);
};

static inline void
rp_instance_run(RpInstance* self)
{
    if (RP_IS_INSTANCE(self)) \
        RP_INSTANCE_GET_IFACE(self)->run(self);
}
static inline RpClusterManager*
rp_instance_cluster_manager(RpInstance* self)
{
    return RP_IS_INSTANCE(self) ?
        RP_INSTANCE_GET_IFACE(self)->cluster_manager(self) : NULL;
}
static inline void
rp_instance_shutdown(RpInstance* self)
{
    if (RP_IS_INSTANCE(self)) \
        RP_INSTANCE_GET_IFACE(self)->shutdown(self);
}
static inline bool
rp_instance_is_shutdown(RpInstance* self)
{
    return RP_IS_INSTANCE(self) ?
        RP_INSTANCE_GET_IFACE(self)->is_shutdown(self) : false;
}
static inline RpLocalInfo*
rp_instance_local_info(RpInstance* self)
{
    return RP_IS_INSTANCE(self) ?
        RP_INSTANCE_GET_IFACE(self)->local_info(self) : NULL;
}
static inline RpServerFactoryContext*
rp_instance_server_factory_context(RpInstance* self)
{
    return RP_IS_INSTANCE(self) ?
        RP_INSTANCE_GET_IFACE(self)->server_factory_context(self) : NULL;
}
static inline RpTransportSocketFactoryContext*
rp_instance_transport_socket_factory_context(RpInstance* self)
{
    return RP_IS_INSTANCE(self) ?
        RP_INSTANCE_GET_IFACE(self)->transport_socket_factory_context(self) : NULL;
}
static inline bool
rp_instance_enable_reuse_port_default(RpInstance* self)
{
    return RP_IS_INSTANCE(self) ?
        RP_INSTANCE_GET_IFACE(self)->enable_reuse_port_default(self) : false;
}
static inline const rproxy_hooks_t*
rp_instance_hooks(RpInstance* self)
{
    return RP_IS_INSTANCE(self) ?
        RP_INSTANCE_GET_IFACE(self)->hooks(self) : NULL;
}

G_END_DECLS
