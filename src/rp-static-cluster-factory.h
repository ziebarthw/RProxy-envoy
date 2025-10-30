/*
 * rp-static-cluster-factory.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "upstream/rp-cluster-factory-impl.h"

G_BEGIN_DECLS

/**
 * Factory for StaticClusterImpl cluster.
 */
#define RP_TYPE_STATIC_CLUSTER_FACTORY rp_static_cluster_factory_get_type()
G_DECLARE_FINAL_TYPE(RpStaticClusterFactory, rp_static_cluster_factory, RP, STATIC_CLUSTER_FACTORY, RpClusterFactoryImplBase)

RpStaticClusterFactory* rp_static_cluster_factory_new(void);

G_END_DECLS
