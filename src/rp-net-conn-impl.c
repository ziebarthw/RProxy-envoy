/*
 * rp-net-conn-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_net_conn_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_net_conn_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-stream-info.h"
#include "rp-net-filter.h"
#include "rp-net-filter-mgr-impl.h"
#include "rp-net-conn-impl.h"

typedef struct _RpNetworkConnectionImplPrivate RpNetworkConnectionImplPrivate;
struct _RpNetworkConnectionImplPrivate {

    RpNetworkTransportSocket* m_transport_socket;
    RpConnectionSocket* m_socket;
    RpStreamInfo* m_stream_info;
    UNIQUE_PTR(RpNetworkFilterManagerImpl) m_filter_manager;

    volatile gint m_next_global_id;//TODO...guint64?

    //TODO...std::list<BytesSentCb> bytes_sent_callbacks_;
    const char* m_failure_reason;
    guint64 m_last_read_buffer_size;
    guint64 m_last_write_buffer_size;

    evbuf_t* m_write_buffer;
    evbuf_t* m_read_buffer;
    evbuf_t* m_current_write_buffer;

    guint32 m_read_disable_count;

    RpDelayedCloseState_e m_detected_close_type;
    RpNetworkConnectionEvent_e m_immediate_error_event;

    bool m_write_buffer_above_high_watermark : 1;
    bool m_detect_early_close : 1;
    bool m_enable_half_close : 1;
    bool m_read_end_stream_raised : 1;
    bool m_read_end_stream : 1;
    bool m_current_write_end_stream : 1;
    bool m_write_end_stream : 1;
    bool m_dispatch_buffered_data : 1;
    bool m_tranport_wants_read : 1;

    bool m_connected : 1;
    bool m_connecting : 1;
};

enum
{
    PROP_0, // Reserved.
    PROP_SOCKET,
    PROP_TRANSPORT_SOCKET,
    PROP_CONNECTED,
    PROP_STREAM_INFO,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void read_buffer_source_iface_init(RpReadBufferSourceInterface* iface);
static void write_buffer_source_iface_init(RpWriteBufferSourceInterface* iface);
static void network_filter_manager_iface_init(RpNetworkFilterManagerInterface* iface);
static void network_transport_socket_callbacks_iface_init(RpNetworkTransportSocketCallbacksInterface* iface);
static void network_connection_iface_init(RpNetworkConnectionInterface* iface);
static void filter_manager_connection_iface_init(RpFilterManagerConnectionInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpNetworkConnectionImpl, rp_network_connection_impl, RP_TYPE_NETWORK_CONNECTION_IMPL_BASE,
    G_ADD_PRIVATE(RpNetworkConnectionImpl)
    G_IMPLEMENT_INTERFACE(RP_TYPE_READ_BUFFER_SOURCE, read_buffer_source_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_WRITE_BUFFER_SOURCE, write_buffer_source_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_FILTER_MANAGER, network_filter_manager_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_TRANSPORT_SOCKET_CALLBACKS, network_transport_socket_callbacks_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_FILTER_MANAGER_CONNECTION, filter_manager_connection_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_CONNECTION, /*network_connection_iface_init*/NULL)
)

#define PRIV(obj) \
    ((RpNetworkConnectionImplPrivate*)rp_network_connection_impl_get_instance_private(RP_NETWORK_CONNECTION_IMPL(obj)))
#define PARENT_NETWORK_CONNECTION_IFACE(s) \
    ((RpNetworkConnectionInterface*)g_type_interface_peek_parent(RP_NETWORK_CONNECTION_GET_IFACE(s)))
#define SOCKET(s) PRIV(s)->m_socket
#define RPSOCKET(s) RP_SOCKET(SOCKET(s))
#define SOCKFD(s) rp_socket_sockfd(RPSOCKET(s))

static bool
filter_chain_wants_data(RpNetworkConnectionImplPrivate* me)
{
    NOISY_MSG_("(%p)", me);
NOISY_MSG_("returning %s", me->m_read_disable_count == 0 || (me->m_read_disable_count == 1 && false) ? "true" : "false");
    return me->m_read_disable_count == 0 ||
        (me->m_read_disable_count == 1 /*TODO...&& read_buffer_->highWatermarkTriggered()*/ && false);
}

