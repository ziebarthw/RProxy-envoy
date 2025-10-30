/*
 * rp-http-conn-manager-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_conn_manager_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_conn_manager_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-codec.h"
#include "rp-codec-client.h"
#include "rp-header-utility.h"
#include "rp-headers.h"
#include "rp-net-connection.h"
#include "rp-net-filter.h"
#include "rp-http-conn-manager-impl.h"

struct _RpHttpConnectionManagerImpl {
    GObject parent_instance;

    evthr_cb m_deferred_request_processing_callback;

    RpNetworkReadFilterCallbacks* m_read_callbacks;
    RpDispatcher* m_dispatcher;

    RpConnectionManagerConfig* m_config;
    RpLocalInfo* m_local_info;
    RpClusterManager* m_cluster_manager;
    RpHttpServerConnection* m_codec;
    RpDrainState_e m_drain_state;

    GSList* m_streams;

    guint64 m_accumulated_requests;
    guint64 m_closed_non_internally_destroyed_requests;
    guint64 m_number_premature_stream_requests;
    guint32 m_requests_during_dispatch_count;
    guint32 m_max_requests_during_dispatch;

    bool m_soft_drain_http1 : 1;
    bool m_go_away_sent : 1;
    bool m_remote_close : 1;
};

static void network_read_filter_iface_init(RpNetworkReadFilterInterface* iface);
static void network_connection_callbacks_iface_init(RpNetworkConnectionCallbacksInterface* iface);
static void http_server_connection_callbacks_iface_init(RpHttpServerConnectionCallbacksInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpHttpConnectionManagerImpl, rp_http_connection_manager_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_READ_FILTER, network_read_filter_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_CONNECTION_CALLBACKS, network_connection_callbacks_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_HTTP_CONNECTION_CALLBACKS, NULL)
    G_IMPLEMENT_INTERFACE(RP_TYPE_HTTP_SERVER_CONNECTION_CALLBACKS, http_server_connection_callbacks_iface_init)
)

static void
reset_all_streams(RpHttpConnectionManagerImpl* self/*, response_flag*/, const char* details)
{
    NOISY_MSG_("(%p, %p(%s))", self, details, details);
NOISY_MSG_("streams %p", self->m_streams);
    while (self->m_streams)
    {
        RpHttpConnMgrImplActiveStream* stream = self->m_streams->data;
NOISY_MSG_("removing stream %p", stream);
        self->m_streams = g_slist_remove(self->m_streams, stream);
NOISY_MSG_("streams %p", self->m_streams);
        rp_stream_remove_callbacks(
            rp_stream_encoder_get_stream(
                RP_STREAM_ENCODER(rp_http_conn_mgr_impl_active_stream_response_encoder(stream))), RP_STREAM_CALLBACKS(stream));
        //TODO...if (!stream.response_encoder_->getStream().responseDetails().empty())
        rp_stream_callbacks_on_reset_stream(RP_STREAM_CALLBACKS(stream), RpStreamResetReason_ConnectionTermination, ""/*TODO...*/);
    }
}

static void
do_connection_close(RpHttpConnectionManagerImpl* self, RpNetworkConnectionCloseType_e close_type/*TODO...response_flag*/, const char* details)
{
    NOISY_MSG_("(%p, %d, %p(%s))", self, close_type, details, details);
    //TODO...if (connection_idle_timer_)

    //TODO...if (connection_duration_timer)

    //TODO...if (drain_timer_)

NOISY_MSG_("streams %p", self->m_streams);
    if (self->m_streams)
    {
        //TODO...event
        //TODO...user_agent_.onConnectionDestroy();
        reset_all_streams(self, details);
    }

    RpNetworkConnection* connection = rp_network_filter_callbacks_connection(RP_NETWORK_FILTER_CALLBACKS(self->m_read_callbacks));
    if (close_type != RpNetworkConnectionCloseType_None)
    {
NOISY_MSG_("here");
        rp_network_connection_close(connection, close_type/*details*/);
    }
}

