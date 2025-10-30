/*
 * rp-net-filter-mgr-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <evhtp.h>
#include "rp-socket.h"
#include "rp-net-connection.h"
#include "rp-net-filter.h"

G_BEGIN_DECLS

struct RpStreamBuffer {
    evbuf_t* m_buffer;
    bool m_end_stream;
};

static inline struct RpStreamBuffer
rp_stream_buffer_ctor(evbuf_t* data, bool end_stream)
{
    struct RpStreamBuffer self = {
        .m_buffer = data,
        .m_end_stream = end_stream
    };
    return self;
}

/**
 * Interface used to obtain read buffers.
 */
#define RP_TYPE_READ_BUFFER_SOURCE rp_read_buffer_source_get_type()
G_DECLARE_INTERFACE(RpReadBufferSource, rp_read_buffer_source, RP, READ_BUFFER_SOURCE, GObject)

struct _RpReadBufferSourceInterface {
    GTypeInterface parent_iface;

    struct RpStreamBuffer (*get_read_buffer)(RpReadBufferSource*);
};

static inline struct RpStreamBuffer
rp_read_buffer_source_get_read_buffer(RpReadBufferSource* self)
{
    return RP_IS_READ_BUFFER_SOURCE(self) ?
        RP_READ_BUFFER_SOURCE_GET_IFACE(self)->get_read_buffer(self) :
        rp_stream_buffer_ctor(NULL, true);
}


/**
 * Interface used to obtain write buffers.
 */
#define RP_TYPE_WRITE_BUFFER_SOURCE rp_write_buffer_source_get_type()
G_DECLARE_INTERFACE(RpWriteBufferSource, rp_write_buffer_source, RP, WRITE_BUFFER_SOURCE, GObject)

struct _RpWriteBufferSourceInterface {
    GTypeInterface parent_iface;

    struct RpStreamBuffer (*get_write_buffer)(RpWriteBufferSource*);
};

static inline struct RpStreamBuffer
rp_write_buffer_source_get_write_buffer(RpWriteBufferSource* self)
{
    return RP_IS_WRITE_BUFFER_SOURCE(self) ?
        RP_WRITE_BUFFER_SOURCE_GET_IFACE(self)->get_write_buffer(self) :
        rp_stream_buffer_ctor(NULL, true);
}


/**
 * Connection enriched with methods for advanced cases, i.e. write data bypassing filter chain.
 *
 * Since FilterManager is only user of those methods for now, the class is named after it.
 */
#define RP_TYPE_FILTER_MANAGER_CONNECTION rp_filter_manager_connection_get_type()
G_DECLARE_INTERFACE(RpFilterManagerConnection, rp_filter_manager_connection, RP, FILTER_MANAGER_CONNECTION, RpNetworkConnection)

struct _RpFilterManagerConnectionInterface {
    RpNetworkConnectionInterface parent_iface;

    void (*raw_write)(RpFilterManagerConnection*, evbuf_t*, bool);
};

static inline void
rp_filter_manager_connection_raw_write(RpFilterManagerConnection* self, evbuf_t* data, bool end_stream)
{
    if (RP_IS_FILTER_MANAGER_CONNECTION(self))
    {
        RP_FILTER_MANAGER_CONNECTION_GET_IFACE(self)->raw_write(self, data, end_stream);
    }
}


/**
 * This is a filter manager for TCP (L4) filters. It is split out for ease of testing.
 */
#define RP_TYPE_NETWORK_FILTER_MANAGER_IMPL rp_network_filter_manager_impl_get_type()
G_DECLARE_FINAL_TYPE(RpNetworkFilterManagerImpl, rp_network_filter_manager_impl, RP, NETWORK_FILTER_MANAGER_IMPL, GObject)

RpNetworkFilterManagerImpl* rp_network_filter_manager_impl_new(RpFilterManagerConnection* connection,
                                                                RpSocket* socket);
void rp_network_filter_manager_impl_add_write_filter(RpNetworkFilterManagerImpl* self,
                                                        RpNetworkWriteFilter* filter);
void rp_network_filter_manager_impl_add_read_filter(RpNetworkFilterManagerImpl* self,
                                                    RpNetworkReadFilter* filter);
void rp_network_filter_manager_impl_remove_read_filter(RpNetworkFilterManagerImpl* self,
                                                        RpNetworkReadFilter* filter);
bool rp_network_filter_manager_impl_initialize_read_filters(RpNetworkFilterManagerImpl* self);
void rp_network_filter_manager_impl_on_read(RpNetworkFilterManagerImpl* self);
RpNetworkFilterStatus_e rp_network_filter_manager_impl_on_write(RpNetworkFilterManagerImpl* self);
bool rp_network_filter_manager_impl_start_upstream_secure_transport(RpNetworkFilterManagerImpl* self);

G_END_DECLS
