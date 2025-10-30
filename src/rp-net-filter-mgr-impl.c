/*
 * rp-net-filter-mgr-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_net_filter_mgr_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_net_filter_mgr_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include "rp-nfmi-active-read-filter.h"
#include "rp-nfmi-active-write-filter.h"
#include "rp-net-filter-mgr-impl.h"

G_DEFINE_INTERFACE(RpReadBufferSource, rp_read_buffer_source, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpWriteBufferSource, rp_write_buffer_source, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpFilterManagerConnection, rp_filter_manager_connection, RP_TYPE_NETWORK_CONNECTION)

static void
rp_read_buffer_source_default_init(RpReadBufferSourceInterface* iface G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", iface);
}
static void
rp_write_buffer_source_default_init(RpWriteBufferSourceInterface* iface G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", iface);
}
static void
rp_filter_manager_connection_default_init(RpFilterManagerConnectionInterface* iface G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", iface);
}

struct _RpNetworkFilterManagerImpl {
    GObject parent_instance;

    RpFilterManagerConnection* m_connection;
    RpSocket* m_socket;

    GSList* m_upstream_filters;
    GSList* m_downstream_filters;
};

G_DEFINE_FINAL_TYPE(RpNetworkFilterManagerImpl, rp_network_filter_manager_impl, G_TYPE_OBJECT)

static void
free_read_filter(RpNetworkReadFilter* self)
{
    NOISY_MSG_("(%p(%s))", self, G_OBJECT_TYPE_NAME(self));
    g_object_unref(self);
}

static void
free_write_filter(RpNetworkWriteFilter* self)
{
    NOISY_MSG_("(%p(%s))", self, G_OBJECT_TYPE_NAME(self));
    g_object_unref(self);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpNetworkFilterManagerImpl* self = RP_NETWORK_FILTER_MANAGER_IMPL(obj);
    g_slist_free_full(g_steal_pointer(&self->m_downstream_filters), (GDestroyNotify)/*g_object_unref*/free_write_filter);
    g_slist_free_full(g_steal_pointer(&self->m_upstream_filters), (GDestroyNotify)/*g_object_unref*/free_read_filter);

    G_OBJECT_CLASS(rp_network_filter_manager_impl_parent_class)->dispose(obj);
}

static void
rp_network_filter_manager_impl_class_init(RpNetworkFilterManagerImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_network_filter_manager_impl_init(RpNetworkFilterManagerImpl* self)
{
    LOGD("(%p)", self);

    self->m_downstream_filters = NULL;
    self->m_upstream_filters = NULL;
}

RpNetworkFilterManagerImpl*
rp_network_filter_manager_impl_new(RpFilterManagerConnection* connection, RpSocket* socket)
{
    NOISY_MSG_("(%p, %p)", connection, socket);
    g_return_val_if_fail(RP_IS_NETWORK_CONNECTION(connection), NULL);
    g_return_val_if_fail(RP_IS_SOCKET(socket), NULL);
    RpNetworkFilterManagerImpl* self = g_object_new(RP_TYPE_NETWORK_FILTER_MANAGER_IMPL, NULL);
    self->m_connection = connection;
    self->m_socket = socket;
    return self;
}

void
rp_network_filter_manager_impl_add_write_filter(RpNetworkFilterManagerImpl* self, RpNetworkWriteFilter* filter)
{
    LOGD("(%p, %p(%s))", self, filter, G_OBJECT_TYPE_NAME(filter));
    g_return_if_fail(RP_IS_NETWORK_FILTER_MANAGER_IMPL(self));
    g_return_if_fail(RP_IS_NETWORK_WRITE_FILTER(filter));
    RpNfmiActiveWriteFilter* new_filter = rp_nfmi_active_write_filter_new(self, filter);
    rp_network_write_filter_initialize_write_filter_callbacks(filter, RP_NETWORK_WRITE_FILTER_CALLBACKS(new_filter));
    self->m_downstream_filters = g_slist_append(self->m_downstream_filters, new_filter);
}

void
rp_network_filter_manager_impl_add_read_filter(RpNetworkFilterManagerImpl* self, RpNetworkReadFilter* filter)
{
    LOGD("(%p, %p(%s))", self, filter, G_OBJECT_TYPE_NAME(filter));
    g_return_if_fail(RP_IS_NETWORK_FILTER_MANAGER_IMPL(self));
    g_return_if_fail(RP_IS_NETWORK_READ_FILTER(filter));
    RpNfmiActiveReadFilter* new_filter = rp_nfmi_active_read_filter_new(self, filter);
    rp_network_read_filter_initialize_read_filter_callbacks(filter, RP_NETWORK_READ_FILTER_CALLBACKS(new_filter));
    self->m_upstream_filters = g_slist_append(self->m_upstream_filters, new_filter);
}

bool
rp_network_filter_manager_impl_initialize_read_filters(RpNetworkFilterManagerImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_NETWORK_FILTER_MANAGER_IMPL(self), false);
    if (!self->m_upstream_filters)
    {
        NOISY_MSG_("returning false");
        return false;
    }

    RpNetworkConnection* connection = RP_NETWORK_CONNECTION(self->m_connection);
    for (GSList* entry = self->m_upstream_filters; entry; entry = entry->next)
    {
        RpNfmiActiveReadFilter* filter = RP_NFMI_ACTIVE_READ_FILTER(entry->data);
        if (filter && !rp_nfmi_active_read_filter_initialized(filter))
        {
            rp_nfmi_active_read_filter_set_initialized(filter, true);
            RpNetworkFilterStatus_e status = rp_network_read_filter_on_new_connection(rp_nfmi_active_read_filter_get_filter(filter));
            if (status == RpNetworkFilterStatus_StopIteration ||
                rp_network_connection_state(connection) != RpNetworkConnectionState_Open)
            {
                NOISY_MSG_("breaking");
                break;
            }
        }
    }

    NOISY_MSG_("returning true");
    return true;
}