static void
check_for_deferred_close(RpHttpConnectionManagerImpl* self, bool skip_delay_close)
{
    NOISY_MSG_("(%p, %u)", self, skip_delay_close);
    RpNetworkConnectionCloseType_e close_type = RpNetworkConnectionCloseType_FlushWrite;//TODO... RpNetworkConnectionCloseType_FlushWriteAndDelay;
    //TODO...
NOISY_MSG_("drain state %d(%u), streams.empty() %u", self->m_drain_state, self->m_drain_state == RpDrainState_Closing, !self->m_streams);
    if (self->m_drain_state == RpDrainState_Closing &&
        !self->m_streams &&
        !rp_http_connection_wants_to_write(RP_HTTP_CONNECTION(self->m_codec)))
    {
        NOISY_MSG_("calling do_connection_close(%p, %d, \"%s\")", self, close_type, "");
        do_connection_close(self, close_type, "");
    }
}

static void
on_event_i(RpNetworkConnectionCallbacks* self, RpNetworkConnectionEvent_e event)
{
    NOISY_MSG_("(%p, %d)", self, event);

    if (event == RpNetworkConnectionEvent_LocalClose)
    {
        NOISY_MSG_("local close");
        //TODO...stats_.named_.downstream_cx...
    }

    if (event == RpNetworkConnectionEvent_RemoteClose ||
        event == RpNetworkConnectionEvent_LocalClose)
    {
NOISY_MSG_("%s", event == RpNetworkConnectionEvent_RemoteClose ? "RemoteClose" : "LocalClose");
        RpHttpConnectionManagerImpl* me = RP_HTTP_CONNECTION_MANAGER_IMPL(self);
        const char* details;
        if (event == RpNetworkConnectionEvent_RemoteClose)
        {
            me->m_remote_close = true;
            //TODO...stats_.
            details = "downstream_remote_disconnect";
        }
        else
        {
#if 0
            RpNetworkConnection* connection = rp_network_filter_callbacks_connection(RP_NETWORK_FILTER_CALLBACKS(me->m_read_callbacks));
            const char* local_close_reason = rp_network_connection_local_close_reason(connection);
#endif//0
            details = "downstream_local_disconnect";//TODO...+ local_close_reason...
        }

        NOISY_MSG_("calling do_connection_close(%p, %d, \"%s\")", me, RpNetworkConnectionCloseType_None, details);
        do_connection_close(me, RpNetworkConnectionCloseType_None, details);
    }
}

static void
network_connection_callbacks_iface_init(RpNetworkConnectionCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_event = on_event_i;
}

static RpNetworkFilterStatus_e
on_data_i(RpNetworkReadFilter* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuffer_get_length(data), end_stream);

    RpHttpConnectionManagerImpl* me = RP_HTTP_CONNECTION_MANAGER_IMPL(self);
    me->m_requests_during_dispatch_count = 0;
    if (!me->m_codec)
    {
        rp_http_connection_manager_impl_create_codec(me, data);
    }

    RpHttpConnection* codec = RP_HTTP_CONNECTION(me->m_codec);
    bool redispatch;
    do
    {
        redispatch = false;

        RpStatusCode_e status = rp_http_connection_dispatch(codec, data);

        if (status == RpStatusCode_CodecProtocolError)
        {
            LOGD("codec protocol error");
        }

        check_for_deferred_close(me, false);

        if (rp_http_connection_protocol(codec) <= EVHTP_PROTO_11)
        {
            RpNetworkFilterCallbacks* callbacks = RP_NETWORK_FILTER_CALLBACKS(me->m_read_callbacks);
            RpNetworkConnection* connection = rp_network_filter_callbacks_connection(callbacks);
            if (rp_network_connection_state(connection) == RpNetworkConnectionState_Open &&
                evbuffer_get_length(data) > 0 &&
                !me->m_streams)
            {
                redispatch = true;
            }
        }
    }
    while (redispatch);

    //TODO...if (!read_callbacks_->connection().streamInfo().protocol())

    return RpNetworkFilterStatus_StopIteration;
}

static RpNetworkFilterStatus_e
on_new_connection_i(RpNetworkReadFilter* self)
{
    NOISY_MSG_("(%p)", self);
    //TODO...if (!read_callbacks_->connection().streamInfo().protocol())
    return RpNetworkFilterStatus_Continue;
}

