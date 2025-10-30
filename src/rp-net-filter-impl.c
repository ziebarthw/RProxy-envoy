/*
 * rp-net-filter-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_net_filter_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_net_filter_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include "rp-net-filter-impl.h"

static void network_read_filter_iface_init(RpNetworkReadFilterInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpNetworkReadFilterBaseImpl, rp_network_read_filter_base_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_READ_FILTER, network_read_filter_iface_init)
)

static void
initialize_read_filter_callbacks_i(RpNetworkReadFilter* self G_GNUC_UNUSED, RpNetworkReadFilterCallbacks* callbacks G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
}

static RpNetworkFilterStatus_e
on_new_connection_i(RpNetworkReadFilter* self)
{
    NOISY_MSG_("(%p)", self);
    return RpNetworkFilterStatus_Continue;
}

static void
network_read_filter_iface_init(RpNetworkReadFilterInterface* iface)
{
    NOISY_MSG_("(%p)", iface);
    iface->initialize_read_filter_callbacks = initialize_read_filter_callbacks_i;
    iface->on_new_connection = on_new_connection_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p(%s))", obj, G_OBJECT_TYPE_NAME(obj));
    G_OBJECT_CLASS(rp_network_read_filter_base_impl_parent_class)->dispose(obj);
}

static void
rp_network_read_filter_base_impl_class_init(RpNetworkReadFilterBaseImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_network_read_filter_base_impl_init(RpNetworkReadFilterBaseImpl* self G_GNUC_UNUSED)
{
    LOGD("(%p)", self);
}