void
rp_nfmi_on_continue_reading(RpNetworkFilterManagerImpl* self, RpNfmiActiveReadFilter* filter, RpReadBufferSource* buffer_source)
{
    NOISY_MSG_("(%p, %p, %p)", self, filter, buffer_source);

    RpNetworkConnection* connection = RP_NETWORK_CONNECTION(self->m_connection);
    if (rp_network_connection_state(connection) != RpNetworkConnectionState_Open)
    {
        LOGD("not open");
        return;
    }

    GSList* entry;
    if (!filter)
    {
        //TODO...addBytesReceived()
        entry = self->m_upstream_filters;
    }
    else
    {
        // Pick up where we left off.
        entry = (rp_nfmi_active_read_filter_get_entry(filter))->next;
    }

    for (; entry; entry = entry->next)
    {
        if (!entry->data)
        {
            NOISY_MSG_("continuing");
            continue;
        }
        RpNfmiActiveReadFilter* filter_ = RP_NFMI_ACTIVE_READ_FILTER(entry->data);
        RpNetworkReadFilter* wrapped_filter = rp_nfmi_active_read_filter_get_filter(filter_);
        if (!rp_nfmi_active_read_filter_initialized(filter_))
        {
            rp_nfmi_active_read_filter_set_initialized(filter_, true);
            RpNetworkFilterStatus_e status = rp_network_read_filter_on_new_connection(wrapped_filter);
            if (status == RpNetworkFilterStatus_StopIteration ||
                rp_network_connection_state(connection) != RpNetworkConnectionState_Open)
            {
                NOISY_MSG_("returning");
                return;
            }
        }

        struct RpStreamBuffer read_buffer = rp_read_buffer_source_get_read_buffer(buffer_source);
        evbuf_t* buffer = read_buffer.m_buffer;
NOISY_MSG_("buffer %p(%zu), end stream %u", buffer, buffer ? evbuffer_get_length(buffer) : 0, read_buffer.m_end_stream);
        if ((buffer && evbuffer_get_length(buffer) > 0) || read_buffer.m_end_stream)
        {
            RpNetworkFilterStatus_e status = rp_network_read_filter_on_data(wrapped_filter,
                                                                    buffer,
                                                                    read_buffer.m_end_stream);
            if (status == RpNetworkFilterStatus_StopIteration ||
                rp_network_connection_state(connection) != RpNetworkConnectionState_Open)
            {
                NOISY_MSG_("returning");
                return;
            }
        }
    }

    NOISY_MSG_("done");
}