static void
initialize_read_filter_callbacks_i(RpNetworkReadFilter* self, RpNetworkReadFilterCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    RpHttpConnectionManagerImpl* me = RP_HTTP_CONNECTION_MANAGER_IMPL(self);
    RpNetworkConnection* connection = rp_network_filter_callbacks_connection(RP_NETWORK_FILTER_CALLBACKS(callbacks));
    me->m_read_callbacks = callbacks;
    me->m_dispatcher = rp_network_connection_dispatcher(connection);
    if (me->m_max_requests_during_dispatch != G_MAXUINT32)
    {
        //TODO...deferred_request_processing_callback_ = ...
    }

    //TODO...stats...

    rp_network_connection_add_connection_callbacks(connection, RP_NETWORK_CONNECTION_CALLBACKS(self));

    //TODO...if (config_->addProxyProtocolConnectionState())

    //TODO...if (config_->idleTimeout())

    //TODO...if (config_->maxConnectionDuration())

    //TODO...read_callbacks_->connection().setDelayedCloseTimeout()

    //TODO...read_callbacks_->connection().setConnectionStats()
}

static void
network_read_filter_iface_init(RpNetworkReadFilterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_data = on_data_i;
    iface->on_new_connection = on_new_connection_i;
    iface->initialize_read_filter_callbacks = initialize_read_filter_callbacks_i;
}

static void
on_go_away_i(RpHttpConnectionCallbacks* self, RpGoAwayErrorCode_e error_code)
{
    NOISY_MSG_("(%p, %d)", self, error_code);
    // Currently we do nothing with remote go away frames. In the future we can decide to no longer
    // push resources if applicable.
}

static void
http_connection_callbacks_iface_init(RpHttpConnectionCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_go_away = on_go_away_i;
}

static RpRequestDecoder*
new_stream_i(RpHttpServerConnectionCallbacks* self, RpResponseEncoder* response_encoder, bool is_internally_created)
{
    NOISY_MSG_("(%p, %p, %u)", self, response_encoder, is_internally_created);

    RpHttpConnectionManagerImpl* me = RP_HTTP_CONNECTION_MANAGER_IMPL(self);
    //TODO...if (connection_idle_timer)...

    RpStream* stream = rp_stream_encoder_get_stream(RP_STREAM_ENCODER(response_encoder));
    guint32 buffer_limit = rp_stream_buffer_limit(stream);
    RpHttpConnMgrImplActiveStream* new_stream = rp_http_conn_mgr_impl_active_stream_new(me, buffer_limit);
NOISY_MSG_("new stream %p", new_stream);

    ++me->m_accumulated_requests;
    guint64 max_requests_per_connection = rp_connection_manager_config_max_requests_per_connection(me->m_config);
    if (max_requests_per_connection > 0 && me->m_accumulated_requests >= max_requests_per_connection)
    {
        //TODO...
    }

    if (me->m_soft_drain_http1)
    {
        //TODO...new_stream->filter_manager_.streamInfo().setShouldDrainConnectionUponCompletion(true);
        me->m_drain_state = RpDrainState_Closing;
    }

    rp_http_conn_mgr_impl_active_stream_set_is_internally_created(new_stream, is_internally_created);
    rp_http_conn_mgr_impl_active_stream_set_response_encoder(new_stream, response_encoder);
    rp_stream_add_callbacks(stream, RP_STREAM_CALLBACKS(new_stream));
    rp_stream_register_codec_event_callbacks(stream, RP_CODEC_EVENT_CALLBACKS(new_stream));
    rp_stream_set_flush_timeout(stream, rp_http_conn_mgr_impl_active_stream_get_idle_timeout(new_stream));

NOISY_MSG_("streams %p", me->m_streams);
//    me->m_streams = g_slist_prepend(me->m_streams, new_stream);
me->m_streams = g_slist_append(me->m_streams, new_stream);
NOISY_MSG_("appended stream %p", new_stream);
NOISY_MSG_("streams %p", me->m_streams);

    return RP_REQUEST_DECODER(new_stream);
}

static void
http_server_connection_callbacks_iface_init(RpHttpServerConnectionCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    http_connection_callbacks_iface_init(&iface->parent_iface);
    iface->new_stream = new_stream_i;
}

#if 0
//TODO...
static void
on_deferred_request_processing(RpHttpConnectionManagerImpl* self)
{
    if (streams_.empty())
    {
        return;
    }
    requests_during_dispatch_count_ = 1;
    ...
}
#endif//0

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpHttpConnectionManagerImpl* self = RP_HTTP_CONNECTION_MANAGER_IMPL(obj);
    self->m_read_callbacks = NULL;
    g_clear_object(&self->m_codec);

    G_OBJECT_CLASS(rp_http_connection_manager_impl_parent_class)->dispose(obj);
}

