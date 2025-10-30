/*
 * rp-factory-context.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <evhtp.h>
#include "rp-cluster-manager.h"
#include "rp-local-info.h"
#include "rp-transport-socket-config.h"

G_BEGIN_DECLS

/**
 * Common interface for downstream and upstream network filters to access server
 * wide resources. This could be treated as limited form of server factory context.
 */
#define RP_TYPE_COMMON_FACTORY_CONTEXT rp_common_factory_context_get_type()
G_DECLARE_INTERFACE(RpCommonFactoryContext, rp_common_factory_context, RP, COMMON_FACTORY_CONTEXT, GObject)

struct _RpCommonFactoryContextInterface {
    GTypeInterface parent_iface;

    evhtp_t* (*main_thread_dispatcher)(RpCommonFactoryContext*);
    RpLocalInfo* (*local_info)(RpCommonFactoryContext*);
    //TODO...Regex::Engine regexEngine() PURE;
    RpClusterManager* (*cluster_manager)(RpCommonFactoryContext*);
};

static inline evhtp_t*
rp_common_factory_context_main_thread_dispatcher(RpCommonFactoryContext* self)
{
    return RP_IS_COMMON_FACTORY_CONTEXT(self) ?
        RP_COMMON_FACTORY_CONTEXT_GET_IFACE(self)->main_thread_dispatcher(self) :
        NULL;
}
static inline RpLocalInfo*
rp_common_factory_context_local_info(RpCommonFactoryContext* self)
{
    return RP_IS_COMMON_FACTORY_CONTEXT(self) ?
        RP_COMMON_FACTORY_CONTEXT_GET_IFACE(self)->local_info(self) :
        NULL;
}
static inline RpClusterManager*
rp_common_factory_context_cluster_manager(RpCommonFactoryContext* self)
{
    return RP_IS_COMMON_FACTORY_CONTEXT(self) ?
        RP_COMMON_FACTORY_CONTEXT_GET_IFACE(self)->cluster_manager(self) :
        NULL;
}


/**
 * ServerFactoryContext is an specialization of common interface for downstream and upstream network
 * filters. The implementation guarantees the lifetime is no shorter than server. It could be used
 * across listeners.
 */
#define RP_TYPE_SERVER_FACTORY_CONTEXT rp_server_factory_context_get_type()
G_DECLARE_INTERFACE(RpServerFactoryContext, rp_server_factory_context, RP, SERVER_FACTORY_CONTEXT, RpCommonFactoryContext)

struct _RpServerFactoryContextInterface {
    RpCommonFactoryContextInterface parent_iface;

    RpTransportSocketFactoryContext* (*get_transport_socket_factory_context)(RpServerFactoryContext*);
};

static inline RpTransportSocketFactoryContext*
rp_server_factory_context_get_transport_socket_factory_context(RpServerFactoryContext* self)
{
    return RP_IS_SERVER_FACTORY_CONTEXT(self) ?
        RP_SERVER_FACTORY_CONTEXT_GET_IFACE(self)->get_transport_socket_factory_context(self) :
        NULL;
}


/**
 * Generic factory context for multiple scenarios. This context provides a server factory context
 * reference and other resources. Note that except for server factory context, other resources are
 * not guaranteed to be available for the entire server lifetime. For example, context powered by a
 * listener is only available for the lifetime of the listener.
 */
#define RP_TYPE_GENERIC_FACTORY_CONTEXT rp_generic_factory_context_get_type()
G_DECLARE_INTERFACE(RpGenericFactoryContext, rp_generic_factory_context, RP, GENERIC_FACTORY_CONTEXT, GObject)

struct _RpGenericFactoryContextInterface {
    GTypeInterface parent_iface;

    RpServerFactoryContext* (*server_factory_context)(RpGenericFactoryContext*);
};

static inline RpServerFactoryContext*
rp_generic_factory_context_server_factory_context(RpGenericFactoryContext* self)
{
    return RP_IS_GENERIC_FACTORY_CONTEXT(self) ?
        RP_GENERIC_FACTORY_CONTEXT_GET_IFACE(self)->server_factory_context(self) :
        NULL;
}


/**
 * Context passed to network and HTTP filters to access server resources.
 * TODO(mattklein123): When we lock down visibility of the rest of the code, filters should only
 * access the rest of the server via interfaces exposed here.
 */
#define RP_TYPE_FACTORY_CONTEXT rp_factory_context_get_type()
G_DECLARE_INTERFACE(RpFactoryContext, rp_factory_context, RP, FACTORY_CONTEXT, RpGenericFactoryContext)

struct _RpFactoryContextInterface {
    RpGenericFactoryContextInterface parent_iface;

    RpTransportSocketFactoryContext* (*get_transport_socket_factory_context)(RpFactoryContext*);
};

static inline RpTransportSocketFactoryContext*
rp_factory_context_get_transport_socket_factory_context(RpFactoryContext* self)
{
    return RP_IS_FACTORY_CONTEXT(self) ?
        RP_FACTORY_CONTEXT_GET_IFACE(self)->get_transport_socket_factory_context(self) :
        NULL;
}


/**
 * An implementation of FactoryContext. The life time is no shorter than the created filter chains.
 * The life time is no longer than the owning listener. It should be used to create
 * NetworkFilterChain.
 */
#define RP_TYPE_FILTER_CHAIN_FACTORY_CONTEXT rp_filter_chain_factory_context_get_type()
G_DECLARE_INTERFACE(RpFilterChainFactoryContext, rp_filter_chain_factory_context, RP, FILTER_CHAIN_FACTORY_CONTEXT, RpFactoryContext)

struct _RpFilterChainFactoryContextInterface {
    RpFactoryContextInterface parent_iface;

    void (*start_draining)(RpFilterChainFactoryContext*);
};

static inline void
rp_filter_chain_factory_context_start_draining(RpFilterChainFactoryContext* self)
{
    if (RP_IS_FILTER_CHAIN_FACTORY_CONTEXT(self))
    {
        RP_FILTER_CHAIN_FACTORY_CONTEXT_GET_IFACE(self)->start_draining(self);
    }
}


/**
 * FactoryContext for upstream HTTP filters.
 */
#define RP_TYPE_UPSTREAM_FACTORY_CONTEXT rp_upstream_factory_context_get_type()
G_DECLARE_INTERFACE(RpUpstreamFactoryContext, rp_upstream_factory_context, RP, UPSTREAM_FACTORY_CONTEXT, GObject)

struct _RpUpstreamFactoryContextInterface {
    GTypeInterface parent_iface;

    RpServerFactoryContext* (*server_factory_context)(RpUpstreamFactoryContext*);
};

static inline RpServerFactoryContext*
rp_upstream_factory_context_server_factory_context(RpUpstreamFactoryContext* self)
{
    return RP_IS_UPSTREAM_FACTORY_CONTEXT(self) ?
        RP_UPSTREAM_FACTORY_CONTEXT_GET_IFACE(self)->server_factory_context(self) :
        NULL;
}

G_END_DECLS
