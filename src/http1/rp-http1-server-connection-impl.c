/*
 * rp-http1-server-connection-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_server_connection_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_server_connection_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-net-conn-impl.h"
#include "rp-dispatcher.h"
#include "rp-headers.h"
#include "rp-parser.h"
#include "rp-response-encoder-impl.h"
#include "rp-http1-server-connection-impl.h"

static inline void
g_string_free_and_clear(GString** strp)
{
    if (strp && *strp)
    {
        g_string_free(*strp, TRUE);
        *strp = NULL;
    }
}

typedef struct ActiveRequest_s ActiveRequest;
struct ActiveRequest_s {
    GString* m_request_url;
    RpRequestDecoder* m_request_decoder;
    RpResponseEncoderImpl* m_response_encoder;
    evhtp_headers_t* m_headers_or_trailers;
    bool m_remote_complete;
};

static inline ActiveRequest*
ActiveRequest_new(RpHttp1ServerConnectionImpl* connection, const struct RpHttp1Settings_s* settings)
{
    NOISY_MSG_("(%p, %p)", connection, settings);
    ActiveRequest* self = g_new0(struct ActiveRequest_s, 1);
    self->m_response_encoder = rp_response_encoder_impl_new(RP_HTTP1_CONNECTION_IMPL(connection),
                                settings->m_stream_error_on_invalid_http_message);
    self->m_request_url = g_string_new(NULL);
NOISY_MSG_("request url %p", self->m_request_url);
    return self;
}

static inline void
ActiveRequest_free(ActiveRequest* self)
{
    NOISY_MSG_("(%p)", self);
NOISY_MSG_("freeing request url %p", self->m_request_url);
    g_string_free_and_clear(&self->m_request_url);
NOISY_MSG_("clearing response encoder %p", self->m_response_encoder);
    g_clear_object(&self->m_response_encoder);
}

struct _RpHttp1ServerConnectionImpl {
    RpHttp1ConnectionImpl parent_instance;

    RpHttpServerConnectionCallbacks* m_callbacks;
    RpNetworkConnection* m_connection;
    guint32 m_outbound_responses;
    //evbuf_t* m_owned_output_buffer;
    evhtp_headers_t* m_headers_or_trailers;
    ActiveRequest* m_active_request;
    struct RpHttp1Settings_s m_settings;
};

static void http_connection_iface_init(RpHttpConnectionInterface* iface);
static void http_server_connection_iface_init(RpHttpServerConnectionInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpHttp1ServerConnectionImpl, rp_http1_server_connection_impl, RP_TYPE_HTTP1_CONNECTION_IMPL,
    G_IMPLEMENT_INTERFACE(RP_TYPE_HTTP_CONNECTION, http_connection_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_HTTP_SERVER_CONNECTION, http_server_connection_iface_init)
)

static inline void
disable_read(RpResponseEncoderImpl* self)
{
    NOISY_MSG_("(%p)", self);
    rp_stream_read_disable(RP_STREAM(self), true);
}

static inline RpHttpConnectionInterface*
parent_iface(RpHttpConnection* self)
{
    return (RpHttpConnectionInterface*)g_type_interface_peek_parent(RP_HTTP_CONNECTION_GET_IFACE(self));
}

static RpStatusCode_e
dispatch_i(RpHttpConnection* self, evbuf_t* data)
{
    NOISY_MSG_("(%p, %p(%zu))", self, data, data ? evbuffer_get_length(data) : 0);

    RpHttp1ServerConnectionImpl* me = RP_HTTP1_SERVER_CONNECTION_IMPL(self);

    ActiveRequest* active_request = me->m_active_request;
    if (active_request && active_request->m_remote_complete)
    {
        disable_read(active_request->m_response_encoder);
        return RpStatusCode_Ok;
    }

    RpStatusCode_e status = parent_iface(self)->dispatch(self, data);

    active_request = me->m_active_request;
    if (active_request && active_request->m_remote_complete)
    {
        if (data && evbuffer_get_length(data) > 0)
        {
            disable_read(active_request->m_response_encoder);
        }
    }

    return status;
}

static void
go_away_i(RpHttpConnection* self)
{
    NOISY_MSG_("(%p)", self);
    parent_iface(self)->go_away(self);
}

static void
on_underlying_connection_above_write_buffer_high_watermark_i(RpHttpConnection* self)
{
    NOISY_MSG_("(%p)", self);
    parent_iface(self)->on_underlying_connection_above_write_buffer_high_watermark(self);
}

static void
on_underlying_connection_below_write_buffer_lo_watermark_i(RpHttpConnection* self)
{
    NOISY_MSG_("(%p)", self);
    parent_iface(self)->on_underlying_connection_below_write_buffer_lo_watermark(self);
}

static enum evhtp_proto
protocol_i(RpHttpConnection* self)
{
    NOISY_MSG_("(%p)", self);
    return parent_iface(self)->protocol(self);
}

static bool
should_keep_alive_i(RpHttpConnection* self)
{
    NOISY_MSG_("(%p)", self);
    return parent_iface(self)->should_keep_alive(self);
}

static void
shutdown_notice_i(RpHttpConnection* self)
{
    NOISY_MSG_("(%p)", self);
    parent_iface(self)->shutdown_notice(self);
}

static bool
wants_to_write_i(RpHttpConnection* self)
{
    NOISY_MSG_("(%p)", self);
    return parent_iface(self)->wants_to_write(self);
}

static void
http_connection_iface_init(RpHttpConnectionInterface* iface)
{
    LOGD("(%p)", iface);
    iface->dispatch = dispatch_i;
    iface->go_away = go_away_i;
    iface->on_underlying_connection_above_write_buffer_high_watermark = on_underlying_connection_above_write_buffer_high_watermark_i;
    iface->on_underlying_connection_below_write_buffer_lo_watermark = on_underlying_connection_below_write_buffer_lo_watermark_i;
    iface->protocol = protocol_i;
    iface->should_keep_alive = should_keep_alive_i;
    iface->shutdown_notice = shutdown_notice_i;
    iface->wants_to_write = wants_to_write_i;
}

static void
http_server_connection_iface_init(RpHttpServerConnectionInterface* iface)
{
    LOGD("(%p)", iface);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpHttp1ServerConnectionImpl* self = RP_HTTP1_SERVER_CONNECTION_IMPL(obj);
    g_clear_pointer(&self->m_active_request, ActiveRequest_free);
    self->m_callbacks = NULL;
    self->m_connection = NULL;

    G_OBJECT_CLASS(rp_http1_server_connection_impl_parent_class)->dispose(obj);
}

OVERRIDE bool
supports_http_10(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HTTP1_SERVER_CONNECTION_IMPL(self)->m_settings.m_accept_http_10;
}

OVERRIDE RpStatusCode_e
on_message_begin_base(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    if (!rp_http1_connection_impl_reset_stream_called(self))
    {
        RpHttp1ServerConnectionImpl* me = RP_HTTP1_SERVER_CONNECTION_IMPL(self);
        ActiveRequest* active_request = ActiveRequest_new(me, &me->m_settings);
        RpResponseEncoder* response_encoder = RP_RESPONSE_ENCODER(active_request->m_response_encoder);
        active_request->m_request_decoder = rp_http_server_connection_callbacks_new_stream(me->m_callbacks,
                                                            response_encoder,
                                                            false);
        me->m_active_request = active_request;
NOISY_MSG_("active request %p", active_request);
        //TODO...RETURN_IF_ERROR(doFloodProtectionChecks())
    }
    return RpStatusCode_Ok;
}

OVERRIDE RpStatusCode_e
on_url_base(RpHttp1ConnectionImpl* self, const char* data, size_t length)
{
    NOISY_MSG_("(%p, %p, %zu)", self, data, length);
    ActiveRequest* active_request = RP_HTTP1_SERVER_CONNECTION_IMPL(self)->m_active_request;
NOISY_MSG_("active request %p", active_request);
    if (active_request)
    {
NOISY_MSG_("calling g_string_append_len(%p, %p(%.*s), %zu)", active_request->m_request_url, data, (int)length, data, length);
        g_string_append_len(active_request->m_request_url, data, length);
        //TODO...RETURN_IF_ERROR(checkMaxHeaderSize());
    }
    return RpStatusCode_Ok;
}

OVERRIDE void
on_body(RpHttp1ConnectionImpl* self, evbuf_t* data)
{
    NOISY_MSG_("(%p, %p(%zu))", self, data, data ? evbuffer_get_length(data) : 0);
    ActiveRequest* active_request = RP_HTTP1_SERVER_CONNECTION_IMPL(self)->m_active_request;
    if (active_request)
    {
        rp_stream_decoder_decode_data(RP_STREAM_DECODER(active_request->m_request_decoder), data, false);
    }
}

OVERRIDE void
on_reset_stream(RpHttp1ConnectionImpl* self, RpStreamResetReason_e reason)
{
    NOISY_MSG_("(%p, %d)", self, reason);
    RpHttp1ServerConnectionImpl* me = RP_HTTP1_SERVER_CONNECTION_IMPL(self);
    if (me->m_active_request)
    {
        rp_stream_callback_helper_run_reset_callbacks(RP_STREAM_CALLBACK_HELPER(me->m_active_request->m_response_encoder), reason, "");
        RpDispatcher* dispatcher = rp_network_connection_dispatcher(me->m_connection);
        rp_dispatcher_deferred_destroy(dispatcher, g_steal_pointer(&me->m_active_request), (GDestroyNotify)ActiveRequest_free);
    }
}

OVERRIDE void
on_encode_complete(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttp1ServerConnectionImpl* me = RP_HTTP1_SERVER_CONNECTION_IMPL(self);
NOISY_MSG_("remote complete %u", me->m_active_request->m_remote_complete);
    if (me->m_active_request->m_remote_complete)
    {
        RpDispatcher* dispatcher = rp_network_connection_dispatcher(me->m_connection);
        rp_dispatcher_deferred_destroy(dispatcher, g_steal_pointer(&me->m_active_request), (GDestroyNotify)ActiveRequest_free);
    }
}

OVERRIDE void
alloc_headers(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttp1ServerConnectionImpl* me = RP_HTTP1_SERVER_CONNECTION_IMPL(self);
    me->m_headers_or_trailers = evhtp_headers_new();
}

OVERRIDE void
alloc_trailers(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttp1ServerConnectionImpl* me = RP_HTTP1_SERVER_CONNECTION_IMPL(self);
    me->m_headers_or_trailers = evhtp_headers_new();
}

OVERRIDE evhtp_headers_t*
headers_or_trailers(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HTTP1_SERVER_CONNECTION_IMPL(self)->m_headers_or_trailers;
}

OVERRIDE evhtp_headers_t*
request_or_response_headers(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HTTP1_SERVER_CONNECTION_IMPL(self)->m_headers_or_trailers;
}

#ifndef EVHTP_RES_UPGRADEREQ
#define EVHTP_RES_UPGRADEREQ    426
#endif

static RpStatusCode_e
check_protocol_version(RpHttp1ServerConnectionImpl* self, evhtp_headers_t* request_headers)
{
    NOISY_MSG_("(%p, %p)", self, request_headers);

    g_return_val_if_fail(RP_IS_HTTP1_SERVER_CONNECTION_IMPL(self), RpStatusCode_CodecClientError);
    g_return_val_if_fail(request_headers != NULL, RpStatusCode_CodecClientError);

    if (rp_http_connection_protocol(RP_HTTP_CONNECTION(self)) == EVHTP_PROTO_10)
    {
        if (!self->m_settings.m_accept_http_10)
        {
            rp_http1_connection_impl_set_error_code(RP_HTTP1_CONNECTION_IMPL(self), EVHTP_RES_UPGRADEREQ);
        }
    }
    return RpStatusCode_Ok;
}

static inline char*
url_host_and_port(GUri* url)
{
    GString* s = g_string_new(g_uri_get_host(url));
    gint port = g_uri_get_port(url);
    if (port > 0)
    {
        g_string_append_printf(s, ":%d", port);
    }
    return g_string_free_and_steal(s);
}

static inline char*
url_path_and_query_params(GUri* url)
{
    NOISY_MSG_("(%p)", url);
    const gchar* path = g_uri_get_path(url);
    if (path && path[0])
    {
        GString* s = g_string_new(path);
        const char* query_params = g_uri_get_query(url);
        if (query_params && query_params[0])
        {
            g_string_append_c(s, ':');
            g_string_append(s, query_params);
        }
        return g_string_free_and_steal(s);
    }
    return NULL;
}

static inline GUri*
parse_request_url(const char* request_url, bool is_connect, GError** err)
{
    NOISY_MSG_("(%p(%s), %u, %p)", request_url, request_url, is_connect, err);
    if (is_connect)
    {
        g_autofree gchar* uri_str = g_strconcat("http://", request_url, NULL);
        return g_uri_parse(uri_str, G_URI_FLAGS_PARSE_RELAXED, err);
    }
    else
    {
        return g_uri_parse(request_url, G_URI_FLAGS_PARSE_RELAXED, err);
    }
}

static RpStatusCode_e
handle_path(RpHttp1ServerConnectionImpl* self, evhtp_headers_t* request_headers, const char* method)
{
    NOISY_MSG_("(%p, %p, %p(%s))", self, request_headers, method, method);

    const char* path = RpHeaderValues.Path;

    bool is_connect = (g_ascii_strcasecmp(method, RpHeaderValues.MethodValues.Connect) == 0);

    ActiveRequest* active_request = self->m_active_request;

NOISY_MSG_("active request %p", active_request);
NOISY_MSG_("request url %p", active_request->m_request_url);
NOISY_MSG_("request url str %p(%s)", active_request->m_request_url->str, active_request->m_request_url->str);
NOISY_MSG_("is_connect %u", is_connect);

    if (!is_connect && active_request->m_request_url->len &&
        (active_request->m_request_url->str[0] == '/' ||
            (g_ascii_strcasecmp(method, RpHeaderValues.MethodValues.Options) == 0 &&
                active_request->m_request_url->str[0] == '*')))
    {
        NOISY_MSG_("here");
        evhtp_headers_add_header(request_headers,
            evhtp_header_new(path, active_request->m_request_url->str, 0, 1));
        g_string_free_and_clear(&active_request->m_request_url);
        return RpStatusCode_Ok;
    }

    if (!self->m_settings.m_allow_absolute_url && !is_connect)
    {
        NOISY_MSG_("here");
        evhtp_headers_add_header(request_headers,
            evhtp_header_new(path, active_request->m_request_url->str, 0, 0));
        g_string_free_and_clear(&active_request->m_request_url);
        return RpStatusCode_Ok;
    }

    g_autoptr(GError) err = NULL;
    g_autoptr(GUri) absolute_url = parse_request_url(active_request->m_request_url->str, is_connect, &err);
    if (!absolute_url)
    {
        LOGD("error %d(%s)", err->code, err->message);
        LOGE("http/1.1 protocol error: invalid url in request line");
        //TODO...return codecProtocolError("...");
        return RpStatusCode_CodecProtocolError;
    }

    gchar* host_and_port = url_host_and_port(absolute_url);
    evhtp_headers_add_header(request_headers,
                    evhtp_header_new(RpHeaderValues.Host, host_and_port, 0, 1));
    // Set the requested server name for use by other modules throughout the
    // request lifecycle.
    rp_connection_info_setter_set_requested_server_name(
        rp_network_connection_connection_info_setter(self->m_connection), host_and_port);
    g_free(host_and_port);

    if (!is_connect)
    {
        NOISY_MSG_("here");
        evhtp_headers_add_header(request_headers,
            evhtp_header_new(RpHeaderValues.Scheme, g_uri_get_scheme(absolute_url), 0, 1));
        //TODO...
    }

    g_autofree gchar* path_and_query_params = url_path_and_query_params(absolute_url);
NOISY_MSG_("path and query params %p(%s)", path_and_query_params, path_and_query_params);
    if (path_and_query_params && path_and_query_params[0])
    {
        NOISY_MSG_("here");
        evhtp_headers_add_header(request_headers,
                        evhtp_header_new(path, path_and_query_params, 0, 1));
    }
    g_string_free_and_clear(&active_request->m_request_url);
    return RpStatusCode_Ok;
}

OVERRIDE RpStatusCode_e
on_headers_complete_base(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);

    RpHttp1ServerConnectionImpl* me = RP_HTTP1_SERVER_CONNECTION_IMPL(self);
    ActiveRequest* active_request = me->m_active_request;
    if (active_request)
    {
        evhtp_headers_t* headers = me->m_headers_or_trailers;
NOISY_MSG_("headers %p", headers);

        //TODO...handling_upgrade_ && connection header sanitation...

        RpParser* parser = rp_http1_connection_impl_get_parser(self);
        RpResponseEncoderImpl* response_encoder = active_request->m_response_encoder;
        RpStreamEncoderImpl* stream_encoder = RP_STREAM_ENCODER_IMPL(response_encoder);
        const char* method_name = rp_parser_method_name(parser);
        rp_stream_encoder_impl_set_is_response_to_head_request(stream_encoder,
            g_ascii_strcasecmp(method_name, RpHeaderValues.MethodValues.Head) == 0);
        rp_stream_encoder_impl_set_is_response_to_connect_request(stream_encoder,
            g_ascii_strcasecmp(method_name, RpHeaderValues.MethodValues.Connect) == 0);

        RETURN_IF_ERROR(handle_path(me, headers, method_name));

        evhtp_headers_add_header(headers,
                    evhtp_header_new(RpHeaderValues.Method, method_name, 0, 1));
        RETURN_IF_ERROR(check_protocol_version(me, headers));

        //TODO...requestHeadersValue(...)

        if (rp_parser_is_chunked(parser) ||
            rp_parser_content_length(parser) > 0 ||
            rp_http1_connection_impl_handling_upgrade_(self))
        {
            RpRequestDecoder* request_decoder = active_request->m_request_decoder;
            rp_request_decoder_decode_headers(request_decoder, headers, false);

            if (rp_network_connection_state(me->m_connection) != RpNetworkConnectionState_Open)
            {
                rp_parser_pause(rp_http1_connection_impl_get_parser(self));
            }
        }
        else
        {
            rp_http1_connection_impl_set_deferred_end_stream_headers(self, true);
        }
    }

    return RpStatusCode_Ok;
}

#include "rproxy.h"
static void
dump_headers(RpHttp1ServerConnectionImpl* me)
{
    NOISY_MSG_("(%p)", me);
    evbuf_t* buf = evbuffer_new();
NOISY_MSG_("headers %p", me->m_headers_or_trailers);
    evhtp_headers_for_each(me->m_headers_or_trailers, util_write_header_to_evbuffer, buf);
    evbuffer_add(buf, "\r\n", 2);
    LOGD("\n%.*s", (int)evbuffer_get_length(buf), evbuffer_pullup(buf, -1));
    evbuffer_free(buf);
}

OVERRIDE RpCallbackResult_e
on_message_complete_base(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);

g_assert(!rp_http1_connection_impl_handling_upgrade_(self));

    RpHttp1ServerConnectionImpl* me = RP_HTTP1_SERVER_CONNECTION_IMPL(self);
    ActiveRequest* active_request = me->m_active_request;
    if (active_request)
    {

        active_request->m_remote_complete = true;

        if (rp_http1_connection_impl_get_deferred_end_stream_headers(self))
        {
            rp_request_decoder_decode_headers(active_request->m_request_decoder, me->m_headers_or_trailers, true);
            rp_http1_connection_impl_set_deferred_end_stream_headers(self, false);
        }
        else if (rp_http1_connection_impl_processing_trailers(self))
        {
            rp_request_decoder_decode_trailers(active_request->m_request_decoder, me->m_headers_or_trailers);
        }
        else
        {
            evbuf_t* buffer = rp_http1_connection_impl_empty_buffer_(self);
            rp_stream_decoder_decode_data(RP_STREAM_DECODER(active_request->m_request_decoder), buffer, true);
        }

dump_headers(me);

        me->m_headers_or_trailers = NULL;
    }

    return rp_parser_pause(rp_http1_connection_impl_get_parser(self));
}

OVERRIDE void
on_above_high_watermark(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttp1ServerConnectionImpl* me = RP_HTTP1_SERVER_CONNECTION_IMPL(self);
    ActiveRequest* active_request = me->m_active_request;
    if (active_request)
    {
        rp_stream_callback_helper_run_high_watermark_callbacks(RP_STREAM_CALLBACK_HELPER(active_request->m_response_encoder));
    }
}

OVERRIDE void
on_below_low_watermark(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttp1ServerConnectionImpl* me = RP_HTTP1_SERVER_CONNECTION_IMPL(self);
    ActiveRequest* active_request = me->m_active_request;
    if (active_request)
    {
        rp_stream_callback_helper_run_low_watermark_callbacks(RP_STREAM_CALLBACK_HELPER(active_request->m_response_encoder));
    }
}

OVERRIDE RpStatusCode_e
send_protocol_error(RpHttp1ConnectionImpl* self, const char* details)
{
    NOISY_MSG_("(%p, %p(%s))", self, details, details);
    RpHttp1ServerConnectionImpl* me = RP_HTTP1_SERVER_CONNECTION_IMPL(self);
    ActiveRequest* active_request = me->m_active_request;
    if (!active_request)
    {
        RETURN_IF_ERROR(rp_http1_connection_impl_on_message_begin_impl(self));
    }

    if (rp_http1_connection_impl_reset_stream_called(self))
    {
        return RpStatusCode_Ok;
    }

    //TODO...active_request_->response_encoder_.setDetails(details);
    if (rp_response_encoder_impl_started_reponse(active_request->m_response_encoder))
    {
        rp_request_decoder_send_local_reply(active_request->m_request_decoder,
                                            rp_http1_connection_impl_error_code_(self),
                                            NULL/*TODO...CodeUtility::toString(error_code_)*/,
                                            NULL,
                                            details,
                                            self);
    }
    return RpStatusCode_Ok;
}

