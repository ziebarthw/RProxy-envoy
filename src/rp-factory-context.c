/*
 * rp-factory-context.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#include "rp-factory-context.h"

G_DEFINE_INTERFACE(RpCommonFactoryContext, rp_common_factory_context, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpServerFactoryContext, rp_server_factory_context, RP_TYPE_COMMON_FACTORY_CONTEXT)
G_DEFINE_INTERFACE(RpGenericFactoryContext, rp_generic_factory_context, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpFactoryContext, rp_factory_context, RP_TYPE_GENERIC_FACTORY_CONTEXT)
G_DEFINE_INTERFACE(RpFilterChainFactoryContext, rp_filter_chain_factory_context, RP_TYPE_FACTORY_CONTEXT)
G_DEFINE_INTERFACE(RpListenerFactoryContext, rp_listener_factory_context, RP_TYPE_LISTENER_FACTORY_CONTEXT)
G_DEFINE_INTERFACE(RpUpstreamFactoryContext, rp_upstream_factory_context, G_TYPE_OBJECT)

static void
rp_common_factory_context_default_init(RpCommonFactoryContextInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_server_factory_context_default_init(RpServerFactoryContextInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_generic_factory_context_default_init(RpGenericFactoryContextInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_factory_context_default_init(RpFactoryContextInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_filter_chain_factory_context_default_init(RpFilterChainFactoryContextInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_listener_factory_context_default_init(RpListenerFactoryContextInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_upstream_factory_context_default_init(RpUpstreamFactoryContextInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
