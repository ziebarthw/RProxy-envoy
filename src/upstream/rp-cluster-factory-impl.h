/*
 * rp-cluster-factory-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "upstream/rp-cluster-impl-base.h"
#include "rp-cluster-configuration.h"
#include "rp-cluster-factory.h"

G_BEGIN_DECLS

/**
 * Base class for all cluster factory implementation. This class can be directly extended if the
 * custom cluster does not have any custom configuration. For custom cluster with custom
 * configuration, use ConfigurableClusterFactoryBase instead.
 */
#define RP_TYPE_CLUSTER_FACTORY_IMPL_BASE rp_cluster_factory_impl_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpClusterFactoryImplBase, rp_cluster_factory_impl_base, RP, CLUSTER_FACTORY_IMPL_BASE, GObject)

struct _RpClusterFactoryImplBaseClass {
    GObjectClass parent_class;

    RpClusterImplBase* (*create_cluster_impl)(RpClusterFactoryImplBase*, RpClusterCfg*, RpClusterFactoryContext*);
};

G_END_DECLS
