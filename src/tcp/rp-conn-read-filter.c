/*
 * rp-conn-read-filter.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_conn_read_filter_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_conn_read_filter_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "tcp/rp-conn-read-filter.h"

struct _RpConnReadFilter {
    RpNetworkReadFilterBaseImpl parent_instance;

    RpActiveTcpClient* m_parent;
};

static void network_read_filter_iface_init(RpNetworkReadFilterInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpConnReadFilter, rp_conn_read_filter, RP_TYPE_NETWORK_READ_FILTER_BASE_IMPL,
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_READ_FILTER, network_read_filter_iface_init)
)

static RpNetworkFilterStatus_e
on_data_i(RpNetworkReadFilter* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    rp_active_tcp_client_on_upstream_data(RP_CONN_READ_FILTER(self)->m_parent, data, end_stream);
    return RpNetworkFilterStatus_StopIteration;
}

static void
network_read_filter_iface_init(RpNetworkReadFilterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_data = on_data_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_conn_read_filter_parent_class)->dispose(obj);
}

static void
rp_conn_read_filter_class_init(RpConnReadFilterClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_conn_read_filter_init(RpConnReadFilter* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpConnReadFilter*
rp_conn_read_filter_new(RpActiveTcpClient* parent)
{
    LOGD("(%p)", parent);
    g_return_val_if_fail(RP_IS_ACTIVE_TCP_CLIENT(parent), NULL);
    RpConnReadFilter* self = g_object_new(RP_TYPE_CONN_READ_FILTER, NULL);
    self->m_parent = parent;
    return self;
}
