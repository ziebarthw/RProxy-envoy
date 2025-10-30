/*
 * rp-cluster-factory.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-cluster-configuration.h"
#include "rp-cluster-manager.h"
#include "rp-codec.h"
#include "rp-factory-context.h"

G_BEGIN_DECLS

/**
 * Context passed to cluster factory to access Envoy resources. Cluster factory should only access
 * the rest of the server through this context object.
 */
#define RP_TYPE_CLUSTER_FACTORY_CONTEXT rp_cluster_factory_context_get_type()
G_DECLARE_INTERFACE(RpClusterFactoryContext, rp_cluster_factory_context, RP, CLUSTER_FACTORY_CONTEXT, GObject)

struct _RpClusterFactoryContextInterface {
    GTypeInterface parent_iface;

    RpServerFactoryContext* (*server_factory_context)(RpClusterFactoryContext*);
    RpClusterManager* (*cluster_manager)(RpClusterFactoryContext*);
    //TODO...
};

static inline RpServerFactoryContext*
rp_cluster_factory_context_server_factory_context(RpClusterFactoryContext* self)
{
    return RP_IS_CLUSTER_FACTORY_CONTEXT(self) ?
        RP_CLUSTER_FACTORY_CONTEXT_GET_IFACE(self)->server_factory_context(self) :
        NULL;
}
static inline RpClusterManager*
rp_cluster_factory_context_cluster_manager(RpClusterFactoryContext* self)
{
    return RP_IS_CLUSTER_FACTORY_CONTEXT(self) ?
        RP_CLUSTER_FACTORY_CONTEXT_GET_IFACE(self)->cluster_manager(self) :
        NULL;
}


/**
 * Implemented by cluster and registered via Registry::registerFactory() or the convenience class
 * RegisterFactory.
 */
#define RP_TYPE_CLUSTER_FACTORY rp_cluster_factory_get_type()
G_DECLARE_INTERFACE(RpClusterFactory, rp_cluster_factory, RP, CLUSTER_FACTORY, GObject)

struct _RpClusterFactoryInterface {
    GTypeInterface parent_iface;

    RpCluster* (*create)(RpClusterFactory*, RpClusterCfg*, RpClusterFactoryContext*);
};

static inline RpCluster*
rp_cluster_factory_create(RpClusterFactory* self, RpClusterCfg* cluster, RpClusterFactoryContext* context)
{
    return RP_IS_CLUSTER_FACTORY(self) ?
        RP_CLUSTER_FACTORY_GET_IFACE(self)->create(self, cluster, context) :
        NULL;
}

G_END_DECLS
