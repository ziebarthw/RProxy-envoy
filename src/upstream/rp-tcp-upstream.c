/*
 * rp-tcp-upstream.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_tcp_upstream_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_tcp_upstream_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-headers.h"
#include "rp-router.h"
#include "../rp-tcp-conn-pool.h"
#include "upstream/rp-tcp-upstream.h"

#define NETWORK_CONNECTION(m) RP_NETWORK_CONNECTION(\
        rp_tcp_conn_pool_connection_data_connection((m)->m_upstream_conn_data))
#define RESPONSE_DECODER(m) RP_RESPONSE_DECODER((m)->m_upstream_request)
#define STREAM_DECODER(m) RP_STREAM_DECODER((m)->m_upstream_request)
#define STREAM_CALLBACKS(m) RP_STREAM_CALLBACKS((m)->m_upstream_request)
#define NETWORK_CONNECTION_CALLBACKS(m) RP_NETWORK_CONNECTION_CALLBACKS((m)->m_upstream_request)

typedef struct _RpTcpUpstreamPrivate RpTcpUpstreamPrivate;
struct _RpTcpUpstreamPrivate {
    RpUpstreamToDownstream* m_upstream_request;
    RpTcpConnPoolConnectionData* m_upstream_conn_data;

    evbuf_t* m_owned_data;

    bool m_downstream_complete : 1;
    bool m_force_reset_on_upstream_half_close : 1;
};

enum
{
    PROP_0, // Reserved.
    PROP_UPSTREAM_REQUEST,
    PROP_UPSTREAM,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void generic_upstream_iface_init(RpGenericUpstreamInterface* iface);
static void network_connection_callbacks_iface_init(RpNetworkConnectionCallbacksInterface* iface);
static void tcp_conn_pool_upstream_callbacks_iface_init(RpTcpConnPoolUpstreamCallbacksInterface* iface);

// https://github.com/envoyproxy/envoy/blob/main/source/extensions/upstreams/http/tcp/upstream_request.h
// class TcpUpstream : public Router::GenericUpstream, public Envoy::Http::StreamCallbacks

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpTcpUpstream, rp_tcp_upstream, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpTcpUpstream)
    G_IMPLEMENT_INTERFACE(RP_TYPE_GENERIC_UPSTREAM, generic_upstream_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_CONNECTION_CALLBACKS, network_connection_callbacks_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_TCP_CONN_POOL_UPSTREAM_CALLBACKS, tcp_conn_pool_upstream_callbacks_iface_init)
)

#define PRIV(obj) \
    ((RpTcpUpstreamPrivate*) rp_tcp_upstream_get_instance_private(RP_TCP_UPSTREAM(obj)))

static inline evbuf_t*
ensure_owned_data(RpTcpUpstreamPrivate* me)
{
    NOISY_MSG_("(%p)", me);
    if (me->m_owned_data)
    {
        NOISY_MSG_("pre-allocated owned data %p", me->m_owned_data);
        return me->m_owned_data;
    }
    me->m_owned_data = evbuffer_new();
    NOISY_MSG_("allocated owned data %p", me->m_owned_data);
    return me->m_owned_data;
}

static RpStatusCode_e
encode_headers_i(RpGenericUpstream* self, evhtp_headers_t* request_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, request_headers, end_stream);
    RpTcpUpstreamPrivate* me = PRIV(self);
    me->m_downstream_complete = end_stream;
    RpRouteEntry* route_entry = rp_route_route_entry(
                                    rp_upstream_to_downstream_route(me->m_upstream_request));
    g_assert(route_entry != NULL);
    //TODO...if (route_entry->connectConfig().has_value())...

    // TcpUpstream::encodeHeaders is called after the UpstreamRequest is fully
    // initialized. Also use this time to synthesize the 200 response headers
    // downstream to complete the CONNECT handshake.
    evhtp_headers_t* response_headers = evhtp_headers_new();
    evhtp_headers_add_header(response_headers,
        evhtp_header_new(RpHeaderValues.Status, "200", 0, 0));
    rp_response_decoder_decode_headers(RESPONSE_DECODER(me), response_headers, /*end_stream=*/false);
    return RpStatusCode_Ok;
}

static void
encode_data_i(RpGenericUpstream* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    RpTcpUpstreamPrivate* me = PRIV(self);
    me->m_downstream_complete = end_stream;
    //TODO...bytes_meter_->addWireBytesSent(data.length());
    rp_network_connection_write(NETWORK_CONNECTION(me), data, end_stream);
}

static void
encode_trailers_i(RpGenericUpstream* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    RpTcpUpstreamPrivate* me = PRIV(self);
    me->m_downstream_complete = true;
    evbuf_t* owned_data = ensure_owned_data(me);
    rp_network_connection_write(NETWORK_CONNECTION(me), owned_data, true);
}

static void
read_disable_i(RpGenericUpstream* self, bool disable)
{
    NOISY_MSG_("(%p, %u)", self, disable);
    RpTcpUpstreamPrivate* me = PRIV(self);
    if (rp_network_connection_state(NETWORK_CONNECTION(me)) != RpNetworkConnectionState_Open)
    {
        NOISY_MSG_("not open");
        return;
    }
    rp_network_connection_read_disable(NETWORK_CONNECTION(me), disable);
}