static void
close_socket(RpNetworkConnectionImpl* self, RpNetworkConnectionEvent_e close_type)
{
    NOISY_MSG_("(%p(fd %d), %d)", self, SOCKFD(self), close_type);
    RpNetworkConnectionImplPrivate* me = PRIV(self);
    if (!rp_socket_is_open(RP_SOCKET(me->m_socket)))
    {
        NOISY_MSG_("not open");
        return;
    }

    //TODO...if (delayed_close_timer_)...

    rp_network_transport_socket_close_socket(me->m_transport_socket);

    rp_socket_close(RP_SOCKET(me->m_socket));

    rp_network_connection_impl_raise_event(self, close_type);
}

// REVISIT...not sure why this is a separate function?
static inline void
set_detected_close_type(RpNetworkConnectionImplPrivate* me, RpDetectedCloseType_e close_type)
{
    NOISY_MSG_("(%p, %d)", me, close_type);
    me->m_detected_close_type = close_type;
}

static inline bool
both_sides_half_closed(RpNetworkConnectionImplPrivate* me)
{
    NOISY_MSG_("(%p)", me);
    // If the write_buffer_ is not empty, then the end_stream has not been sent to the transport yet.
    return me->m_read_end_stream && me->m_write_end_stream && evbuffer_get_length(me->m_write_buffer) == 0;
}

static void
on_write_ready(RpNetworkConnectionImpl* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));

    RpNetworkConnectionImplPrivate* me = PRIV(self);

    if (me->m_connecting)
    {
        //TODO...check errors?
        NOISY_MSG_("connected on fd %d", SOCKFD(self));
        me->m_connecting = false;
        RP_NETWORK_CONNECTION_IMPL_GET_CLASS(self)->on_connected(self);
        if (rp_network_connection_state(RP_NETWORK_CONNECTION(self)) != RpNetworkConnectionState_Open)
        {
            LOGD("connection closed");
            return;
        }
    }

NOISY_MSG_("\n\"%.*s\"", (int)evbuffer_get_length(me->m_write_buffer), (char*)evbuffer_pullup(me->m_write_buffer, -1));

    RpIoResult result = rp_network_transport_socket_do_write(me->m_transport_socket, me->m_write_buffer, me->m_write_end_stream);
    guint64 new_buffer_size G_GNUC_UNUSED = evbuffer_get_length(me->m_write_buffer);
    //TODO...updateWriteBufferStats(...)
NOISY_MSG_("%zu bytes written, new buffer size %zu", result.m_bytes_processed, new_buffer_size);

    if (result.m_err_code == ECONNRESET)
    {
        LOGD("write: rst close from peer.");
        set_detected_close_type(me, RpDetectedCloseType_RemoteReset);
        close_socket(self, RpNetworkConnectionEvent_RemoteClose);
        return;
    }

    if (result.m_action == RpPostIoAction_Close)
    {
        NOISY_MSG_("closing");
        close_socket(self, RpNetworkConnectionEvent_RemoteClose);
    }
    //TODO...
NOISY_MSG_("done");
}

static struct RpStreamBuffer
get_read_buffer_i(RpReadBufferSource* self)
{
    RpNetworkConnectionImplPrivate* me = PRIV(self);
    return rp_stream_buffer_ctor(me->m_read_buffer, me->m_read_end_stream);
}

static void
read_buffer_source_iface_init(RpReadBufferSourceInterface* iface)
{
    LOGD("(%p)", iface);
    iface->get_read_buffer = get_read_buffer_i;
}

static struct RpStreamBuffer
get_write_buffer_i(RpWriteBufferSource* self)
{
    RpNetworkConnectionImplPrivate* me = PRIV(self);
    return rp_stream_buffer_ctor(me->m_current_write_buffer, me->m_current_write_end_stream);
}

static void
write_buffer_source_iface_init(RpWriteBufferSourceInterface* iface)
{
    LOGD("(%p)", iface);
    iface->get_write_buffer = get_write_buffer_i;
}

