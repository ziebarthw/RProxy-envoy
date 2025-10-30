/*
 * rp-net-filter.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#include "rp-net-filter.h"

G_DEFINE_INTERFACE(RpNetworkFilterCallbacks, rp_network_filter_callbacks, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpNetworkWriteFilterCallbacks, rp_network_write_filter_callbacks, RP_TYPE_NETWORK_FILTER_CALLBACKS)
G_DEFINE_INTERFACE(RpNetworkWriteFilter, rp_network_write_filter, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpNetworkReadFilterCallbacks, rp_network_read_filter_callbacks, RP_TYPE_NETWORK_FILTER_CALLBACKS)
G_DEFINE_INTERFACE(RpNetworkReadFilter, rp_network_read_filter, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpNetworkFilterManager, rp_network_filter_manager, G_TYPE_OBJECT)

static void
rp_network_filter_callbacks_default_init(RpNetworkFilterCallbacksInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_network_write_filter_callbacks_default_init(RpNetworkWriteFilterCallbacksInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_network_write_filter_default_init(RpNetworkWriteFilterInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_network_read_filter_callbacks_default_init(RpNetworkReadFilterCallbacksInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_network_read_filter_default_init(RpNetworkReadFilterInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
static void
rp_network_filter_manager_default_init(RpNetworkFilterManagerInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
