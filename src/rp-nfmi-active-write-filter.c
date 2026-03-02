/*
 * rp-nfmi-active-write-filter.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_nfmi_active_write_filter_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_nfmi_active_write_filter_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-fixed-write-buffer-source.h"
#include "rp-net-filter-mgr-impl.h"
#include "rp-nfmi-active-write-filter.h"

struct _RpNfmiActiveWriteFilter {
    GObject rp_nfmi_instance;

    RpNetworkFilterManagerImpl* m_parent;
    RpNetworkWriteFilterSharedPtr m_filter;
    GSList* m_entry;
};

static void network_filter_callbacks_iface_init(RpNetworkFilterCallbacksInterface* iface);
static void network_write_filter_callbacks_iface_init(RpNetworkWriteFilterCallbacksInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpNfmiActiveWriteFilter, rp_nfmi_active_write_filter, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_FILTER_CALLBACKS, network_filter_callbacks_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_WRITE_FILTER_CALLBACKS, network_write_filter_callbacks_iface_init)
)

extern RpNetworkConnection* rp_nfmi_get_connection(RpNetworkFilterManagerImpl*);
extern RpSocket* rp_nfmi_get_socket(RpNetworkFilterManagerImpl*);
extern void rp_nfmi_on_resume_writing(RpNetworkFilterManagerImpl*, RpNfmiActiveWriteFilter*, RpWriteBufferSource*);

static RpNetworkConnection*
connection_i(RpNetworkFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_nfmi_get_connection(RP_NFMI_ACTIVE_WRITE_FILTER(self)->m_parent);
}

static RpSocket*
socket_i(RpNetworkFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_nfmi_get_socket(RP_NFMI_ACTIVE_WRITE_FILTER(self)->m_parent);
}

static void
network_filter_callbacks_iface_init(RpNetworkFilterCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->connection = connection_i;
    iface->socket = socket_i;
}

static void
inject_write_data_to_filter_chain_i(RpNetworkWriteFilterCallbacks* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    g_autoptr(RpFixedWriteBufferSource) buffer_source = rp_fixed_write_buffer_source_new(data, end_stream);
    RpNfmiActiveWriteFilter* me = RP_NFMI_ACTIVE_WRITE_FILTER(self);
    rp_nfmi_on_resume_writing(me->m_parent, me, RP_WRITE_BUFFER_SOURCE(buffer_source));
}

static void
network_write_filter_callbacks_iface_init(RpNetworkWriteFilterCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->inject_write_data_to_filter_chain = inject_write_data_to_filter_chain_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpNfmiActiveWriteFilter* self = RP_NFMI_ACTIVE_WRITE_FILTER(obj);
    g_clear_object(&self->m_filter);

    G_OBJECT_CLASS(rp_nfmi_active_write_filter_parent_class)->dispose(obj);
}

static void
rp_nfmi_active_write_filter_class_init(RpNfmiActiveWriteFilterClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_nfmi_active_write_filter_init(RpNfmiActiveWriteFilter* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpNfmiActiveWriteFilter*
rp_nfmi_active_write_filter_new(RpNetworkFilterManagerImpl* parent, RpNetworkWriteFilterSharedPtr filter)
{
    LOGD("(%p, %p)", parent, filter);
    g_return_val_if_fail(RP_IS_NETWORK_FILTER_MANAGER_IMPL(parent), NULL);
    g_return_val_if_fail(RP_IS_NETWORK_WRITE_FILTER(filter), NULL);
    RpNfmiActiveWriteFilter* self = g_object_new(RP_TYPE_NFMI_ACTIVE_WRITE_FILTER, NULL);
    self->m_parent = parent;
    self->m_filter = g_object_ref(filter);
    return self;
}

GSList*
rp_nfmi_active_write_filter_get_entry(RpNfmiActiveWriteFilter* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_NFMI_ACTIVE_WRITE_FILTER(self), NULL);
    return self->m_entry;
}

void
rp_nfmi_active_write_filter_set_entry(RpNfmiActiveWriteFilter* self, GSList* entry)
{
    LOGD("(%p, %p)", self, entry);
    g_return_if_fail(RP_IS_NFMI_ACTIVE_WRITE_FILTER(self));
    self->m_entry = entry;
}

RpNetworkWriteFilter*
rp_nfmi_active_write_filter_get_filter(RpNfmiActiveWriteFilter* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_NFMI_ACTIVE_WRITE_FILTER(self), NULL);
    return self->m_filter;
}