static void
write_internal(RpNetworkConnectionImpl* self, evbuf_t* data, bool end_stream, bool through_filter_chain)
{
    NOISY_MSG_("(%p(fd %d), %p(%zu), %u, %u)",
        self, SOCKFD(self), data, evbuf_length(data), end_stream, through_filter_chain);

    RpNetworkConnectionImplPrivate* me = PRIV(self);

    if (me->m_write_end_stream)
    {
        NOISY_MSG_("write end stream");
        return;
    }

    if (through_filter_chain)
    {
        me->m_current_write_buffer = data;
        me->m_current_write_end_stream = end_stream;
        RpNetworkFilterStatus_e status = rp_network_filter_manager_impl_on_write(me->m_filter_manager);
        me->m_current_write_buffer = NULL;

        if (status == RpNetworkFilterStatus_StopIteration)
        {
            NOISY_MSG_("returning");
            return;
        }
    }

    me->m_write_end_stream = end_stream;
    if ((evbuf_length(data) > 0) || end_stream)
    {
        NOISY_MSG_("writing %zu bytes, end_stream %u", evbuffer_get_length(data), end_stream);
        evbuffer_add_buffer(me->m_write_buffer, data);

        if (!me->m_connecting)
        {
            NOISY_MSG_("enabling write");
//#define WITH_ACTIVATE_FILE_EVENTS
#ifdef WITH_ACTIVATE_FILE_EVENTS
            rp_io_handle_activate_file_events(
                rp_socket_io_handle(RP_SOCKET(me->m_socket)), RpFileReadyType_Write);
#else
            on_write_ready(self);
#endif//WITH_ACTIVATE_FILE_EVENTS
        }
    }
}

static void
raw_write_i(RpFilterManagerConnection* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuf_length(data), end_stream);
    write_internal(RP_NETWORK_CONNECTION_IMPL(self), data, end_stream, false);
}

static void
filter_manager_connection_iface_init(RpFilterManagerConnectionInterface* iface)
{
    LOGD("(%p)", iface);
    iface->raw_write = raw_write_i;
}

static void
add_read_filter_i(RpNetworkFilterManager* self, RpNetworkReadFilter* filter)
{
    NOISY_MSG_("(%p, %p(%s))", self, filter, G_OBJECT_TYPE_NAME(filter));
    rp_network_filter_manager_impl_add_read_filter(PRIV(self)->m_filter_manager, filter);
}

static void
add_write_filter_i(RpNetworkFilterManager* self, RpNetworkWriteFilter* filter)
{
    NOISY_MSG_("(%p, %p(%s))", self, filter, G_OBJECT_TYPE_NAME(filter));
    rp_network_filter_manager_impl_add_write_filter(PRIV(self)->m_filter_manager, filter);
}

static bool
initialize_read_filters_i(RpNetworkFilterManager* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
    return rp_network_filter_manager_impl_initialize_read_filters(PRIV(self)->m_filter_manager);
}

static void
network_filter_manager_iface_init(RpNetworkFilterManagerInterface* iface)
{
    LOGD("(%p)", iface);
    iface->add_read_filter = add_read_filter_i;
    iface->add_write_filter = add_write_filter_i;
    iface->initialize_read_filters = initialize_read_filters_i;
}

static void
write_i(RpNetworkConnection* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p(fd %d), %p(%zu), %u)",
        self, SOCKFD(self), data, evbuf_length(data), end_stream);
    write_internal(RP_NETWORK_CONNECTION_IMPL(self), data, end_stream, true);
}

static void
no_delay_i(RpNetworkConnection* self, bool enable)
{
    NOISY_MSG_("(%p, %u)", self, enable);

    //TODO...
}

static RpNetworkConnectionState_e
state_i(RpNetworkConnection* self)
{
    NOISY_MSG_("(%p)", self);
    RpNetworkConnectionImplPrivate* me = PRIV(self);
    if (!rp_socket_is_open(RP_SOCKET(me->m_socket)))
    {
        NOISY_MSG_("closed");
        return RpNetworkConnectionState_Closed;
    }
    else if (rp_network_connection_impl_base_in_delayed_close(RP_NETWORK_CONNECTION_IMPL_BASE(self)))
    {
        NOISY_MSG_("closing");
        return RpNetworkConnectionState_Closing;
    }
    else
    {
        NOISY_MSG_("open");
        return RpNetworkConnectionState_Open;
    }
}

static RpConnectionInfoProvider*
connection_info_provider_i(RpNetworkConnection* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CONNECTION_INFO_PROVIDER(
            rp_socket_connection_info_provider(RP_SOCKET(PRIV(self)->m_socket)));
}

static bool
connecting_i(RpNetworkConnection* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_connecting;
}

static const char*
transport_failure_reason_i(RpNetworkConnection* self)
{
    NOISY_MSG_("(%p)", self);
    RpNetworkConnectionImplPrivate* me = PRIV(self);
    if (me->m_failure_reason && me->m_failure_reason[0])
    {
        return me->m_failure_reason;
    }
//TODO...return transport_socket_->failureReason();
return "";
}

static RpStreamInfo*
stream_info_i(RpNetworkConnection* self)
{
    NOISY_MSG_("(%p)", self);
NOISY_MSG_("stream info %p", PRIV(self)->m_stream_info);
    return PRIV(self)->m_stream_info;
}