void
rp_network_filter_manager_impl_on_read(RpNetworkFilterManagerImpl* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_NETWORK_FILTER_MANAGER_IMPL(self));
    rp_nfmi_on_continue_reading(self, NULL, RP_READ_BUFFER_SOURCE(self->m_connection));
}

bool
rp_network_filter_manager_impl_start_upstream_secure_transport(RpNetworkFilterManagerImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_NETWORK_FILTER_MANAGER_IMPL(self), false);
    for (GSList* entry = self->m_upstream_filters; entry; entry = entry->next)
    {
        if (!entry->data)
        {
            continue;
        }
        RpNfmiActiveReadFilter* filter = RP_NFMI_ACTIVE_READ_FILTER(entry->data);
        RpNetworkReadFilter* wrapped_filter = rp_nfmi_active_read_filter_get_filter(filter);
        if (rp_network_read_filter_start_upstream_secure_transport(wrapped_filter))
        {
            NOISY_MSG_("yep");
            return true;
        }
    }
    NOISY_MSG_("nope");
    return false;
}

static RpNetworkFilterStatus_e
on_write(RpNetworkFilterManagerImpl* self, RpNfmiActiveWriteFilter* filter, RpWriteBufferSource* buffer_source)
{
    NOISY_MSG_("(%p, %p, %p)", self, filter, buffer_source);

    RpNetworkConnection* connection = RP_NETWORK_CONNECTION(self->m_connection);
    if (rp_network_connection_state(connection) != RpNetworkConnectionState_Open)
    {
        NOISY_MSG_("not open");
        return RpNetworkFilterStatus_StopIteration;
    }

    GSList* entry;
    if (!filter)
    {
        entry = self->m_downstream_filters;
    }
    else
    {
        entry = (rp_nfmi_active_write_filter_get_entry(filter))->next;
    }

    for (; entry; entry = entry->next)
    {
        struct RpStreamBuffer write_buffer = rp_write_buffer_source_get_write_buffer(buffer_source);
        RpNfmiActiveWriteFilter* filter_ = RP_NFMI_ACTIVE_WRITE_FILTER(entry->data);
        RpNetworkWriteFilter* wrapped_filter = rp_nfmi_active_write_filter_get_filter(filter_);
        RpNetworkFilterStatus_e status = rp_network_write_filter_on_write(wrapped_filter,
                                                        write_buffer.m_buffer,
                                                        write_buffer.m_end_stream);
        if (status == RpNetworkFilterStatus_StopIteration ||
            rp_network_connection_state(connection) != RpNetworkConnectionState_Open)
        {
            NOISY_MSG_("returning");
            return RpNetworkFilterStatus_StopIteration;
        }
    }

    //TODO...connection_.streamInfo().addBytesSent()
    return RpNetworkFilterStatus_Continue;
}

RpNetworkFilterStatus_e
rp_network_filter_manager_impl_on_write(RpNetworkFilterManagerImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_NETWORK_FILTER_MANAGER_IMPL(self), RpNetworkFilterStatus_Continue);
    return on_write(self, NULL, RP_WRITE_BUFFER_SOURCE(self->m_connection));
}

void
rp_nfmi_on_resume_writing(RpNetworkFilterManagerImpl* self, RpNfmiActiveWriteFilter* filter, RpWriteBufferSource* buffer_source)
{
    NOISY_MSG_("(%p, %p, %p)", self, filter, buffer_source);
    RpNetworkFilterStatus_e status = on_write(self, filter, buffer_source);
    if (status == RpNetworkFilterStatus_Continue)
    {
        struct RpStreamBuffer write_buffer = rp_write_buffer_source_get_write_buffer(buffer_source);
        RpFilterManagerConnection* connection = RP_FILTER_MANAGER_CONNECTION(self->m_connection);
        rp_filter_manager_connection_raw_write(connection,
                                                write_buffer.m_buffer,
                                                write_buffer.m_end_stream);
    }
}

RpNetworkConnection*
rp_nfmi_get_connection(RpNetworkFilterManagerImpl* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_NETWORK_CONNECTION(self->m_connection);
}

RpSocket*
rp_nfmi_get_socket(RpNetworkFilterManagerImpl* self)
{
    NOISY_MSG_("(%p)", self);
    return self->m_socket;
}
