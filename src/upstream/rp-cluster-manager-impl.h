/*
 * rp-cluster-manager-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-factory-context.h"
#include "rp-instance.h"
#include "rp-cluster-manager.h"

G_BEGIN_DECLS

/**
 * Production implementation of ClusterManagerFactory.
 */
#define RP_TYPE_PROD_CLUSTER_MANAGER_FACTORY rp_prod_cluster_manager_factory_get_type()
G_DECLARE_FINAL_TYPE(RpProdClusterManagerFactory, rp_prod_cluster_manager_factory, RP, PROD_CLUSTER_MANAGER_FACTORY, GObject)

RpProdClusterManagerFactory* rp_prod_cluster_manager_factory_new(RpServerFactoryContext* context,
                                                                    RpInstance* server);

G_END_DECLS