static RpSslConnectionInfo*
ssl_i(RpNetworkConnection* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_connection_info_provider_ssl_connection(RP_CONNECTION_INFO_PROVIDER(
            rp_socket_connection_info_provider(RP_SOCKET(PRIV(self)->m_socket))));
}

static void
close_internal(RpNetworkConnectionImpl* self, RpNetworkConnectionCloseType_e type)
{
    NOISY_MSG_("(%p(fd %d), %d)", self, SOCKFD(self), type);
    RpNetworkConnectionImplPrivate* me = PRIV(self);
    if (!rp_socket_is_open(RP_SOCKET(me->m_socket)))
    {
        NOISY_MSG_("not open");
        return;
    }

    gsize data_to_write = evbuffer_get_length(me->m_write_buffer);
    NOISY_MSG_("%zu bytes to write to fd %d", data_to_write, SOCKFD(self));

    if (data_to_write == 0 || type == RpNetworkConnectionCloseType_NoFlush ||
        type == RpNetworkConnectionCloseType_Abort || !rp_network_transport_socket_can_flush_close(me->m_transport_socket))
    {
        if (data_to_write > 0 && type != RpNetworkConnectionCloseType_Abort)
        {
            rp_network_transport_socket_do_write(me->m_transport_socket, me->m_write_buffer, true);
        }

        if (type != RpNetworkConnectionCloseType_FlushWriteAndDelay)
        {
            NOISY_MSG_("closing");
            rp_network_connection_impl_base_close_connection_immediately(RP_NETWORK_CONNECTION_IMPL_BASE(self));
            return;
        }

        //TODO...
        return;
    }

    //TODO...
}

static void
close_i(RpNetworkConnection* self, RpNetworkConnectionCloseType_e type)
{
    NOISY_MSG_("(%p(fd %d), %d)", self, SOCKFD(self), type);
    RpNetworkConnectionImplPrivate* me = PRIV(self);
    if (!rp_socket_is_open(RP_SOCKET(me->m_socket)))
    {
        NOISY_MSG_("not open");
        return;
    }

    if (type == RpNetworkConnectionCloseType_AbortReset)
    {
        LOGD("connection closing type=AbortReset, setting LocalReset to the detected close type");
        set_detected_close_type(me, RpDetectedCloseType_LocalReset);
        close_socket(RP_NETWORK_CONNECTION_IMPL(self), RpNetworkConnectionEvent_LocalClose);
        return;
    }

    if (type == RpNetworkConnectionCloseType_Abort || type == RpNetworkConnectionCloseType_NoFlush)
    {
        close_internal(RP_NETWORK_CONNECTION_IMPL(self), type);
        return;
    }

NOISY_MSG_("calling rp_io_handle_enable_file_events(%p, %u)", me->m_socket, RpFileReadyType_Write | (me->m_enable_half_close ? 0 : RpFileReadyType_Closed));
    rp_io_handle_enable_file_events(rp_socket_io_handle(RP_SOCKET(me->m_socket)), RpFileReadyType_Write | (me->m_enable_half_close ? 0 : RpFileReadyType_Closed));
}

static void
enable_half_close_i(RpNetworkConnection* self, bool enabled)
{
    NOISY_MSG_("(%p, %u)", self, enabled);
    PRIV(self)->m_enable_half_close = enabled;
}