static void
http1_connection_impl_class_init(RpHttp1ConnectionImplClass* klass)
{
    LOGD("(%p)", klass);
    klass->supports_http_10 = supports_http_10;
    klass->on_url_base = on_url_base;
    klass->on_body = on_body;
    klass->on_reset_stream = on_reset_stream;
    klass->on_encode_complete = on_encode_complete;
    klass->alloc_headers = alloc_headers;
    klass->alloc_trailers = alloc_trailers;
    klass->headers_or_trailers = headers_or_trailers;
    klass->on_message_begin_base = on_message_begin_base;
    klass->request_or_response_headers = request_or_response_headers;
    klass->on_headers_complete_base = on_headers_complete_base;
    klass->on_message_complete_base = on_message_complete_base;
    klass->on_above_high_watermark = on_above_high_watermark;
    klass->on_below_low_watermark = on_below_low_watermark;
    klass->send_protocol_error = send_protocol_error;
}

static void
rp_http1_server_connection_impl_class_init(RpHttp1ServerConnectionImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    http1_connection_impl_class_init(RP_HTTP1_CONNECTION_IMPL_CLASS(klass));
}

static void
rp_http1_server_connection_impl_init(RpHttp1ServerConnectionImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpHttp1ServerConnectionImpl*
rp_http1_server_connection_impl_new(RpNetworkConnection* connection, RpHttpServerConnectionCallbacks* callbacks,
                                    const struct RpHttp1Settings_s* settings,
                                    guint32 max_request_headers_kb,
                                    guint32 max_request_headers_count)
{
    NOISY_MSG_("(%p, %p, %p, %u, %u)",
        connection, callbacks, settings, max_request_headers_kb, max_request_headers_count);
    RpHttp1ServerConnectionImpl* self = g_object_new(RP_TYPE_HTTP1_SERVER_CONNECTION_IMPL,
                                                        "connection", connection,
                                                        "codec-settings", settings,
                                                        "message-type", RpMessageType_Request,
                                                        "max-headers-kb", max_request_headers_kb,
                                                        "max-headers-count", max_request_headers_count,
                                                        NULL);
    self->m_callbacks = callbacks;
    self->m_connection = connection;
    self->m_settings = *settings;
    return self;
}
