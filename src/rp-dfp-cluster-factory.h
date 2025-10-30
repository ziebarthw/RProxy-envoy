/*
 * rp-dfp-cluster-factory.h
 */

#pragma once

#include <glib-object.h>
#include "upstream/rp-cluster-factory-impl.h"

G_BEGIN_DECLS

#define RP_TYPE_DFP_CLUSTER_FACTORY rp_dfp_cluster_factory_get_type()
G_DECLARE_FINAL_TYPE(RpDfpClusterFactory, rp_dfp_cluster_factory, RP, DFP_CLUSTER_FACTORY, RpClusterFactoryImplBase)

RpDfpClusterFactory* rp_dfp_cluster_factory_new(void);

G_END_DECLS