static RpReadDisabaleStatus_e
read_disable_i(RpNetworkConnection* self, bool disable)
{
    NOISY_MSG_("(%p(fd %d), %u)", self, SOCKFD(self), disable);
    RpNetworkConnectionImplPrivate* me = PRIV(self);
    if (disable)
    {
        ++me->m_read_disable_count;
NOISY_MSG_("read disable count %u", me->m_read_disable_count);

        if (rp_network_connection_state(self) != RpNetworkConnectionState_Open)
        {
            LOGD("called on closed connection");
            return RpReadDisabaleStatus_NoTransition;
        }

        if (me->m_read_disable_count > 1)
        {
            LOGD("already disabled");
            return RpReadDisabaleStatus_StillReadDisabled;
        }

        if (me->m_detect_early_close && !me->m_enable_half_close)
        {
            NOISY_MSG_("enabling write and closed");
            rp_io_handle_enable_file_events(
                rp_socket_io_handle(RP_SOCKET(me->m_socket)), RpFileReadyType_Write|RpFileReadyType_Closed);
        }
        else
        {
            NOISY_MSG_("enabling write");
            rp_io_handle_enable_file_events(
                rp_socket_io_handle(RP_SOCKET(me->m_socket)), RpFileReadyType_Write);
        }

        return RpReadDisabaleStatus_TransitionedToReadDisabled;
    }
    else
    {
        g_assert(me->m_read_disable_count != 0);
        --me->m_read_disable_count;
NOISY_MSG_("read disable count %u", me->m_read_disable_count);
        if (rp_network_connection_state(self) != RpNetworkConnectionState_Open)
        {
            LOGD("called on closed connection");
            return RpReadDisabaleStatus_NoTransition;
        }

        RpReadDisabaleStatus_e read_disable_status = RpReadDisabaleStatus_StillReadDisabled;
        if (me->m_read_disable_count == 0)
        {
            NOISY_MSG_("enabling read and write");
            rp_io_handle_enable_file_events(
                rp_socket_io_handle(RP_SOCKET(me->m_socket)), RpFileReadyType_Read|RpFileReadyType_Write);
            read_disable_status = RpReadDisabaleStatus_TransitionedToReadEnabled;
        }

        if (filter_chain_wants_data(me) &&
            (evbuf_length(me->m_read_buffer) > 0 || me->m_tranport_wants_read))
        {
            NOISY_MSG_("activating read");
            me->m_dispatch_buffered_data = true;
            rp_io_handle_activate_file_events(
                rp_socket_io_handle(RP_SOCKET(me->m_socket)), RpFileReadyType_Read);
        }

        return read_disable_status;
    }
}

static bool
read_enabled_i(RpNetworkConnection* self)
{
    NOISY_MSG_("(%p)", self);
    g_assert(rp_network_connection_state(self) == RpNetworkConnectionState_Open);
    return PRIV(self)->m_read_disable_count == 0;
}

static RpDispatcher*
dispatcher_i(RpNetworkConnection* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_NETWORK_CONNECTION_IFACE(self)->dispatcher(self);
}

static void
add_connection_callbacks_i(RpNetworkConnection* self, RpNetworkConnectionCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    PARENT_NETWORK_CONNECTION_IFACE(self)->add_connection_callbacks(self, callbacks);
}

static void
remove_connection_callbacks_i(RpNetworkConnection* self, RpNetworkConnectionCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    PARENT_NETWORK_CONNECTION_IFACE(self)->remove_connection_callbacks(self, callbacks);
}

static const char*
local_close_reason_i(RpNetworkConnection* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_NETWORK_CONNECTION_IFACE(self)->local_close_reason(self);
}

static guint64
id_i(RpNetworkConnection* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_NETWORK_CONNECTION_IFACE(self)->id(self);
}

static void
detect_early_close_when_read_disabled_i(RpNetworkConnection* self, bool value)
{
    NOISY_MSG_("(%p, %u)", self, value);
    PRIV(self)->m_detect_early_close = value;
}

static RpConnectionInfoSetter*
connection_info_setter_i(RpNetworkConnection* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_socket_connection_info_provider(RPSOCKET(self));
}

static int
sockfd_i(RpNetworkConnection* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_socket_sockfd(RPSOCKET(self));
}

static void
network_connection_iface_init(RpNetworkConnectionInterface* iface)
{
    LOGD("(%p)", iface);
    iface->no_delay = no_delay_i;
    iface->state = state_i;
    iface->write = write_i;
    iface->connection_info_provider = connection_info_provider_i;
    iface->connecting = connecting_i;
    iface->transport_failure_reason = transport_failure_reason_i;
    iface->stream_info = stream_info_i;
    iface->ssl = ssl_i;
    iface->close = close_i;
    iface->enable_half_close = enable_half_close_i;
    iface->read_disable = read_disable_i;
    iface->read_enabled = read_enabled_i;
    iface->dispatcher = dispatcher_i;
    iface->add_connection_callbacks = add_connection_callbacks_i;
    iface->remove_connection_callbacks = remove_connection_callbacks_i;
    iface->local_close_reason = local_close_reason_i;
    iface->id = id_i;
    iface->detect_early_close_when_read_disabled = detect_early_close_when_read_disabled_i;
    iface->connection_info_setter = connection_info_setter_i;
    iface->sockfd = sockfd_i;
}

static RpNetworkConnection*
connection_i(RpNetworkTransportSocketCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_NETWORK_CONNECTION(self);
}

static RpIoHandle*
io_handle_i(RpNetworkTransportSocketCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_socket_io_handle(RP_SOCKET(PRIV(self)->m_socket));
}

