/*
 * rp-cluster-info-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-cluster-configuration.h"
#include "rp-factory-context.h"
#include "rp-upstream.h"

G_BEGIN_DECLS

/**
 * Implementation of ClusterInfo that reads from JSON.
 */
#define RP_TYPE_CLUSTER_INFO_IMPL rp_cluster_info_impl_get_type()
G_DECLARE_FINAL_TYPE(RpClusterInfoImpl, rp_cluster_info_impl, RP, CLUSTER_INFO_IMPL, GObject)

RpClusterInfoImpl* rp_cluster_info_impl_new(RpServerFactoryContext* server_context, RpClusterCfg* config, bool added_via_api);

G_END_DECLS
