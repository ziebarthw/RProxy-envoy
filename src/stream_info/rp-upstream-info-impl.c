/*
 * rp-upstream-info-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_upstream_info_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_upstream_info_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "stream_info/rp-upstream-info-impl.h"

#define UPSTREAM_INFO_IMPL(s) RP_UPSTREAM_INFO_IMPL((GObject*)s)

struct _RpUpstreamInfoImpl {
    GObject parent_instance;

    RpHostDescriptionSharedPtr m_upstream_host;

    RpNetworkAddressInstanceSharedPtr m_upstream_local_address;
    RpNetworkAddressInstanceSharedPtr m_upstream_remote_address;

    const char* m_upstream_connection_interface_name;
    const char* m_upstream_transport_failure_reason;

    RpFilterState* m_upstream_filter_state;
    RpSslConnectionInfo* m_upstream_ssl_info;

    RpUpstreamTiming m_upstream_timing;

    guint64 m_upstream_connection_id;
    gsize m_num_streams;
    evhtp_proto m_upstream_protocol;
};

static void upstream_info_iface_init(RpUpstreamInfoInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpUpstreamInfoImpl, rp_upstream_info_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_UPSTREAM_INFO, upstream_info_iface_init)
)

static void
set_upstream_connection_id_i(RpUpstreamInfo* self, guint64 id)
{
    NOISY_MSG_("(%p, %zu)", self, id);
    UPSTREAM_INFO_IMPL(self)->m_upstream_connection_id = id;
}

static void
set_upstream_filter_state_i(RpUpstreamInfo* self, RpFilterState* filter_state)
{
    NOISY_MSG_("(%p, %p)", self, filter_state);
    UPSTREAM_INFO_IMPL(self)->m_upstream_filter_state = filter_state;
}

static void
set_upstream_host_i(RpUpstreamInfo* self, RpHostDescriptionConstSharedPtr host)
{
    NOISY_MSG_("(%p, %p)", self, host);
    rp_host_description_set_object(&UPSTREAM_INFO_IMPL(self)->m_upstream_host, host);
}

static void
set_upstream_interface_name_i(RpUpstreamInfo* self, const char* interface_name)
{
    NOISY_MSG_("(%p, %p(%s))", self, interface_name, interface_name);
    UPSTREAM_INFO_IMPL(self)->m_upstream_connection_interface_name = interface_name;
}

static void
set_upstream_local_address_i(RpUpstreamInfo* self, RpNetworkAddressInstanceConstSharedPtr local_address)
{
    NOISY_MSG_("(%p, %p)", self, local_address);
    RpUpstreamInfoImpl* me = UPSTREAM_INFO_IMPL(self);
    rp_network_address_instance_set_object(&me->m_upstream_local_address, local_address);
}

static void
set_upstream_num_streams_i(RpUpstreamInfo* self, gint64 num_streams)
{
    NOISY_MSG_("(%p, %zu)", self, num_streams);
    UPSTREAM_INFO_IMPL(self)->m_num_streams = num_streams;
}

static void
set_upstream_protocol_i(RpUpstreamInfo* self, evhtp_proto protocol)
{
    NOISY_MSG_("(%p, %d)", self, protocol);
    UPSTREAM_INFO_IMPL(self)->m_upstream_protocol = protocol;
}

static void
set_upstream_remote_address_i(RpUpstreamInfo* self, RpNetworkAddressInstanceConstSharedPtr remote_address)
{
    NOISY_MSG_("(%p, %p)", self, remote_address);
    RpUpstreamInfoImpl* me = UPSTREAM_INFO_IMPL(self);
    rp_network_address_instance_set_object(&me->m_upstream_remote_address, remote_address);
}

static void
set_upstream_ssl_connection_i(RpUpstreamInfo* self, RpSslConnectionInfo* ssl_connection_info)
{
    NOISY_MSG_("(%p, %p)", self, ssl_connection_info);
    UPSTREAM_INFO_IMPL(self)->m_upstream_ssl_info = ssl_connection_info;
}

static void
set_upstream_transport_failure_reason_i(RpUpstreamInfo* self, const char* failure_reason)
{
    NOISY_MSG_("(%p, %p(%s))", self, failure_reason, failure_reason);
    UPSTREAM_INFO_IMPL(self)->m_upstream_transport_failure_reason = failure_reason;
}

static guint64
upstream_connection_id_i(const RpUpstreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return UPSTREAM_INFO_IMPL(self)->m_upstream_connection_id;
}

static RpFilterState*
upstream_filter_state_i(const RpUpstreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return UPSTREAM_INFO_IMPL(self)->m_upstream_filter_state;
}

static RpHostDescriptionConstSharedPtr
upstream_host_i(const RpUpstreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return UPSTREAM_INFO_IMPL(self)->m_upstream_host;
}

static const char*
upstream_interface_name_i(const RpUpstreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return UPSTREAM_INFO_IMPL(self)->m_upstream_connection_interface_name;
}

static RpNetworkAddressInstanceConstSharedPtr
upstream_local_address_i(const RpUpstreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return UPSTREAM_INFO_IMPL(self)->m_upstream_local_address;
}

static guint64
upstream_num_streams_i(const RpUpstreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return UPSTREAM_INFO_IMPL(self)->m_num_streams;
}

static evhtp_proto
upstream_protocol_i(const RpUpstreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return UPSTREAM_INFO_IMPL(self)->m_upstream_protocol;
}

static RpNetworkAddressInstanceConstSharedPtr
upstream_remote_address_i(const RpUpstreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return UPSTREAM_INFO_IMPL(self)->m_upstream_remote_address;
}

static RpSslConnectionInfo*
upstream_ssl_connection_i(const RpUpstreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return UPSTREAM_INFO_IMPL(self)->m_upstream_ssl_info;
}

static const char*
upstream_transport_failure_reason_i(const RpUpstreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return UPSTREAM_INFO_IMPL(self)->m_upstream_transport_failure_reason;
}

static RpUpstreamTiming*
upstream_timing_i(const RpUpstreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return &UPSTREAM_INFO_IMPL(self)->m_upstream_timing;
}

static void
upstream_info_iface_init(RpUpstreamInfoInterface* iface)
{
    LOGD("(%p)", iface);
    iface->set_upstream_connection_id = set_upstream_connection_id_i;
    iface->set_upstream_filter_state = set_upstream_filter_state_i;
    iface->set_upstream_host = set_upstream_host_i;
    iface->set_upstream_interface_name = set_upstream_interface_name_i;
    iface->set_upstream_local_address = set_upstream_local_address_i;
    iface->set_upstream_num_streams = set_upstream_num_streams_i;
    iface->set_upstream_protocol = set_upstream_protocol_i;
    iface->set_upstream_remote_address = set_upstream_remote_address_i;
    iface->set_upstream_ssl_connection = set_upstream_ssl_connection_i;
    iface->set_upstream_transport_failure_reason = set_upstream_transport_failure_reason_i;
    iface->upstream_connection_id = upstream_connection_id_i;
    iface->upstream_filter_state = upstream_filter_state_i;
    iface->upstream_host = upstream_host_i;
    iface->upstream_interface_name = upstream_interface_name_i;
    iface->upstream_local_address = upstream_local_address_i;
    iface->upstream_num_streams = upstream_num_streams_i;
    iface->upstream_protocol = upstream_protocol_i;
    iface->upstream_remote_address = upstream_remote_address_i;
    iface->upstream_ssl_connection = upstream_ssl_connection_i;
    iface->upstream_transport_failure_reason = upstream_transport_failure_reason_i;
    iface->upstream_timing = upstream_timing_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    LOGD("(%p)", obj);

    RpUpstreamInfoImpl* self = RP_UPSTREAM_INFO_IMPL(obj);
if (self->m_upstream_local_address) NOISY_MSG_("%p, upstream local address %p(%u)", self, self->m_upstream_local_address, G_OBJECT(self->m_upstream_local_address)->ref_count);
    g_clear_object(&self->m_upstream_local_address);
    g_clear_object(&self->m_upstream_remote_address);

    G_OBJECT_CLASS(rp_upstream_info_impl_parent_class)->dispose(obj);
}

static void
rp_upstream_info_impl_class_init(RpUpstreamInfoImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_upstream_info_impl_init(RpUpstreamInfoImpl* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_upstream_timing = rp_upstream_timing_ctor();
}

RpUpstreamInfoImpl*
rp_upstream_info_impl_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_UPSTREAM_INFO_IMPL, NULL);
}
