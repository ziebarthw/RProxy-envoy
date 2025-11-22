/*
 * rp-codec-client.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_codec_client_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_codec_client_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-host-description.h"
#include "rp-codec-client-active-request.h"
#include "rp-codec-read-filter.h"
#include "rp-dispatcher.h"
#include "rp-net-filter.h"
#include "rp-codec-client.h"

G_DEFINE_INTERFACE(RpCodecClientCallbacks, rp_codec_client_callbacks, G_TYPE_OBJECT)
static void
rp_codec_client_callbacks_default_init(RpCodecClientCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
}

typedef struct _RpCodecClientPrivate RpCodecClientPrivate;
struct _RpCodecClientPrivate {

    RpNetworkClientConnection* m_connection;
    RpHttpClientConnection* m_codec;
    GSList* m_active_requests;
    RpHttpConnectionCallbacks* m_codec_callbacks;
    RpCodecClientCallbacks* m_codec_client_callbacks;

    RpHostDescription* m_host;
    evhtp_t* m_dispatcher;

    evbuf_t* m_empty_buf;
    //idle_timer
    //idel_timeout

    RpCodecType_e m_type;

    bool m_connected : 1;
    bool m_remote_closed : 1;
    bool m_protocol_error : 1;
    bool m_connect_failed : 1;
    bool m_connect_called : 1;
};

enum
{
    PROP_0, // Reserved.
    PROP_TYPE,
    PROP_CONNECTION,
    PROP_HOST,
    PROP_DISPATCHER,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void network_connection_callbacks_iface_init(RpNetworkConnectionCallbacksInterface* iface);
static void http_connection_callbacks_iface_init(RpHttpConnectionCallbacksInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpCodecClient, rp_codec_client, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpCodecClient)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_CONNECTION_CALLBACKS, network_connection_callbacks_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_HTTP_CONNECTION_CALLBACKS, http_connection_callbacks_iface_init)
)

#define PRIV(obj) \
    ((RpCodecClientPrivate*) rp_codec_client_get_instance_private(RP_CODEC_CLIENT(obj)))

static inline evbuf_t*
ensure_empty_buf(RpCodecClientPrivate* me)
{
    NOISY_MSG_("(%p)", me);
    if (me->m_empty_buf)
    {
        NOISY_MSG_("pre-allocated empty buf %p", me->m_empty_buf);
        return me->m_empty_buf;
    }
    me->m_empty_buf = evbuffer_new();
    NOISY_MSG_("allocated empty buf %p", me->m_empty_buf);
    return me->m_empty_buf;
}

static void
on_event_i(RpNetworkConnectionCallbacks* self, RpNetworkConnectionEvent_e event)
{
    NOISY_MSG_("(%p, %d)", self, event);

    RpCodecClientPrivate* me = PRIV(self);

    if (event == RpNetworkConnectionEvent_Connected)
    {
        NOISY_MSG_("connected");
        me->m_connected = true;
        return;
    }

    if (event == RpNetworkConnectionEvent_RemoteClose)
    {
        NOISY_MSG_("remote closed");
        me->m_remote_closed = true;
    }

    if (me->m_type == RpCodecType_HTTP1 &&
        event == RpNetworkConnectionEvent_RemoteClose &&
        me->m_active_requests)
    {
        evbuf_t* empty = ensure_empty_buf(me);
        rp_codec_client_on_data(RP_CODEC_CLIENT(self), empty);
    }

    if (event == RpNetworkConnectionEvent_RemoteClose ||
        event == RpNetworkConnectionEvent_LocalClose)
    {
        NOISY_MSG_("disconnect; %s close", event == RpNetworkConnectionEvent_RemoteClose ? "remote" : "local");
        //TODO...disableIdleTimer();
        //TODO...idle_timer_.reset();
        RpStreamResetReason_e reason = event == RpNetworkConnectionEvent_RemoteClose ?
                                        RpStreamResetReason_RemoteConnectionFailure :
                                        RpStreamResetReason_LocalConnectionFailure;
        if (me->m_connected)
        {
            reason = RpStreamResetReason_ConnectionTermination;
            if (me->m_protocol_error)
            {
                reason = RpStreamResetReason_ProtocolError;
                //TODO...connection_->streamInfo().setResponseFlag(...);
            }
        }
        while (me->m_active_requests)
        {
            RpStream* active_request = me->m_active_requests->data;
            RpStream* stream = rp_stream_encoder_get_stream(RP_STREAM_ENCODER(active_request));
            rp_stream_reset_handler_reset_stream(RP_STREAM_RESET_HANDLER(stream), reason);
        }
    }
}

static void
network_connection_callbacks_iface_init(RpNetworkConnectionCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_event = on_event_i;
}

static void
on_go_away_i(RpHttpConnectionCallbacks* self, RpGoAwayErrorCode_e error_code)
{
    NOISY_MSG_("(%p, %d)", self, error_code);
    RpHttpConnectionCallbacks* callbacks = PRIV(self)->m_codec_callbacks;
    if (callbacks) rp_http_connection_callbacks_on_go_away(self, error_code);
}

static void
on_max_streams_changed_i(RpHttpConnectionCallbacks* self, guint32 num_streams)
{
    NOISY_MSG_("(%p, %u)", self, num_streams);
    RpHttpConnectionCallbacks* callbacks = PRIV(self)->m_codec_callbacks;
    if (callbacks) rp_http_connection_callbacks_on_max_streams_changed(self, num_streams);
}

static void
on_settings_i(RpHttpConnectionCallbacks* self, RpReceivedSettings* settings)
{
    NOISY_MSG_("(%p, %p)", self, settings);
    RpHttpConnectionCallbacks* callbacks = PRIV(self)->m_codec_callbacks;
    if (callbacks) rp_http_connection_callbacks_on_settings(self, settings);
}

static void
http_connection_callbacks_iface_init(RpHttpConnectionCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_go_away = on_go_away_i;
    iface->on_max_streams_changed = on_max_streams_changed_i;
    iface->on_settings = on_settings_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_TYPE:
            g_value_set_enum(value, PRIV(obj)->m_type);
            break;
        case PROP_CONNECTION:
            g_value_set_object(value, PRIV(obj)->m_connection);
            break;
        case PROP_HOST:
            g_value_set_object(value, PRIV(obj)->m_host);
            break;
        case PROP_DISPATCHER:
            g_value_set_pointer(value, PRIV(obj)->m_dispatcher);
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
        case PROP_TYPE:
            PRIV(obj)->m_type = g_value_get_enum(value);
            break;
        case PROP_CONNECTION:
            PRIV(obj)->m_connection = g_value_get_object(value);
            break;
        case PROP_HOST:
            PRIV(obj)->m_host = g_value_get_object(value);
            break;
        case PROP_DISPATCHER:
            PRIV(obj)->m_dispatcher = g_value_get_pointer(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_codec_client_parent_class)->constructed(obj);

    RpCodecClientPrivate* me = PRIV(obj);
    RpNetworkClientConnection* connection = me->m_connection;
    NOISY_MSG_("connection %p", connection);

    rp_network_connection_add_connection_callbacks(RP_NETWORK_CONNECTION(connection),
                                                    RP_NETWORK_CONNECTION_CALLBACKS(obj));
    rp_network_filter_manager_add_read_filter(RP_NETWORK_FILTER_MANAGER(connection),
                                                RP_NETWORK_READ_FILTER(rp_codec_read_filter_new(obj)));
    //TODO...if (idle_timeout_)

    rp_network_connection_no_delay(RP_NETWORK_CONNECTION(connection), true);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_codec_client_parent_class)->dispose(obj);
}

static void
rp_codec_client_class_init(RpCodecClientClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    obj_properties[PROP_TYPE] = g_param_spec_enum("type",
                                                    "Type",
                                                    "Codec Type",
                                                    RP_TYPE_CODEC_TYPE,
                                                    RpCodecType_HTTP1,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_CONNECTION] = g_param_spec_object("connection",
                                                    "Connection",
                                                    "Network ClientConnection Instance",
                                                    RP_TYPE_NETWORK_CLIENT_CONNECTION,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_HOST] = g_param_spec_object("host",
                                                    "Host",
                                                    "HostDescription Instance",
                                                    RP_TYPE_HOST_DESCRIPTION,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_DISPATCHER] = g_param_spec_pointer("dispatcher",
                                                    "Dispatcher",
                                                    "Dispatcher Instance(evhtp_t)",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_codec_client_init(RpCodecClient* self)
{
    NOISY_MSG_("(%p)", self);

    RpCodecClientPrivate* me = PRIV(self);
    me->m_active_requests = NULL;
    me->m_remote_closed = false;
}

void
rp_codec_client_set_codec_connection_callbacks(RpCodecClient* self, RpHttpConnectionCallbacks* callbacks)
{
    LOGD("(%p, %p)", self, callbacks);
    g_return_if_fail(RP_IS_CODEC_CLIENT(self));
    PRIV(self)->m_codec_callbacks = callbacks;
}

void
rp_codec_client_go_away(RpCodecClient* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_CODEC_CLIENT(self));
    rp_http_connection_go_away(RP_HTTP_CONNECTION(PRIV(self)->m_codec));
}

evhtp_proto
rp_codec_client_protocol(RpCodecClient* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CODEC_CLIENT(self), EVHTP_PROTO_INVALID);
    return rp_http_connection_protocol(RP_HTTP_CONNECTION(PRIV(self)->m_codec));
}

RpRequestEncoder*
rp_codec_client_new_stream(RpCodecClient* self, RpResponseDecoder* response_decoder)
{
    LOGD("(%p, %p)", self, response_decoder);

    g_return_val_if_fail(RP_IS_CODEC_CLIENT(self), NULL);
    g_return_val_if_fail(RP_IS_RESPONSE_DECODER(response_decoder), NULL);

    RpCodecClientPrivate* me = PRIV(self);
    RpCodecClientActiveRequest* request = rp_codec_client_active_request_new(self, response_decoder);
    RpRequestEncoder* encoder = rp_http_client_connection_new_stream(me->m_codec, RP_RESPONSE_DECODER(request));
    rp_codec_client_active_request_set_encoder(request, encoder);
    me->m_active_requests = g_slist_prepend(me->m_active_requests, request);

    RpUpstreamInfo* upstream_info = rp_stream_info_upstream_info(
                                        rp_network_connection_stream_info(RP_NETWORK_CONNECTION(me->m_connection)));
    rp_upstream_info_set_upstream_num_streams(upstream_info,
        rp_upstream_info_upstream_num_streams(upstream_info) + 1);

    //TODO...disableIdleTimer()
    return me->m_active_requests->data;
}

static void
delete_request(RpCodecClient* self, RpCodecClientActiveRequest* request)
{
    NOISY_MSG_("(%p, %p)", self, request);
    RpCodecClientPrivate* me = PRIV(self);
    me->m_active_requests = g_slist_remove(me->m_active_requests, request);
    RpNetworkConnection* connection = RP_NETWORK_CONNECTION(me->m_connection);
    RpDispatcher* dispatcher = rp_network_connection_dispatcher(connection);
    rp_dispatcher_deferred_delete(dispatcher, G_OBJECT(request));
    RpCodecClientCallbacks* codec_client_callbacks = me->m_codec_client_callbacks;
    if (codec_client_callbacks)
    {
        rp_codec_client_callbacks_on_stream_destroy(codec_client_callbacks);
    }
    if (rp_codec_client_num_active_requests(self) == 0)
    {
        //TODO...enableIdleTimer()????
    }
}

static void
complete_request(RpCodecClient* self, RpCodecClientActiveRequest* request)
{
    NOISY_MSG_("(%p, %p)", self, request);
    delete_request(self, request);
    rp_codec_client_active_request_remove_encoder_callbacks(request);
}

void
rp_codec_client_response_pre_decode_complete(RpCodecClient* self, void* arg)
{
    LOGD("(%p, %p)", self, arg);
    g_return_if_fail(RP_IS_CODEC_CLIENT(self));
    g_return_if_fail(RP_IS_CODEC_CLIENT_ACTIVE_REQUEST(arg));
    RpCodecClientPrivate* me = PRIV(self);
    RpCodecClientCallbacks* codec_client_callbacks = me->m_codec_client_callbacks;
    if (codec_client_callbacks)
    {
        rp_codec_client_callbacks_on_stream_pre_decode_complete(codec_client_callbacks);
    }

    RpCodecClientActiveRequest* request = RP_CODEC_CLIENT_ACTIVE_REQUEST(arg);
    rp_codec_client_active_request_set_decode_complete(request);
    if (rp_codec_client_active_request_get_encode_complete(request) ||
        !rp_codec_client_active_request_get_wait_encode_complete(request))
    {
        complete_request(self, request);
    }
    else
    {
        LOGD("waiting for encode to complete");
    }
}

void
rp_codec_client_request_encode_complete(RpCodecClient* self, void* arg)
{
    LOGD("(%p, %p)", self, arg);
    g_return_if_fail(RP_IS_CODEC_CLIENT(self));
    g_return_if_fail(RP_IS_CODEC_CLIENT_ACTIVE_REQUEST(arg));
    RpCodecClientActiveRequest* request = RP_CODEC_CLIENT_ACTIVE_REQUEST(arg);
    rp_codec_client_active_request_set_encode_complete(request);
    if (rp_codec_client_active_request_get_decode_complete(request))
    {
        complete_request(self, request);
    }
}

gsize
rp_codec_client_num_active_requests(RpCodecClient* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CODEC_CLIENT(self), 0);
    return g_slist_length(PRIV(self)->m_active_requests);
}

void
rp_codec_client_on_data(RpCodecClient* self, evbuf_t* data)
{
    LOGD("(%p, %p(%zu))", self, data, evbuffer_get_length(data));

    RpCodecClientPrivate* me = PRIV(self);
    RpStatusCode_e status = rp_http_connection_dispatch(RP_HTTP_CONNECTION(me->m_codec), data);

    if (status != RpStatusCode_Ok)
    {
        //TODO...
    }
}

void
rp_codec_client_add_connection_callbacks(RpCodecClient* self, RpNetworkConnectionCallbacks* callbacks)
{
    LOGD("(%p, %p)", self, callbacks);
    g_return_if_fail(RP_IS_CODEC_CLIENT(self));
    g_return_if_fail(RP_IS_NETWORK_CONNECTION_CALLBACKS(callbacks));
    RpNetworkConnection* connection = RP_NETWORK_CONNECTION(PRIV(self)->m_connection);
    rp_network_connection_add_connection_callbacks(connection, callbacks);
}

bool
rp_codec_client_is_half_close_enabled(RpCodecClient* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CODEC_CLIENT(self), false);
    RpNetworkConnection* connection = RP_NETWORK_CONNECTION(PRIV(self)->m_connection);
    return rp_network_connection_is_half_close_enabled(connection);
}

void
rp_codec_client_initialize_read_filters(RpCodecClient* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_CODEC_CLIENT(self));
    rp_network_filter_manager_initialize_read_filters(RP_NETWORK_FILTER_MANAGER(PRIV(self)->m_connection));
}

void
rp_codec_client_close(RpCodecClient* self, RpNetworkConnectionCloseType_e type)
{
    LOGD("(%p, %d)", self, type);
    g_return_if_fail(RP_IS_CODEC_CLIENT(self));
    RpNetworkConnection* connection = RP_NETWORK_CONNECTION(PRIV(self)->m_connection);
    rp_network_connection_close(connection, type);
}

void
rp_codec_client_close_(RpCodecClient* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_CODEC_CLIENT(self));
    rp_codec_client_close(self, RpNetworkConnectionCloseType_NoFlush);
}

guint64
rp_codec_client_id(RpCodecClient* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CODEC_CLIENT(self), 0);
    RpNetworkConnection* connection = RP_NETWORK_CONNECTION(PRIV(self)->m_connection);
    return rp_network_connection_id(connection);
}

const char*
rp_codec_client_connection_failure_reason(RpCodecClient* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CODEC_CLIENT(self), "");
    RpNetworkConnection* connection = RP_NETWORK_CONNECTION(PRIV(self)->m_connection);
    return rp_network_connection_transport_failure_reason(connection);
}

bool
rp_codec_client_remote_closed(RpCodecClient* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CODEC_CLIENT(self), true);
    return PRIV(self)->m_remote_closed;
}

#define NETWORK_CONNECTION(s) \
    RP_NETWORK_CONNECTION((s)->m_connection)

void
rp_codec_client_connect(RpCodecClient* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_CODEC_CLIENT(self));
    RpCodecClientPrivate* me = PRIV(self);
    me->m_connect_called = true;
    if (!rp_network_connection_connecting(NETWORK_CONNECTION(me)))
    {
        NOISY_MSG_("connection_->state() == Network::Connection::State::Open %u",
            rp_network_connection_state(NETWORK_CONNECTION(me)) == RpNetworkConnectionState_Open);
        me->m_connected = true;
    }
    else
    {
        NOISY_MSG_("connecting");
        rp_network_client_connection_connect(me->m_connection);
    }
}

RpStreamInfo*
rp_codec_client_stream_info(RpCodecClient* self)
{
    LOGD("(%p)", self);
    return rp_network_connection_stream_info(RP_NETWORK_CONNECTION(PRIV(self)->m_connection));
}

void
rp_codec_client_set_requested_server_name(RpCodecClient* self, const char* requested_server_name)
{
    LOGD("(%p, %p(%s))", self, requested_server_name, requested_server_name);
    g_return_if_fail(RP_IS_CODEC_CLIENT(self));
    RpConnectionInfoSetter* info = rp_network_connection_connection_info_setter(NETWORK_CONNECTION(PRIV(self)));
    rp_connection_info_setter_set_requested_server_name(info, requested_server_name);
}

RpCodecType_e
rp_codec_client_type_(RpCodecClient* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CODEC_CLIENT(self), RpCodecType_HTTP1);
    return PRIV(self)->m_type;
}

void
rp_codec_client_set_codec_(RpCodecClient* self, RpHttpClientConnection* codec)
{
    LOGD("(%p, %p(%s))", self, codec, G_OBJECT_TYPE_NAME(codec));
    g_return_if_fail(RP_IS_CODEC_CLIENT(self));
    PRIV(self)->m_codec = codec;
}
