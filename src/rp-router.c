/*
 * rp-router.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_router_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_router_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-router.h"

G_DEFINE_INTERFACE(RpResponseEntry, rp_response_entry, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpDirectResponseEntry, rp_direct_response_entry, RP_TYPE_RESPONSE_ENTRY)
G_DEFINE_INTERFACE(RpRouteEntry, rp_route_entry, RP_TYPE_RESPONSE_ENTRY)
G_DEFINE_INTERFACE(RpRoute, rp_route, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpUpstreamToDownstream, rp_upstream_to_downstream, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpGenericUpstream, rp_generic_upstream, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpGenericConnPool, rp_generic_conn_pool, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpGenericConnectionPoolCallbacks, rp_generic_connection_pool_callbacks, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpGenericConnPoolFactory, rp_generic_conn_pool_factory, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpRouteCommonConfig, rp_route_common_config, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpRouteConfig, rp_route_config, RP_TYPE_ROUTE_COMMON_CONFIG)
G_DEFINE_INTERFACE(RpVirtualHost, rp_virtual_host, G_TYPE_OBJECT)

static void
rp_response_entry_default_init(RpResponseEntryInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_direct_response_entry_default_init(RpDirectResponseEntryInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_route_entry_default_init(RpRouteEntryInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_route_default_init(RpRouteInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_upstream_to_downstream_default_init(RpUpstreamToDownstreamInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_generic_upstream_default_init(RpGenericUpstreamInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_generic_conn_pool_default_init(RpGenericConnPoolInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_generic_connection_pool_callbacks_default_init(RpGenericConnectionPoolCallbacksInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_generic_conn_pool_factory_default_init(RpGenericConnPoolFactoryInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_route_common_config_default_init(RpRouteCommonConfigInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_route_config_default_init(RpRouteConfigInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_virtual_host_default_init(RpVirtualHostInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
