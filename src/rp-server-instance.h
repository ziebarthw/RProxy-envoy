/*
 * rp-server-instance.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-cluster-manager.h"
#include "rp-dispatcher.h"
#include "rp-factory-context.h"
#include "rp-local-info.h"
#include "rp-singleton-manager.h"
#include "rp-thread-local.h"
#include "rp-transport-socket-config.h"

G_BEGIN_DECLS

/**
 * An instance of the running server.
 */
#define RP_TYPE_SERVER_INSTANCE rp_server_instance_get_type()
G_DECLARE_INTERFACE(RpServerInstance, rp_server_instance, RP, SERVER_INSTANCE, GObject)

struct _RpServerInstanceInterface {
    GTypeInterface parent_iface;

    void (*run)(RpServerInstance*);
    RpClusterManager* (*cluster_manager)(RpServerInstance*);
    RpDispatcher* (*dispatcher)(RpServerInstance*);
    void (*shutdown)(RpServerInstance*);
    bool (*is_shutdown)(RpServerInstance*);
//TODO...shutdownAdmin();
    RpSingletonManager* (*singleton_manager)(RpServerInstance*);
//TODO...Http::Context& httpContext();
//TODO...Router::Context& routerContext();
//TODO...ProcessContextOptRef processContext();
    RpThreadLocalInstance* (*thread_local)(RpServerInstance*);
    RpLocalInfo* (*local_info)(RpServerInstance*);
//TODO...Regex::Engine& regexEngine();
    rproxy_t* (*bootstrap)(RpServerInstance*);
    RpServerFactoryContext* (*server_factory_context)(RpServerInstance*);
    RpTransportSocketFactoryContext* (*transport_socket_factory_context)(RpServerInstance*);
    bool (*enable_reuse_port_default)(RpServerInstance*);

    // Custom.
    GMutex* (*lock)(RpServerInstance*);
};

static inline void
rp_server_instance_run(RpServerInstance* self)
{
    if (RP_IS_SERVER_INSTANCE(self)) \
        RP_SERVER_INSTANCE_GET_IFACE(self)->run(self);
}
static inline RpClusterManager*
rp_server_instance_cluster_manager(RpServerInstance* self)
{
    return RP_IS_SERVER_INSTANCE(self) ?
        RP_SERVER_INSTANCE_GET_IFACE(self)->cluster_manager(self) : NULL;
}
static inline RpDispatcher*
rp_server_instance_dispatcher(RpServerInstance* self)
{
    return RP_IS_SERVER_INSTANCE(self) ?
        RP_SERVER_INSTANCE_GET_IFACE(self)->dispatcher(self) : NULL;
}
static inline void
rp_server_instance_shutdown(RpServerInstance* self)
{
    if (RP_IS_SERVER_INSTANCE(self)) \
        RP_SERVER_INSTANCE_GET_IFACE(self)->shutdown(self);
}
static inline bool
rp_server_instance_is_shutdown(RpServerInstance* self)
{
    return RP_IS_SERVER_INSTANCE(self) ?
        RP_SERVER_INSTANCE_GET_IFACE(self)->is_shutdown(self) : false;
}
static inline RpSingletonManager*
rp_server_instance_singleton_manager(RpServerInstance* self)
{
    return RP_IS_SERVER_INSTANCE(self) ?
        RP_SERVER_INSTANCE_GET_IFACE(self)->singleton_manager(self) : NULL;
}
static inline RpThreadLocalInstance*
rp_server_instance_thread_local(RpServerInstance* self)
{
    return RP_IS_SERVER_INSTANCE(self) ?
        RP_SERVER_INSTANCE_GET_IFACE(self)->thread_local(self) : NULL;
}
static inline RpLocalInfo*
rp_server_instance_local_info(RpServerInstance* self)
{
    return RP_IS_SERVER_INSTANCE(self) ?
        RP_SERVER_INSTANCE_GET_IFACE(self)->local_info(self) : NULL;
}
static inline rproxy_t*
rp_server_instance_bootstrap(RpServerInstance* self)
{
    return RP_IS_SERVER_INSTANCE(self) ?
        RP_SERVER_INSTANCE_GET_IFACE(self)->bootstrap(self) : NULL;
}
static inline RpServerFactoryContext*
rp_server_instance_server_factory_context(RpServerInstance* self)
{
    return RP_IS_SERVER_INSTANCE(self) ?
        RP_SERVER_INSTANCE_GET_IFACE(self)->server_factory_context(self) : NULL;
}
static inline RpTransportSocketFactoryContext*
rp_server_instance_transport_socket_factory_context(RpServerInstance* self)
{
    return RP_IS_SERVER_INSTANCE(self) ?
        RP_SERVER_INSTANCE_GET_IFACE(self)->transport_socket_factory_context(self) :
        NULL;
}
static inline bool
rp_server_instance_enable_reuse_port_default(RpServerInstance* self)
{
    return RP_IS_SERVER_INSTANCE(self) ?
        RP_SERVER_INSTANCE_GET_IFACE(self)->enable_reuse_port_default(self) : false;
}
static inline GMutex*
rp_server_instance_lock(RpServerInstance* self)
{
    return RP_IS_SERVER_INSTANCE(self) ?
        RP_SERVER_INSTANCE_GET_IFACE(self)->lock(self) : NULL;
}

G_END_DECLS
