/*
 * rp-nfmi-active-read-filter.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#if (defined(rp_nfmi_active_read_filter_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_nfmi_active_read_filter_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rproxy.h"
#include "rp-fixed-read-buffer-source.h"
#include "rp-net-filter-mgr-impl.h"
#include "rp-nfmi-active-read-filter.h"

struct _RpNfmiActiveReadFilter {
    GObject rp_nfmi_instance;

    RpNetworkFilterManagerImpl* m_parent;
    UNIQUE_PTR(RpNetworkReadFilter) m_filter;
    GSList* m_entry;
    bool m_initialized;
};

static void network_filter_callbacks_iface_init(RpNetworkFilterCallbacksInterface* iface);
static void network_read_filter_callbacks_iface_init(RpNetworkReadFilterCallbacksInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpNfmiActiveReadFilter, rp_nfmi_active_read_filter, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_FILTER_CALLBACKS, network_filter_callbacks_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_READ_FILTER_CALLBACKS, network_read_filter_callbacks_iface_init)
)

extern RpNetworkConnection* rp_nfmi_get_connection(RpNetworkFilterManagerImpl*);
extern RpSocket* rp_nfmi_get_socket(RpNetworkFilterManagerImpl*);
extern void rp_nfmi_on_continue_reading(RpNetworkFilterManagerImpl*, RpNfmiActiveReadFilter*, RpReadBufferSource*);

static RpNetworkConnection*
connection_i(RpNetworkFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_nfmi_get_connection(RP_NFMI_ACTIVE_READ_FILTER(self)->m_parent);
}

static RpSocket*
socket_i(RpNetworkFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_nfmi_get_socket(RP_NFMI_ACTIVE_READ_FILTER(self)->m_parent);
}

static void
network_filter_callbacks_iface_init(RpNetworkFilterCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->connection = connection_i;
    iface->socket = socket_i;
}

static void
continue_reading_i(RpNetworkReadFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpNfmiActiveReadFilter* me = RP_NFMI_ACTIVE_READ_FILTER(self);
    RpNetworkConnection* connection = rp_nfmi_get_connection(me->m_parent);
    rp_nfmi_on_continue_reading(me->m_parent, me, RP_READ_BUFFER_SOURCE(connection));
}

static void
inject_read_data_to_filter_chain_i(RpNetworkReadFilterCallbacks* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    g_autoptr(RpFixedReadBufferSource) buffer_source = rp_fixed_read_buffer_source_new(data, end_stream);
    RpNfmiActiveReadFilter* me = RP_NFMI_ACTIVE_READ_FILTER(self);
    rp_nfmi_on_continue_reading(me->m_parent, me, RP_READ_BUFFER_SOURCE(buffer_source));
}

static void
start_upstream_secure_transport_i(RpNetworkReadFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpNfmiActiveReadFilter* me = RP_NFMI_ACTIVE_READ_FILTER(self);
    rp_network_filter_manager_impl_start_upstream_secure_transport(me->m_parent);
}

static void
network_read_filter_callbacks_iface_init(RpNetworkReadFilterCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->continue_reading = continue_reading_i;
    iface->inject_read_data_to_filter_chain = inject_read_data_to_filter_chain_i;
    iface->start_upstream_secure_transport = start_upstream_secure_transport_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpNfmiActiveReadFilter* self = RP_NFMI_ACTIVE_READ_FILTER(obj);
//NOISY_MSG_("clearing filter %p(%u)", self->m_filter, G_OBJECT(self->m_filter)->ref_count);
NOISY_MSG_("clearing filter %p(%s)", self->m_filter, G_OBJECT_TYPE_NAME(self->m_filter));
    g_clear_object(&self->m_filter);

    G_OBJECT_CLASS(rp_nfmi_active_read_filter_parent_class)->dispose(obj);
}

static void
rp_nfmi_active_read_filter_class_init(RpNfmiActiveReadFilterClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_nfmi_active_read_filter_init(RpNfmiActiveReadFilter* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_initialized = false;
}

RpNfmiActiveReadFilter*
rp_nfmi_active_read_filter_new(RpNetworkFilterManagerImpl* parent, RpNetworkReadFilter* filter)
{
    LOGD("(%p, %p)", parent, filter);
    g_return_val_if_fail(RP_IS_NETWORK_FILTER_MANAGER_IMPL(parent), NULL);
    g_return_val_if_fail(RP_IS_NETWORK_READ_FILTER(filter), NULL);
    RpNfmiActiveReadFilter* self = g_object_new(RP_TYPE_NFMI_ACTIVE_READ_FILTER, NULL);
    self->m_parent = parent;
    self->m_filter = g_object_ref(g_steal_pointer(&filter));
    return self;
}

bool
rp_nfmi_active_read_filter_initialized(RpNfmiActiveReadFilter* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_NFMI_ACTIVE_READ_FILTER(self), false);
    return self->m_initialized;
}

void
rp_nfmi_active_read_filter_set_initialized(RpNfmiActiveReadFilter* self, bool v)
{
    LOGD("(%p, %u)", self, v);
    g_return_if_fail(RP_IS_NFMI_ACTIVE_READ_FILTER(self));
    self->m_initialized = v;
}

GSList*
rp_nfmi_active_read_filter_get_entry(RpNfmiActiveReadFilter* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_NFMI_ACTIVE_READ_FILTER(self), NULL);
    return self->m_entry;
}

void
rp_nfmi_active_read_filter_set_entry(RpNfmiActiveReadFilter* self, GSList* entry)
{
    LOGD("(%p, %p)", self, entry);
    g_return_if_fail(RP_IS_NFMI_ACTIVE_READ_FILTER(self));
    self->m_entry = entry;
}

RpNetworkReadFilter*
rp_nfmi_active_read_filter_get_filter(RpNfmiActiveReadFilter* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_NFMI_ACTIVE_READ_FILTER(self), NULL);
    return self->m_filter;
}