static bool
should_drain_read_buffer_i(RpNetworkTransportSocketCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
//TODO...
return false;
}

static void
set_transport_socket_is_readable_i(RpNetworkTransportSocketCallbacks* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
    RpNetworkConnectionImplPrivate* me = PRIV(self);
    me->m_tranport_wants_read = true;
    if (me->m_read_disable_count == 0)
    {
        RpIoHandle* io_handle = rp_socket_io_handle(RP_SOCKET(me->m_socket));
        rp_io_handle_activate_file_events(io_handle, RpFileReadyType_Read);
    }
}

static void
flush_write_buffer_i(RpNetworkTransportSocketCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    if (rp_network_connection_state(RP_NETWORK_CONNECTION(self)) == RpNetworkConnectionState_Open &&
        evbuf_length(PRIV(self)->m_write_buffer) > 0)
    {
        on_write_ready(RP_NETWORK_CONNECTION_IMPL(self));
    }
}

static void
raise_event_i(RpNetworkTransportSocketCallbacks* self, RpNetworkConnectionEvent_e event)
{
    NOISY_MSG_("(%p(fd %d), %d)", self, SOCKFD(self), event);
    rp_network_connection_impl_raise_event(RP_NETWORK_CONNECTION_IMPL(self), event);
}

static void
network_transport_socket_callbacks_iface_init(RpNetworkTransportSocketCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->connection = connection_i;
    iface->io_handle = io_handle_i;
    iface->raise_event = raise_event_i;
    iface->should_drain_read_buffer = should_drain_read_buffer_i;
    iface->set_transport_socket_is_readable = set_transport_socket_is_readable_i;
    iface->flush_write_buffer = flush_write_buffer_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_CONNECTED:
            g_value_set_boolean(value, PRIV(obj)->m_connected);
            break;
        case PROP_SOCKET:
            g_value_set_object(value, PRIV(obj)->m_socket);
            break;
        case PROP_TRANSPORT_SOCKET:
            g_value_set_object(value, PRIV(obj)->m_transport_socket);
            break;
        case PROP_STREAM_INFO:
            g_value_set_object(value, PRIV(obj)->m_stream_info);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_CONNECTED:
            PRIV(obj)->m_connected = g_value_get_boolean(value);
            break;
        case PROP_SOCKET:
            PRIV(obj)->m_socket = g_value_get_object(value);
            break;
        case PROP_TRANSPORT_SOCKET:
            PRIV(obj)->m_transport_socket = g_value_get_object(value);
            break;
        case PROP_STREAM_INFO:
            PRIV(obj)->m_stream_info = g_value_get_object(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static inline bool
in_delayed_close(RpNetworkConnectionImplPrivate* me)
{
    //TODO...return delayed_close_state_ != DelayedCloseState::None;
    return false;
}

static void
on_read(RpNetworkConnectionImplPrivate* me, guint64 read_buffer_size)
{
    NOISY_MSG_("(%p, %zu)", me, read_buffer_size);
    if (in_delayed_close(me) || !filter_chain_wants_data(me))
    {
        NOISY_MSG_("returning");
        return;
    }

    if (read_buffer_size == 0 && !me->m_read_end_stream)
    {
        NOISY_MSG_("returning");
        return;
    }

    if (me->m_read_end_stream)
    {
        if (me->m_read_end_stream_raised)
        {
            NOISY_MSG_("returning");
            return;
        }
        me->m_read_end_stream_raised = true;
    }

    rp_network_filter_manager_impl_on_read(me->m_filter_manager);
}

static void
on_read_ready(RpNetworkConnectionImpl* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));

    RpNetworkConnectionImplPrivate* me = PRIV(self);
    bool latched_dispatch_buffered_data = me->m_dispatch_buffered_data;
    me->m_dispatch_buffered_data = false;

    //TODO...if (enable_close_through_filter_manager_...)

    if (me->m_read_disable_count != 0)
    {
        NOISY_MSG_("read disabled count %u", me->m_read_disable_count);
        if (latched_dispatch_buffered_data && filter_chain_wants_data(me))
        {
            NOISY_MSG_("calling on_read(%p, %zu)", me, evbuffer_get_length(me->m_read_buffer));
            on_read(me, evbuffer_get_length(me->m_read_buffer));
        }
        return;
    }

    me->m_tranport_wants_read = false;
    RpIoResult result = rp_network_transport_socket_do_read(me->m_transport_socket, me->m_read_buffer);
    guint64 new_buffer_size = evbuffer_get_length(me->m_read_buffer);
    //TODO...updateReadStats(result.bytes_processed_, new_buffer_size);

    if (result.m_err_code && result.m_err_code == ECONNRESET)
    {
        LOGD("read: rst close from peer");
        if (result.m_bytes_processed != 0)
        {
            NOISY_MSG_("calling on_read(%p, %zu)", me, new_buffer_size);
            on_read(me, new_buffer_size);
        }
        set_detected_close_type(me, RpDetectedCloseType_RemoteReset);
        close_socket(self, RpNetworkConnectionEvent_RemoteClose);
        return;
    }

    if ((!me->m_enable_half_close && result.m_end_stream_read))
    {
        NOISY_MSG_("setting to close");
        result.m_end_stream_read = false;
        result.m_action = RpPostIoAction_Close;
    }

    me->m_read_end_stream |= result.m_end_stream_read;
    if (result.m_bytes_processed != 0 ||
        result.m_end_stream_read ||
        (latched_dispatch_buffered_data && evbuffer_get_length(me->m_read_buffer) > 0))
    {
        NOISY_MSG_("calling on_read(%p, %zu)", me, new_buffer_size);
        on_read(me, new_buffer_size);
    }

    if (result.m_action == RpPostIoAction_Close || both_sides_half_closed(me))
    {
        NOISY_MSG_("remote close");
        close_socket(self, RpNetworkConnectionEvent_RemoteClose);
    }

NOISY_MSG_("done");
}