static void
rp_http_connection_manager_impl_class_init(RpHttpConnectionManagerImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_http_connection_manager_impl_init(RpHttpConnectionManagerImpl* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_drain_state = RpDrainState_NotDraining;
    self->m_soft_drain_http1 = false;
    self->m_max_requests_during_dispatch = G_MAXUINT32;
    self->m_closed_non_internally_destroyed_requests = 0;
    self->m_number_premature_stream_requests = 0;
}

RpHttpConnectionManagerImpl*
rp_http_connection_manager_impl_new(RpConnectionManagerConfig* config, RpLocalInfo* local_info, RpClusterManager* cluster_manager)
{
    LOGD("(%p, %p, %p)", config, local_info, cluster_manager);
    g_return_val_if_fail(RP_IS_CONNECTION_MANAGER_CONFIG(config), NULL);
    g_return_val_if_fail(RP_IS_LOCAL_INFO(local_info), NULL);
    g_return_val_if_fail(RP_IS_CLUSTER_MANAGER(cluster_manager), NULL);
    RpHttpConnectionManagerImpl* self = g_object_new(RP_TYPE_HTTP_CONNECTION_MANAGER_IMPL, NULL);
    self->m_config = config;
    self->m_local_info = local_info;
    self->m_cluster_manager = cluster_manager;
    return self;
}

static void
maybe_drain_due_to_premature_resets(RpHttpConnectionManagerImpl* self)
{
    NOISY_MSG_("(%p)", self);

    if (self->m_closed_non_internally_destroyed_requests == 0)
    {
        NOISY_MSG_("returning");
        return;
    }

    //TODO...
guint64 limit = 500;

if (self->m_closed_non_internally_destroyed_requests < limit)
{
    NOISY_MSG_("number premature stream requests %zu", self->m_number_premature_stream_requests);
    if (self->m_number_premature_stream_requests * 2 < limit)
    {
        NOISY_MSG_("returning");
        return;
    }
}
else
{
    NOISY_MSG_("number premature stream requests %zu", self->m_number_premature_stream_requests);
    if (self->m_number_premature_stream_requests * 2 < self->m_closed_non_internally_destroyed_requests)
    {
        NOISY_MSG_("returning");
        return;
    }
}

    RpNetworkConnection* connection = rp_network_filter_callbacks_connection(RP_NETWORK_FILTER_CALLBACKS(self->m_read_callbacks));
    if (rp_network_connection_state(connection) == RpNetworkConnectionState_Open)
    {
        NOISY_MSG_("calling do_connection_close(%p, %d, \"%s\")", self, RpNetworkConnectionCloseType_Abort, "too_many_premature_resets");
        do_connection_close(self, RpNetworkConnectionCloseType_Abort, "too_many_premature_resets");
    }
}

static inline bool
is_premature_rst_stream(RpHttpConnMgrImplActiveStream* stream)
{
    NOISY_MSG_("(%p)", stream);
    //TODO...duration = ...

    //TODO...if (duration) ....

    return !rp_stream_info_response_code(
                rp_filter_manager_stream_info(RP_FILTER_MANAGER(
                    rp_http_conn_mgr_impl_active_stream_filter_manager_(stream))));
}

void
rp_http_connection_manager_impl_do_deferred_stream_destroy(RpHttpConnectionManagerImpl* self, RpHttpConnMgrImplActiveStream* stream)
{
    NOISY_MSG_("(%p, %p)", self, stream);

    if (!rp_http_conn_mgr_impl_active_stream_is_internally_destroyed(stream))
    {
        ++self->m_closed_non_internally_destroyed_requests;
        if (is_premature_rst_stream(stream))
        {
            NOISY_MSG_("premature rst stream");
            ++self->m_number_premature_stream_requests;
        }
    }

    //TODO...timers...

    if (!rp_http_conn_mgr_impl_active_stream_can_destroy_stream(stream))
    {
        NOISY_MSG_("zombie");
        rp_http_conn_mgr_impl_active_stream_set_is_zombie_stream(stream, true);
        return;
    }

    RpResponseEncoder* response_encoder = rp_http_conn_mgr_impl_active_stream_response_encoder(stream);
    if (response_encoder)
    {
        RpStream* stream_ = rp_stream_encoder_get_stream(RP_STREAM_ENCODER(response_encoder));
        rp_stream_register_codec_event_callbacks(stream_, NULL);
    }

    NOISY_MSG_("calling rp_http_conn_mgr_impl_active_stream_complete_request(%p)", stream);
    rp_http_conn_mgr_impl_active_stream_complete_request(stream);
    RpDownstreamFilterManager* filter_manager = rp_http_conn_mgr_impl_active_stream_filter_manager_(stream);
    NOISY_MSG_("calling rp_filter_manager_on_stream_complete(%p)", filter_manager);
    rp_filter_manager_on_stream_complete(RP_FILTER_MANAGER(filter_manager));

    //TODO...http/3

NOISY_MSG_("streams %p", self->m_streams);

    NOISY_MSG_("calling rp_filter_manager_destroy_filters(%p)", filter_manager);
    rp_filter_manager_destroy_filters(RP_FILTER_MANAGER(filter_manager));

NOISY_MSG_("streams %p", self->m_streams);
    self->m_streams = g_slist_remove(self->m_streams, stream);
NOISY_MSG_("streams %p", self->m_streams);
    RpNetworkConnection* connection = rp_network_filter_callbacks_connection(RP_NETWORK_FILTER_CALLBACKS(self->m_read_callbacks));
NOISY_MSG_("connection %p", connection);
    RpDispatcher* dispatcher = rp_network_connection_dispatcher(connection);
NOISY_MSG_("dispatcher %p", dispatcher);
    rp_dispatcher_deferred_delete(dispatcher, G_OBJECT(stream));

    if (response_encoder)
    {
        NOISY_MSG_("removing callbacks");
        rp_stream_remove_callbacks(
            rp_stream_encoder_get_stream(RP_STREAM_ENCODER(response_encoder)), RP_STREAM_CALLBACKS(stream));
    }

    //TODO...if (connection_idle_timer_)

    NOISY_MSG_("calling maybe_drain_due_to_premature_resets(%p)", self);
    maybe_drain_due_to_premature_resets(self);
}

static bool
request_was_connect(evhtp_headers_t* request_headers, evhtp_proto protocol)
{
    NOISY_MSG_("(%p, %d)", request_headers, protocol);
    if (!request_headers)
    {
        NOISY_MSG_("no headers");
        return false;
    }
    if (protocol <= EVHTP_PROTO_11)
    {
        NOISY_MSG_("returning %u", rp_header_utility_is_connect(request_headers));
        return rp_header_utility_is_connect(request_headers);
    }
    NOISY_MSG_("checking http/2 style upgrade");
    return rp_header_utility_is_connect(request_headers) ||
            rp_header_utility_is_upgrade(request_headers);
}

void
rp_http_connection_manager_impl_do_end_stream(RpHttpConnectionManagerImpl* self,
                                                RpHttpConnMgrImplActiveStream* stream,
                                                bool check_for_deferred_close_flag)
{
    LOGD("(%p, %p, %u)", self, stream, check_for_deferred_close_flag);

    g_return_if_fail(RP_IS_HTTP_CONNECTION_MANAGER_IMPL(self));
    g_return_if_fail(RP_IS_HTTP_CONN_MGR_IMPL_ACTIVE_STREAM(stream));

    RpDownstreamFilterManager* filter_manager = rp_http_conn_mgr_impl_active_stream_filter_manager_(stream);
    bool reset_stream = false;

NOISY_MSG_("response encoder %p, last byte recvd %u, saw local complete %u",
    rp_http_conn_mgr_impl_active_stream_response_encoder(stream),
    rp_downstream_filter_manager_has_last_downstream_byte_received(filter_manager),
    rp_http_conn_mgr_impl_active_stream_get_codec_saw_local_complete(stream));

    if (rp_http_conn_mgr_impl_active_stream_response_encoder(stream) &&
        (!rp_downstream_filter_manager_has_last_downstream_byte_received(filter_manager) ||
            !rp_http_conn_mgr_impl_active_stream_get_codec_saw_local_complete(stream)))
    {
        rp_filter_manager_set_local_complete(RP_FILTER_MANAGER(filter_manager));
        rp_http_conn_mgr_impl_active_stream_set_codec_saw_local_complete(stream, true);

        RpResponseEncoder* response_encoder = rp_http_conn_mgr_impl_active_stream_response_encoder(stream);
        RpStream* stream_ = rp_stream_encoder_get_stream(RP_STREAM_ENCODER(response_encoder));
        RpStreamInfo* stream_info = rp_filter_manager_stream_info(RP_FILTER_MANAGER(filter_manager));
        //TODO...if (requestWasConnect(....))
        if (request_was_connect(rp_http_conn_mgr_impl_active_stream_request_headers_(stream),
                                    rp_http_connection_manager_impl_get_protocol(self)) &&
            (rp_stream_info_has_response_flag(stream_info, RpCoreResponseFlag_UpstreamConnectionFailure) ||
                rp_stream_info_has_response_flag(stream_info, RpCoreResponseFlag_UpstreamConnectionTermination)))
        {
            NOISY_MSG_("resetting stream %p", stream_);
            rp_stream_reset_handler_reset_stream(RP_STREAM_RESET_HANDLER(stream_), RpStreamResetReason_ConnectError);
        }
        else
        {
            if (rp_stream_info_has_response_flag(stream_info, RpCoreResponseFlag_UpstreamProtocolError))
            {
                NOISY_MSG_("resetting stream %p", stream_);
                rp_stream_reset_handler_reset_stream(RP_STREAM_RESET_HANDLER(stream_), RpStreamResetReason_ProtocolError);
            }
            else
            {
                NOISY_MSG_("resetting stream %p", stream_);
                rp_stream_reset_handler_reset_stream(RP_STREAM_RESET_HANDLER(stream_), RpStreamResetReason_LocalReset);
            }
            reset_stream = true;
        }
    }

    //TODO...
    if (!reset_stream)
    {
        NOISY_MSG_("reset_stream is false");
        rp_http_connection_manager_impl_do_deferred_stream_destroy(self, stream);
    }

    if (reset_stream && rp_http_connection_protocol(RP_HTTP_CONNECTION(self->m_codec)) <= EVHTP_PROTO_11)
    {
        NOISY_MSG_("setting drain state to closing");
        self->m_drain_state = RpDrainState_Closing;
    }

    if (check_for_deferred_close_flag)
    {
NOISY_MSG_("checking for deferred close flag");
        evhtp_headers_t* response_headers = rp_http_conn_mgr_impl_active_stream_response_headers_(stream);
NOISY_MSG_("response headers %p", response_headers);
        bool http_10_sans_cl = (rp_http_connection_protocol(RP_HTTP_CONNECTION(self->m_codec)) == EVHTP_PROTO_10) &&
                                (!response_headers || !evhtp_header_find(response_headers, RpHeaderValues.ContentLength));
NOISY_MSG_("http 10 sans cl %u", http_10_sans_cl);
        RpStreamInfo* stream_info = rp_filter_manager_stream_info(RP_FILTER_MANAGER(filter_manager));
        bool connection_close = rp_stream_info_should_drain_connection_upon_completion(stream_info);
NOISY_MSG_("connection close %u", connection_close);
        bool request_complete = rp_downstream_filter_manager_has_last_downstream_byte_received(filter_manager);
NOISY_MSG_("request complete %u", request_complete);
        check_for_deferred_close(self, connection_close && (request_complete || http_10_sans_cl));
    }
}

evhtp_proto
rp_http_connection_manager_impl_get_protocol(RpHttpConnectionManagerImpl* self)
{
    g_return_val_if_fail(RP_IS_HTTP_CONNECTION_MANAGER_IMPL(self), EVHTP_PROTO_INVALID);
    return rp_http_connection_protocol(RP_HTTP_CONNECTION(self->m_codec));
}

RpDrainState_e
rp_http_connection_manager_impl_get_drain_state(RpHttpConnectionManagerImpl* self)
{
    g_return_val_if_fail(RP_IS_HTTP_CONNECTION_MANAGER_IMPL(self), RpDrainState_NotDraining);
    return self->m_drain_state;
}

void
rp_http_connection_manager_impl_set_drain_state(RpHttpConnectionManagerImpl* self, RpDrainState_e drain_state)
{
    NOISY_MSG_("(%p, %d)", self, drain_state);
    g_return_if_fail(RP_IS_HTTP_CONNECTION_MANAGER_IMPL(self));
    self->m_drain_state = drain_state;
}

bool
rp_http_connection_manager_impl_should_keep_alive_(RpHttpConnectionManagerImpl* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_CONNECTION_MANAGER_IMPL(self), false);
NOISY_MSG_("rp_http_connection_should_keep_alive(%p) %u",
    RP_HTTP_CONNECTION(self->m_codec), rp_http_connection_should_keep_alive(RP_HTTP_CONNECTION(self->m_codec)));
    return rp_http_connection_should_keep_alive(RP_HTTP_CONNECTION(self->m_codec));
}

