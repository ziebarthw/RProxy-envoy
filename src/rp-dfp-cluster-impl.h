/*
 * rp-dfp-cluster-impl.h
 *
 * Dynamic Forward Proxy cluster implementation (skeleton) analogous to Envoy's DFP cluster.
 */

#pragma once

#include <glib-object.h>
#include "rp-cluster-configuration.h"
//#include "upstream/rp-cluster-factory-impl.h"
#include "rp-static-cluster-impl.h"

G_BEGIN_DECLS

/**
 * Implementation of a dynamic forward proxy cluster.
 */
#define RP_TYPE_DFP_CLUSTER_IMPL rp_dfp_cluster_impl_get_type()
G_DECLARE_FINAL_TYPE(RpDfpClusterImpl, rp_dfp_cluster_impl, RP, DFP_CLUSTER_IMPL, RpStaticClusterImpl)

RpDfpClusterImpl* rp_dfp_cluster_impl_new(RpClusterCfg* config,
                                          RpClusterFactoryContext* context,
                                          RpStatusCode_e* creation_status);
RpHostSelectionResponse rp_dfp_cluster_impl_choose_host(RpDfpClusterImpl* self,
                                                        const char* host,
                                                        RpLoadBalancerContext* context);
RpClusterCfg rp_dfp_cluster_impl_create_sub_cluster_config(RpDfpClusterImpl* self,
                                                            const char* cluster_name,
                                                            const char* host_name,
                                                            int port);
RpClusterImplBase* rp_dfp_cluster_impl_create_cluster_with_config(RpClusterCfg* cluster,
                                                                    RpClusterFactoryContext* context);
void rp_dfp_cluster_impl_set_disable_sub_cluster(RpDfpClusterImpl* self,
                                                bool disable);

G_END_DECLS