static void
on_file_event(gpointer arg, guint32 events)
{
    NOISY_MSG_("(%p(fd %d), %u)", arg, SOCKFD(arg), events);

    RpNetworkConnectionImpl* self = RP_NETWORK_CONNECTION_IMPL(arg);
    RpNetworkConnectionImplPrivate* me = PRIV(self);

    if (me->m_immediate_error_event == RpNetworkConnectionEvent_LocalClose ||
        me->m_immediate_error_event == RpNetworkConnectionEvent_RemoteClose)
    {
        NOISY_MSG_("calling close_socket(%p, %d)", self, me->m_immediate_error_event);
        close_socket(self, me->m_immediate_error_event);
        return;
    }

    if (events & RpFileReadyType_Closed)
    {
        LOGD("remote early close");
        g_assert(!(events & RpFileReadyType_Read));
        close_socket(self, RpNetworkConnectionEvent_RemoteClose);
        return;
    }

    if (events & RpFileReadyType_Write)
    {
        on_write_ready(self);
    }

    if (rp_socket_is_open(RP_SOCKET(me->m_socket)) && (events & RpFileReadyType_Read))
    {
        on_read_ready(self);
    }

NOISY_MSG_("done");
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_network_connection_impl_parent_class)->constructed(obj);

    RpNetworkConnectionImplPrivate* me = PRIV(obj);
    me->m_filter_manager = rp_network_filter_manager_impl_new(RP_FILTER_MANAGER_CONNECTION(obj), RP_SOCKET(me->m_socket));

    g_object_ref(me->m_socket);

    if (!me->m_connected)
    {
        NOISY_MSG_("connecting");
        me->m_connecting = true;
    }

    RpIoHandle* io_handle = rp_socket_io_handle(RP_SOCKET(me->m_socket));
    RpDispatcher* dispatcher = rp_network_connection_impl_base_dispatcher_(RP_NETWORK_CONNECTION_IMPL_BASE(obj));
    rp_io_handle_initialize_file_event(io_handle,
                                        dispatcher,
                                        on_file_event,
                                        obj,
                                        RpFileReadyType_Read | RpFileReadyType_Write);

    rp_network_transport_socket_set_transport_socket_callbacks(me->m_transport_socket,
                                                                RP_NETWORK_TRANSPORT_SOCKET_CALLBACKS(obj));

    //TODO...setConnectionId(id());
    rp_connection_info_setter_set_ssl_connection(
        rp_socket_connection_info_provider(RP_SOCKET(me->m_socket)),
            rp_network_transport_socket_ssl(me->m_transport_socket));
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpNetworkConnectionImplPrivate* me = PRIV(obj);
NOISY_MSG_("clearing filter manager %p", me->m_filter_manager);
    g_clear_object(&me->m_filter_manager);
    g_clear_object(&me->m_socket);
    g_clear_pointer(&me->m_read_buffer, evbuffer_free);
    g_clear_pointer(&me->m_write_buffer, evbuffer_free);

    G_OBJECT_CLASS(rp_network_connection_impl_parent_class)->dispose(obj);
}