RpConnectionManagerConfig*
rp_http_connection_manager_impl_connection_manager_config_(RpHttpConnectionManagerImpl* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_CONNECTION_MANAGER_IMPL(self), NULL);
    return self->m_config;
}

RpNetworkReadFilterCallbacks*
rp_http_connection_manager_impl_read_callbacks_(RpHttpConnectionManagerImpl* self)
{
    g_return_val_if_fail(RP_IS_HTTP_CONNECTION_MANAGER_IMPL(self), NULL);
    return self->m_read_callbacks;
}

RpNetworkConnection*
rp_http_connection_manager_impl_connection_(RpHttpConnectionManagerImpl* self)
{
    g_return_val_if_fail(RP_IS_HTTP_CONNECTION_MANAGER_IMPL(self), NULL);
    return rp_network_filter_callbacks_connection(RP_NETWORK_FILTER_CALLBACKS(self->m_read_callbacks));
}

RpDispatcher*
rp_http_connection_manager_impl_dispatcher_(RpHttpConnectionManagerImpl* self)
{
    g_return_val_if_fail(RP_IS_HTTP_CONNECTION_MANAGER_IMPL(self), NULL);
    return self->m_dispatcher;
}

bool
rp_http_connection_manager_impl_should_defer_request_proxying_to_next_io_cycle(RpHttpConnectionManagerImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_CONNECTION_MANAGER_IMPL(self), false);
    if (!self->m_deferred_request_processing_callback)
    {
        return false;
    }
