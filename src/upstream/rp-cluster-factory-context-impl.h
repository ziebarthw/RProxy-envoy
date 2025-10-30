/*
 * rp-cluster-factory-context-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-cluster-factory.h"
#include "rp-cluster-manager.h"

G_BEGIN_DECLS

#define RP_TYPE_CLUSTER_FACTORY_CONTEXT_IMPL rp_cluster_factory_context_impl_get_type()
G_DECLARE_FINAL_TYPE(RpClusterFactoryContextImpl, rp_cluster_factory_context_impl, RP, CLUSTER_FACTORY_CONTEXT_IMPL, GObject)

RpClusterFactoryContextImpl* rp_cluster_factory_context_impl_new(RpServerFactoryContext* server_context,
                                                                    RpClusterManager* cm,
                                                                    bool added_via_api);

G_END_DECLS