OVERRIDE void
close_connection_immediately(RpNetworkConnectionImplBase* self)
{
    NOISY_MSG_("(%p)", self);
    close_socket(RP_NETWORK_CONNECTION_IMPL(self), RpNetworkConnectionEvent_LocalClose);
}

static inline void
network_connection_impl_base_class_init(RpNetworkConnectionImplBaseClass* klass)
{
    LOGD("(%p)", klass);

    klass->close_connection_immediately = close_connection_immediately;
    network_connection_iface_init(g_type_interface_peek(klass, RP_TYPE_NETWORK_CONNECTION));
//    filter_manager_connection_iface_init(g_type_interface_peek(klass, RP_TYPE_FILTER_MANAGER_CONNECTION));
}

OVERRIDE void
on_connected(RpNetworkConnectionImpl* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
NOISY_MSG_("calling rp_network_transport_socket_on_connected(%p) on fd %d", PRIV(self)->m_transport_socket, SOCKFD(self));
    rp_network_transport_socket_on_connected(PRIV(self)->m_transport_socket);
}

static void
rp_network_connection_impl_class_init(RpNetworkConnectionImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    klass->on_connected = on_connected;

    network_connection_impl_base_class_init(RP_NETWORK_CONNECTION_IMPL_BASE_CLASS(klass));

    obj_properties[PROP_CONNECTED] = g_param_spec_boolean("connected",
                                                    "Connected",
                                                    "Connected flag",
                                                    false,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_SOCKET] = g_param_spec_object("socket",
                                                    "Socket",
                                                    "ConnectionSocket Instance",
                                                    RP_TYPE_CONNECTION_SOCKET,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_TRANSPORT_SOCKET] = g_param_spec_object("transport-socket",
                                                    "Transport socket",
                                                    "Transport Socket Instance",
                                                    RP_TYPE_NETWORK_TRANSPORT_SOCKET,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_STREAM_INFO] = g_param_spec_object("stream-info",
                                                    "Stream info",
                                                    "StreamInfo Instance",
                                                    RP_TYPE_STREAM_INFO,
                                                    G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_network_connection_impl_init(RpNetworkConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);

    RpNetworkConnectionImplPrivate* me = PRIV(self);
    me->m_detected_close_type = RpDetectedCloseType_Normal;
    me->m_immediate_error_event = RpNetworkConnectionEvent_None;
    me->m_write_buffer_above_high_watermark = false;
    me->m_detect_early_close = true;
    me->m_enable_half_close = false;
    me->m_read_end_stream_raised = false;
    me->m_read_end_stream = false;
    me->m_write_end_stream = false;
    me->m_current_write_end_stream = false;
    me->m_dispatch_buffered_data = false;
    me->m_tranport_wants_read = false;
    me->m_connecting = false;
    me->m_connected = false;

    me->m_read_buffer = evbuffer_new();
    me->m_write_buffer = evbuffer_new();
    me->m_current_write_buffer = NULL;
}

RpConnectionSocket*
rp_network_connection_impl_socket_(RpNetworkConnectionImpl* self)
{
    LOGD("(%p)", self);
    return PRIV(self)->m_socket;
}

RpNetworkTransportSocket*
rp_network_connection_impl_transport_socket_(RpNetworkConnectionImpl* self)
{
    LOGD("(%p)", self);
    return PRIV(self)->m_transport_socket;
}

void
rp_network_connection_impl_raise_event(RpNetworkConnectionImpl* self, RpNetworkConnectionEvent_e event)
{
    NOISY_MSG_("(%p(fd %d), %d)", self, SOCKFD(self), event);
    g_return_if_fail(RP_IS_NETWORK_CONNECTION_IMPL(self));
    rp_network_connection_impl_base_raise_connection_event(RP_NETWORK_CONNECTION_IMPL_BASE(self), event);
    if (event == RpNetworkConnectionEvent_Connected)
    {
        NOISY_MSG_("calling rp_network_transport_socket_callbacks_flush_write_buffer(%p)", self);
        rp_network_transport_socket_callbacks_flush_write_buffer(RP_NETWORK_TRANSPORT_SOCKET_CALLBACKS(self));
    }
}

void
rp_network_connection_impl_on_connected_(RpNetworkConnectionImpl* self)
{
    LOGD("(%p(fd %d))", self, SOCKFD(self));
    g_return_if_fail(RP_IS_NETWORK_CONNECTION_IMPL(self));
    on_connected(self);
}