static void
reset_stream_i(RpGenericUpstream* self)
{
    NOISY_MSG_("(%p)", self);
    RpTcpUpstreamPrivate* me = PRIV(self);
    g_clear_object(&me->m_upstream_request);
    rp_network_connection_close(NETWORK_CONNECTION(me), RpNetworkConnectionCloseType_NoFlush);
}

static void
generic_upstream_iface_init(RpGenericUpstreamInterface* iface)
{
    LOGD("(%p)", iface);
    iface->encode_headers = encode_headers_i;
    iface->encode_data = encode_data_i;
    iface->encode_trailers = encode_trailers_i;
    iface->read_disable = read_disable_i;
    iface->reset_stream = reset_stream_i;
}

static void
on_event_i(RpNetworkConnectionCallbacks* self, RpNetworkConnectionEvent_e event)
{
    NOISY_MSG_("(%p, %d)", self, event);
    RpTcpUpstreamPrivate* me = PRIV(self);
    if ((event == RpNetworkConnectionEvent_LocalClose ||
        event == RpNetworkConnectionEvent_RemoteClose) &&
        me->m_upstream_request)
    {
        NOISY_MSG_("closed");
        rp_stream_callbacks_on_reset_stream(STREAM_CALLBACKS(me), RpStreamResetReason_ConnectionTermination, "");
    }
}

static void
on_above_write_buffer_high_water_mark_i(RpNetworkConnectionCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpTcpUpstreamPrivate* me = PRIV(self);
    if (me->m_upstream_request)
    {
        rp_network_connection_callbacks_on_above_write_buffer_high_water_mark(NETWORK_CONNECTION_CALLBACKS(me));
    }
}

static void
on_below_write_buffer_low_watermark_i(RpNetworkConnectionCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpTcpUpstreamPrivate* me = PRIV(self);
    if (me->m_upstream_request)
    {
        rp_network_connection_callbacks_on_below_write_buffer_low_watermark(NETWORK_CONNECTION_CALLBACKS(me));
    }
}

static void
network_connection_callbacks_iface_init(RpNetworkConnectionCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_event = on_event_i;
    iface->on_above_write_buffer_high_water_mark = on_above_write_buffer_high_water_mark_i;
    iface->on_below_write_buffer_low_watermark = on_below_write_buffer_low_watermark_i;
}

static void
on_upstream_data_i(RpTcpConnPoolUpstreamCallbacks* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    RpTcpUpstreamPrivate* me = PRIV(self);
    bool force_reset =
        me->m_force_reset_on_upstream_half_close && end_stream && !me->m_downstream_complete;
    //TODO...bytes_meter_->addWireBytesReceived(...);
    rp_stream_decoder_decode_data(STREAM_DECODER(me), data, end_stream);
    if (force_reset && me->m_upstream_request)
    {
        NOISY_MSG_("resetting stream");
        rp_stream_callbacks_on_reset_stream(STREAM_CALLBACKS(me),
                                            RpStreamResetReason_ConnectionTermination,
                                            "half_close_initiated_full_close");
    }
}

static void
tcp_conn_pool_upstream_callbacks_iface_init(RpTcpConnPoolUpstreamCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_upstream_data = on_upstream_data_i;
}

OVERRIDE void
set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_UPSTREAM_REQUEST:
            PRIV(obj)->m_upstream_request = g_value_get_object(value);
NOISY_MSG_("upstream request %p", PRIV(obj)->m_upstream_request);
            break;
        case PROP_UPSTREAM:
            PRIV(obj)->m_upstream_conn_data = g_value_get_object(value);
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

    G_OBJECT_CLASS(rp_tcp_upstream_parent_class)->constructed(obj);

    RpTcpUpstreamPrivate* me = PRIV(obj);
    rp_network_connection_enable_half_close(NETWORK_CONNECTION(me), true);
    rp_tcp_conn_pool_connection_data_add_upstream_callbacks(me->m_upstream_conn_data, RP_TCP_CONN_POOL_UPSTREAM_CALLBACKS(obj));
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpTcpUpstreamPrivate* me = PRIV(obj);
    g_clear_object(&me->m_upstream_conn_data);
    g_clear_pointer(&me->m_owned_data, evbuffer_free);

    G_OBJECT_CLASS(rp_tcp_upstream_parent_class)->dispose(obj);
}

static void
rp_tcp_upstream_class_init(RpTcpUpstreamClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    obj_properties[PROP_UPSTREAM_REQUEST] = g_param_spec_object("upstream-request",
                                                    "Upstream Request",
                                                    "UpstremRequest Instance",
                                                    RP_TYPE_UPSTREAM_TO_DOWNSTREAM,
                                                    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_UPSTREAM] = g_param_spec_object("upstream",
                                                    "Upstream",
                                                    "RpTcpConnPoolConnectionData Instance",
                                                    RP_TYPE_TCP_CONN_POOL_CONNECTION_DATA,
                                                    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_tcp_upstream_init(RpTcpUpstream* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}
