/*
 * rp-cluster-impl-base.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-cluster-configuration.h"

G_BEGIN_DECLS

/**
 * Base class all primary clusters.
 */
#define RP_TYPE_CLUSTER_IMPL_BASE rp_cluster_impl_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpClusterImplBase, rp_cluster_impl_base, RP, CLUSTER_IMPL_BASE, GObject)

struct _RpClusterImplBaseClass {
    GObjectClass parent_class;

    void (*start_pre_init)(RpClusterImplBase*);
};

void rp_cluster_impl_base_on_pre_init_complete(RpClusterImplBase* self);
const RpClusterCfg* rp_cluster_impl_base_config_(RpClusterImplBase* self);

G_END_DECLS