return false;//TODO...
}

void
rp_http_connection_manager_impl_create_codec(RpHttpConnectionManagerImpl* self, evbuf_t* data)
{
    LOGD("(%p, %p(%zu))", self, data, data ? evbuffer_get_length(data) : 0);
    g_return_if_fail(RP_IS_HTTP_CONNECTION_MANAGER_IMPL(self));
    RpNetworkFilterCallbacks* read_callbacks = RP_NETWORK_FILTER_CALLBACKS(self->m_read_callbacks);
    RpNetworkConnection* connection = rp_network_filter_callbacks_connection(read_callbacks);
    RpHttpServerConnectionCallbacks* callbacks = RP_HTTP_SERVER_CONNECTION_CALLBACKS(self);
    self->m_codec = rp_connection_manager_config_create_codec(self->m_config,
                                                                connection,
                                                                data,
                                                                callbacks);
}

void
rp_http_connection_manager_impl_send_go_away_and_close(RpHttpConnectionManagerImpl* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_if_fail(RP_IS_HTTP_CONNECTION_MANAGER_IMPL(self));
    if (self->m_go_away_sent)
    {
        NOISY_MSG_("already sent");
        return;
    }
    rp_http_connection_go_away(RP_HTTP_CONNECTION(self->m_codec));
    self->m_go_away_sent = true;
    NOISY_MSG_("calling do_connection_close(%p, %d, \"%s\")", self, RpNetworkConnectionCloseType_FlushWriteAndDelay, "forced_goaway");
    do_connection_close(self, RpNetworkConnectionCloseType_FlushWriteAndDelay, "forced_goaway");
}

RpHttpServerConnection*
rp_http_connection_manager_impl_codec_(RpHttpConnectionManagerImpl* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_CONNECTION_MANAGER_IMPL(self), NULL);
    return self->m_codec;
}

RpClusterManager*
rp_http_connection_manager_impl_cluster_manager_(RpHttpConnectionManagerImpl* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_CONNECTION_MANAGER_IMPL(self), NULL);
    return self->m_cluster_manager;
}

RpLocalInfo*
rp_http_connection_manager_impl_local_info_(RpHttpConnectionManagerImpl* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_CONNECTION_MANAGER_IMPL(self), NULL);
    return self->m_local_info;
}
