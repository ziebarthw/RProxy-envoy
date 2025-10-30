/*
 * rp-http1-connection-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_http1_connection_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_http1_connection_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-codec.h"
#include "rp-headers.h"
#include "rp-http-utility.h"
#include "rp-parser.h"
#include "rp-legacy-http-parser-impl.h"
#include "rp-http1-connection-impl.h"

typedef enum {
    RpHeaderParsingState_Field,
    RpHeaderParsingState_Value,
    RpHeaderParsingState_Done
} RpHeaderParsingState_e;

typedef struct _RpHttp1ConnectionImplPrivate RpHttp1ConnectionImplPrivate;
struct _RpHttp1ConnectionImplPrivate {

    RpNetworkConnection* m_connection;

    RpParser* m_parser;

    evbuf_t* m_buffered_body;
    evbuf_t* m_empty_buffer;
    evbuf_t* m_current_dispatching_buffer;
    evbuf_t* m_output_buffer; // Not owned.

    GString* m_current_header_field;
    GString* m_current_header_value;

    struct RpHttp1Settings_s* m_codec_settings;

    evhtp_res m_error_code; //EVHTP_RES_BADREQ
    evhtp_proto m_protocol;

    RpStatusCode_e m_codec_status;
    RpMessageType_e m_message_type;
    RpHeaderParsingState_e m_header_parsing_state;

    guint32 m_max_headers_kb;
    guint32 m_max_headers_count;

    bool m_processing_trailers : 1;
    bool m_handling_upgrade : 1;
    bool m_reset_stream_called : 1;
    bool m_deferred_end_stream_headers : 1;
    bool m_dispatching : 1;
    bool m_dispatching_slice_already_drained : 1;
};

enum
{
    PROP_0, // Reserved.
    PROP_CONNECTION,
    PROP_CODEC_SETTINGS,
    PROP_MESSAGE_TYPE,
    PROP_MAX_HEADERS_KB,
    PROP_MAX_HEADERS_COUNT,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void http_connection_interface_init(RpHttpConnectionInterface* iface);
static void parser_callbacks_interface_init(RpParserCallbacksInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpHttp1ConnectionImpl, rp_http1_connection_impl, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpHttp1ConnectionImpl)
    G_IMPLEMENT_INTERFACE(RP_TYPE_HTTP_CONNECTION, http_connection_interface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_PARSER_CALLBACKS, parser_callbacks_interface_init)
)

#define PRIV(obj) \
    ((RpHttp1ConnectionImplPrivate*) rp_http1_connection_impl_get_instance_private(RP_HTTP1_CONNECTION_IMPL(obj)))

static inline evbuf_t*
ensure_empty_buffer(RpHttp1ConnectionImplPrivate* me)
{
    NOISY_MSG_("(%p)", me);
    if (me->m_empty_buffer)
    {
        NOISY_MSG_("pre-allocated empty buffer %p", me->m_empty_buffer);
        return me->m_empty_buffer;
    }
    me->m_empty_buffer = evbuffer_new();
    NOISY_MSG_("allocated empty buffer %p", me->m_empty_buffer);
    return me->m_empty_buffer;
}

static inline RpStatusCode_e
dispatch_slice(RpHttp1ConnectionImpl* self, const char* slice, size_t len, size_t* nread)
{
    NOISY_MSG_("(%p, %p, %zu, %p)", self, slice, len, nread);

    RpHttp1ConnectionImplPrivate* me = PRIV(self);
    RpParser* parser = me->m_parser;

    *nread = rp_parser_execute(parser, slice, len);
    if (me->m_codec_status != RpStatusCode_Ok)
    {
        NOISY_MSG_("returning %d", me->m_codec_status);
        return me->m_codec_status;
    }

    RpParserStatus_e status = rp_parser_get_status(parser);
    if (status != RpParserStatus_Ok && status != RpParserStatus_Paused)
    {
        NOISY_MSG_("returning CodecProtocolError %d", RpStatusCode_CodecProtocolError);
        //TODO...
        return RpStatusCode_CodecProtocolError;
    }

    NOISY_MSG_("returning Ok");
    return RpStatusCode_Ok;
}

static inline void
on_dispatch(RpHttp1ConnectionImpl* self G_GNUC_UNUSED, evbuf_t* data G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p(%zu))", self, data, evbuffer_get_length(data));
    //TODO...getBytesMeter().addWireBytesReceived(data.length());
}

static bool
maybe_direct_dispatch(RpHttp1ConnectionImpl* self, evbuf_t* data)
{
    NOISY_MSG_("(%p, %p(%zu))", self, data, evbuffer_get_length(data));

    if (!PRIV(self)->m_handling_upgrade)
    {
        NOISY_MSG_("only direct dispatch for Upgrade requests");
        return false;
    }

    NOISY_MSG_("direct-dispatched %zu bytes", evbuffer_get_length(data));
    rp_http1_connection_impl_on_body(self, data);
    evbuffer_drain(data, evbuffer_get_length(data));
    return true;
}

static inline void
dispatch_buffered_body(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    //TODO...asserts()
    evbuf_t* buffered_body = PRIV(self)->m_buffered_body;
    if (evbuffer_get_length(buffered_body) > 0)
    {
        rp_http1_connection_impl_on_body(self, buffered_body);
        evbuffer_drain(buffered_body, -1);
    }
}

static RpStatusCode_e
dispatch(RpHttp1ConnectionImpl* self, evbuf_t* data)
{
    NOISY_MSG_("(%p, %p(%zu))", self, data, evbuffer_get_length(data));

    RpHttp1ConnectionImplPrivate* me = PRIV(self);

    me->m_dispatching = true;
    on_dispatch(self, data);
    if (maybe_direct_dispatch(self, data))
    {
        NOISY_MSG_("returning");
        return RpStatusCode_Ok;
    }

    RpParser* parser = me->m_parser;
    rp_parser_resume(parser);

    size_t total_parsed = 0;
    if (evbuffer_get_length(data) > 0)
    {
        me->m_current_dispatching_buffer = data;
        while (evbuffer_get_length(data) > 0)
        {
            struct evbuffer_iovec v[1];
            int n = evbuffer_peek(data, -1, NULL, v, 1);
            if (n != 1)
            {
                LOGD("peek() returned %d", n);
            }

            me->m_dispatching_slice_already_drained = false;

            size_t nparsed;
            RpStatusCode_e status = dispatch_slice(self, v[0].iov_base, v[0].iov_len, &nparsed);
            if (status != RpStatusCode_Ok)
            {
                NOISY_MSG_("returning %d", status);
                return status;
            }
            if (!me->m_dispatching_slice_already_drained)
            {
                g_assert(nparsed <= v[0].iov_len);
                NOISY_MSG_("draining %zu bytes", nparsed);
                evbuffer_drain(data, nparsed);
            }

            total_parsed += nparsed;
            if (rp_parser_get_status(parser) != RpParserStatus_Ok)
            {
                NOISY_MSG_("breaking on status %d", rp_parser_get_status(parser));
                break;
            }
        }
        me->m_current_dispatching_buffer = NULL;
        dispatch_buffered_body(self);
    }
    else
    {
        size_t nparsed;
        RpStatusCode_e result = dispatch_slice(self, NULL, 0, &nparsed);
        if (result != RpStatusCode_Ok)
        {
            NOISY_MSG_("failed");
            return result;
        }
    }

    NOISY_MSG_("parsed %zu bytes", total_parsed);

    maybe_direct_dispatch(self, data);
    return RpStatusCode_Ok;
}

static RpStatusCode_e
dispatch_i(RpHttpConnection* self, evbuf_t* data)
{
    NOISY_MSG_("(%p, %p(%zu))", self, data, data ? evbuffer_get_length(data) : 0);
    return dispatch(RP_HTTP1_CONNECTION_IMPL(self), data);
}

static void
go_away_i(RpHttpConnection* self)
{
    NOISY_MSG_("(%p)", self);
}

static evhtp_proto
protocol_i(RpHttpConnection* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_protocol;
}

static void
shutdown_notice_i(RpHttpConnection* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static bool
wants_to_write_i(RpHttpConnection* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return false;
}

static bool
should_keep_alive_i(RpHttpConnection* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_parser_should_keep_alive(PRIV(self)->m_parser);
}

static void
http_connection_interface_init(RpHttpConnectionInterface* iface)
{
    LOGD("(%p)", iface);
    iface->dispatch = dispatch_i;
    iface->go_away = go_away_i;
    iface->protocol = protocol_i;
    iface->shutdown_notice = shutdown_notice_i;
    iface->wants_to_write = wants_to_write_i;
    iface->should_keep_alive = should_keep_alive_i;
}

static RpStatusCode_e
on_message_begin_impl(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttp1ConnectionImplPrivate* me = PRIV(self);
    me->m_protocol = EVHTP_PROTO_11;
    me->m_processing_trailers = false;
    me->m_header_parsing_state = RpHeaderParsingState_Field;
    RpHttp1ConnectionImplClass* klass = RP_HTTP1_CONNECTION_IMPL_GET_CLASS(self);
    klass->alloc_headers(self);
    return klass->on_message_begin_base(self);
}

static RpCallbackResult_e
set_and_check_callback_status(RpHttp1ConnectionImpl* self, RpStatusCode_e status)
{
    NOISY_MSG_("(%p, %d)", self, status);
    RpHttp1ConnectionImplPrivate* me = PRIV(self);
    me->m_codec_status = status;
    return me->m_codec_status == RpStatusCode_Ok ?
                            RpCallbackResult_Success : RpCallbackResult_Error;
}

static void
buffer_body_i(RpParserCallbacks* self, const char * data, size_t length)
{
    NOISY_MSG_("(%p, %p, %zu)", self, data, length);

    RpHttp1ConnectionImplPrivate* me = PRIV(self);

    struct evbuffer_iovec v[1];
    int n = evbuffer_peek(me->m_current_dispatching_buffer, -1, NULL, v, 1);
    if (n != 1)
    {
        LOGD("peek() returned %d", n);
    }
    if ((void*)data == v[0].iov_base && length == v[0].iov_len)
    {
        evbuffer_add(me->m_buffered_body, v[0].iov_base, length);
        evbuffer_drain(me->m_current_dispatching_buffer, length);
        me->m_dispatching_slice_already_drained = true;
    }
    else
    {
        evbuffer_add(me->m_buffered_body, data, length);
    }
}

static RpCallbackResult_e
on_message_begin_i(RpParserCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttp1ConnectionImpl* me = RP_HTTP1_CONNECTION_IMPL(self);
    return set_and_check_callback_status(me, on_message_begin_impl(me));
}

static RpCallbackResult_e
on_url_i(RpParserCallbacks* self, const char* data, size_t length)
{
    NOISY_MSG_("(%p, %p(%.*s), %zu)", self, data, (int)length, data, length);
    RpHttp1ConnectionImpl* me = RP_HTTP1_CONNECTION_IMPL(self);
    return set_and_check_callback_status(me,
                                RP_HTTP1_CONNECTION_IMPL_GET_CLASS(me)->on_url_base(me, data, length));
}

static RpCallbackResult_e
on_status_i(RpParserCallbacks* self, const char* data, size_t length)
{
    NOISY_MSG_("(%p, %p, %zu)", self, data, length);
    RpHttp1ConnectionImpl* me = RP_HTTP1_CONNECTION_IMPL(self);
    return set_and_check_callback_status(me,
                                RP_HTTP1_CONNECTION_IMPL_GET_CLASS(me)->on_status_base(me));
}

static RpStatusCode_e
complete_current_header(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttp1ConnectionImplPrivate* me = PRIV(self);
    evhtp_headers_t* headers_or_trailers = RP_HTTP1_CONNECTION_IMPL_GET_CLASS(self)->headers_or_trailers(self);

    //TODO...getBytesMeter().addHeaderBytesReceived(CRLF_SIZE + 1);

    if (me->m_current_header_field->len > 0)
    {
        GString* field = g_string_ascii_down(me->m_current_header_field);
        gchar* value = g_strchomp(me->m_current_header_value->str);

NOISY_MSG_("%s: \"%s\"", field->str, value);

        evhtp_headers_add_header(headers_or_trailers,
                                    evhtp_header_new(field->str, value, 1, 1));
        g_string_truncate(field, 0);
        g_string_truncate(me->m_current_header_value, 0);
    }

    //TODO...check if the number of headers exceeds the limit...

    me->m_header_parsing_state = RpHeaderParsingState_Field;
    return RpStatusCode_Ok;
}

static inline RpStatusCode_e
on_header_field_impl(RpHttp1ConnectionImpl* self, const char* data, size_t length)
{
    NOISY_MSG_("(%p, %p(%.*s), %zu)", self, data, (int)length, data, length);
    RpHttp1ConnectionImplPrivate* me = PRIV(self);
    //TODO...ASSERT(dispatching_);

    //TODO...getBytesMeter().addHeaderBytesReceived(length);

    if (me->m_header_parsing_state == RpHeaderParsingState_Done)
    {
        if (!rp_http1_connection_impl_enable_trailers(self))
        {
            NOISY_MSG_("ignore trailers");
            return RpStatusCode_Ok;
        }
        me->m_processing_trailers = true;
        me->m_header_parsing_state = RpHeaderParsingState_Field;
        //TODO...allocTrailers();
    }

    if (me->m_header_parsing_state == RpHeaderParsingState_Value)
    {
        RpStatusCode_e status = complete_current_header(self);
        if (status != RpStatusCode_Ok)
        {
            return status;
        }
    }

    g_string_append_len(me->m_current_header_field, data, length);

    //TODO...return checkMaxHeadersSize();
    return RpStatusCode_Ok;
}

static inline RpStatusCode_e
on_header_value_impl(RpHttp1ConnectionImpl* self, const char* data, size_t length)
{
    NOISY_MSG_("(%p, %p, %zu)", self, data, length);
    RpHttp1ConnectionImplPrivate* me = PRIV(self);
    //TODO...ASSERT(dispatching_);

    //TODO...getBytesMeter().addHeaderBytesReceived(length);

    if (me->m_header_parsing_state == RpHeaderParsingState_Done && !rp_http1_connection_impl_enable_trailers(self))
    {
        NOISY_MSG_("ignore trailers");
        return RpStatusCode_Ok;
    }

    struct string_view_s header_value = string_view_ctor(data, length);

    me->m_header_parsing_state = RpHeaderParsingState_Value;
    if (!me->m_current_header_value->len)
    {
        header_value = string_view_ltrim(header_value);
    }
    g_string_append_len(me->m_current_header_value, header_value.m_data, header_value.m_length);

    //TODO...return checkMaxHeaderSize();
    return RpStatusCode_Ok;
}

static RpCallbackResult_e
on_header_field_i(RpParserCallbacks* self, const char* data, size_t length)
{
    NOISY_MSG_("(%p, %p(%.*s), %zu)", self, data, (int)length, data, length);
    RpHttp1ConnectionImpl* me = RP_HTTP1_CONNECTION_IMPL(self);
    return set_and_check_callback_status(me, on_header_field_impl(me, data, length));
}

static RpCallbackResult_e
on_header_value_i(RpParserCallbacks* self, const char* data, size_t length)
{
    NOISY_MSG_("(%p, %p(%.*s), %zu)", self, data, (int)length, data, length);
    RpHttp1ConnectionImpl* me = RP_HTTP1_CONNECTION_IMPL(self);
    return set_and_check_callback_status(me, on_header_value_impl(me, data, length));
}

static inline RpStatusCode_e
on_headers_complete_impl(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);

    RETURN_IF_ERROR(complete_current_header(self));

    RpHttp1ConnectionImplPrivate* me = PRIV(self);
    RpParser* parser = me->m_parser;

    if (!rp_parser_is_http_11(parser))
    {
        me->m_protocol = EVHTP_PROTO_10;
    }
    evhtp_headers_t* request_or_response_headers =
        RP_HTTP1_CONNECTION_IMPL_GET_CLASS(self)->request_or_response_headers(self);
    //TODO...upgrade stuff????

    const char* method_name = rp_parser_method_name(parser);
    if (!method_name) method_name = "";
    if (g_ascii_strcasecmp(method_name, RpHeaderValues.MethodValues.Connect) == 0)
    {
        NOISY_MSG_("CONNECT");
        evhtp_header_t* h = evhtp_headers_find_header(request_or_response_headers, RpHeaderValues.ContentLength);
        if (h)
        {
            if (h->vlen == 1 && h->val[0] == '0')
            {
                NOISY_MSG_("removing content-length header");
                evhtp_header_rm_and_free(request_or_response_headers, h);
            }
            else
            {
                me->m_error_code = EVHTP_RES_BADREQ;
                RETURN_IF_ERROR(rp_http1_connection_impl_send_protocol_error(self, "body-disallowed"));
                return RpStatusCode_CodecProtocolError;
            }
        }
        me->m_handling_upgrade = true;
    }

    if (rp_parser_has_transfer_encoding(parser) != 0 && evhtp_header_find(request_or_response_headers, RpHeaderValues.ContentLength))
    {
        if (rp_parser_is_chunked(parser) && me->m_codec_settings->m_allow_chunked_length)
        {
            NOISY_MSG_("removing content-length header");
            evhtp_header_rm_and_free(request_or_response_headers,
                evhtp_headers_find_header(request_or_response_headers, RpHeaderValues.ContentLength));
        }
        else
        {
            me->m_error_code = EVHTP_RES_BADREQ;
            RETURN_IF_ERROR(rp_http1_connection_impl_send_protocol_error(self, "chunked-content-length"));
            return RpStatusCode_CodecProtocolError;
        }
    }

    const char* encoding = evhtp_header_find(request_or_response_headers, RpHeaderValues.TransferEncoding);
    if (encoding)
    {
        if ((g_ascii_strcasecmp(encoding, RpHeaderValues.TransferCodingValues.Chunked) != 0) ||
            (g_ascii_strcasecmp(method_name, RpHeaderValues.MethodValues.Connect) == 0))
        {
            me->m_error_code = EVHTP_RES_NOTIMPL;
            RETURN_IF_ERROR(rp_http1_connection_impl_send_protocol_error(self, "invalid-transfer-encoding"));
            return RpStatusCode_CodecProtocolError;
        }
    }

    RETURN_IF_ERROR(RP_HTTP1_CONNECTION_IMPL_GET_CLASS(self)->on_headers_complete_base(self));

    me->m_header_parsing_state = RpHeaderParsingState_Done;

    return /*TODO...me->m_handling_upgrade ? RpCallbackResult_NoBodyData :*/ RpStatusCode_Ok;
}

static RpCallbackResult_e
on_headers_complete_i(RpParserCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttp1ConnectionImpl* me = RP_HTTP1_CONNECTION_IMPL(self);
    return set_and_check_callback_status(me, on_headers_complete_impl(me));
}

static inline RpCallbackResult_e
on_message_complete_impl(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);

    dispatch_buffered_body(self);

    RpHttp1ConnectionImplPrivate* me = PRIV(self);
    if (me->m_handling_upgrade)
    {
        g_assert(!me->m_deferred_end_stream_headers);
        NOISY_MSG_("Pausing parser due to upgrade");
        return rp_parser_pause(me->m_parser);
    }

    if (me->m_header_parsing_state == RpHeaderParsingState_Value)
    {
        RETURN_IF_ERROR(complete_current_header(self));
    }

    return RP_HTTP1_CONNECTION_IMPL_GET_CLASS(self)->on_message_complete_base(self);
}

static RpCallbackResult_e
on_message_complete_i(RpParserCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttp1ConnectionImpl* me = RP_HTTP1_CONNECTION_IMPL(self);
    return set_and_check_callback_status(me, on_message_complete_impl(me));
}

static void
on_chunk_header_i(RpParserCallbacks* self, bool is_final_chunk)
{
    NOISY_MSG_("(%p, %u)", self, is_final_chunk);
    if (is_final_chunk)
    {
        dispatch_buffered_body(RP_HTTP1_CONNECTION_IMPL(self));
    }
}

static void
parser_callbacks_interface_init(RpParserCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_message_begin = on_message_begin_i;
    iface->on_url = on_url_i;
    iface->on_status = on_status_i;
    iface->buffer_body = buffer_body_i;
    iface->on_header_field = on_header_field_i;
    iface->on_header_value = on_header_value_i;
    iface->on_headers_complete = on_headers_complete_i;
    iface->on_message_complete = on_message_complete_i;
    iface->on_chunk_header = on_chunk_header_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_CONNECTION:
            g_value_set_object(value, PRIV(obj)->m_connection);
            break;
        case PROP_CODEC_SETTINGS:
            g_value_set_pointer(value, PRIV(obj)->m_codec_settings);
            break;
        case PROP_MESSAGE_TYPE:
            g_value_set_enum(value, PRIV(obj)->m_message_type);
            break;
        case PROP_MAX_HEADERS_COUNT:
            g_value_set_int(value, PRIV(obj)->m_max_headers_count);
            break;
        case PROP_MAX_HEADERS_KB:
            g_value_set_int(value, PRIV(obj)->m_max_headers_kb);
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
        case PROP_CONNECTION:
            PRIV(obj)->m_connection = g_value_get_object(value);
            break;
        case PROP_CODEC_SETTINGS:
            PRIV(obj)->m_codec_settings = g_value_get_pointer(value);
            break;
        case PROP_MESSAGE_TYPE:
            PRIV(obj)->m_message_type = g_value_get_enum(value);
            break;
        case PROP_MAX_HEADERS_COUNT:
            PRIV(obj)->m_max_headers_count = g_value_get_uint(value);
            break;
        case PROP_MAX_HEADERS_KB:
            PRIV(obj)->m_max_headers_kb = g_value_get_uint(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
dispose(GObject* object)
{
    NOISY_MSG_("(%p)", object);

    RpHttp1ConnectionImplPrivate* me = PRIV(object);
    g_clear_pointer(&me->m_buffered_body, evbuffer_free);
    g_clear_pointer(&me->m_empty_buffer, evbuffer_free);
    g_clear_pointer(&me->m_output_buffer, evbuffer_free);
    g_string_free(me->m_current_header_field, TRUE);
    g_string_free(me->m_current_header_value, TRUE);
    g_clear_object(&me->m_parser);

    G_OBJECT_CLASS(rp_http1_connection_impl_parent_class)->dispose(object);
}

static bool
supports_http_10_(RpHttp1ConnectionImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return false;
}

static void
maybe_add_sentinel_buffer_fragment_(RpHttp1ConnectionImpl* self G_GNUC_UNUSED, evbuf_t* output_buffer G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p(%zu))", self, output_buffer, output_buffer ? evbuffer_get_length(output_buffer) : 0);
    //TODO:...flood protection...
}

static RpParser*
create_parser(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_PARSER(rp_legacy_http_parser_impl_new(PRIV(self)->m_message_type,
                                                    RP_PARSER_CALLBACKS(self)));
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_http1_connection_impl_parent_class)->constructed(obj);

    RpHttp1ConnectionImpl* self = RP_HTTP1_CONNECTION_IMPL(obj);
    RpHttp1ConnectionImplPrivate* me = PRIV(self);
    me->m_parser = create_parser(self);
    me->m_buffered_body = evbuffer_new();
    me->m_header_parsing_state = RpHeaderParsingState_Field;
    me->m_current_header_field = g_string_new(NULL);
    me->m_current_header_value = g_string_new(NULL);
    me->m_output_buffer = evbuffer_new();
}

static void
rp_http1_connection_impl_class_init(RpHttp1ConnectionImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    klass->supports_http_10 = supports_http_10_;
    klass->maybe_add_sentinel_buffer_fragment = maybe_add_sentinel_buffer_fragment_;

    obj_properties[PROP_CONNECTION] = g_param_spec_object("connection",
                                                    "Connection",
                                                    "Network Connection Instance",
                                                    RP_TYPE_NETWORK_CONNECTION,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_CODEC_SETTINGS] = g_param_spec_pointer("codec-settings",
                                                    "Codec settings",
                                                    "Codec Settings",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_MESSAGE_TYPE] = g_param_spec_enum("message-type",
                                                    "Message type",
                                                    "Message Type",
                                                    RP_TYPE_MESSAGE_TYPE,
                                                    RpMessageType_Request,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_MAX_HEADERS_COUNT] = g_param_spec_uint("max-headers-count",
                                                    "Max headers count",
                                                    "Max number of headers",
                                                    0,
                                                    G_MAXUINT32,
                                                    G_MAXUINT32,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_MAX_HEADERS_KB] = g_param_spec_uint("max-headers-kb",
                                                    "Max headers kb",
                                                    "Max KB headers length",
                                                    0,
                                                    G_MAXUINT32,
                                                    G_MAXUINT32,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_http1_connection_impl_init(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);

    RpHttp1ConnectionImplPrivate* me = PRIV(self);
    me->m_processing_trailers = false;
    me->m_handling_upgrade = false;
    me->m_reset_stream_called = false;
    me->m_deferred_end_stream_headers = false;
    me->m_dispatching = false;
}

void
rp_http1_connection_impl_buffer_add(RpHttp1ConnectionImpl* self, const char* data, size_t len)
{
    LOGD("(%p, %p, %zu)", self, data, len);
    g_return_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self));
    evbuffer_add(PRIV(self)->m_output_buffer, data, len);
}

void
rp_http1_connection_impl_buffer_move(RpHttp1ConnectionImpl* self, evbuf_t* data)
{
    LOGD("(%p, %p(%zu))", self, data, evbuffer_get_length(data));
    g_return_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self));
    evbuffer_add_buffer(PRIV(self)->m_output_buffer, data);
}

guint64
rp_http1_connection_impl_flush_output(RpHttp1ConnectionImpl* self, bool end_encode)
{
    LOGD("(%p, %u)", self, end_encode);
    g_return_val_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self), 0);
    RpHttp1ConnectionImplPrivate* me = PRIV(self);
    if (end_encode)
    {
        rp_http1_connection_impl_maybe_add_sentinel_buffer_fragment(self, me->m_output_buffer);
    }
    guint64 bytes_encoded = evbuffer_get_length(me->m_output_buffer);
    rp_network_connection_write(me->m_connection, me->m_output_buffer, false);
    return bytes_encoded;
}

bool
rp_http1_connection_impl_enable_trailers(RpHttp1ConnectionImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self), false);
    //TODO: return codec_settings.enable_trailers_
    return false;
}

RpNetworkConnection*
rp_http1_connection_impl_connection(RpHttp1ConnectionImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self), NULL);
    return PRIV(self)->m_connection;
}

void
rp_http1_connection_impl_on_reset_stream_base(RpHttp1ConnectionImpl* self, RpStreamResetReason_e reason)
{
    LOGD("(%p, %d)", self, reason);
    g_return_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self));
    PRIV(self)->m_reset_stream_called = true;
    RP_HTTP1_CONNECTION_IMPL_GET_CLASS(self)->on_reset_stream(self, reason);
}

bool
rp_http1_connection_impl_reset_stream_called(RpHttp1ConnectionImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self), true);
    return PRIV(self)->m_reset_stream_called;
}

void
rp_http1_connection_impl_set_error_code(RpHttp1ConnectionImpl* self, evhtp_res code)
{
    LOGD("(%p, %d)", self, code);
    g_return_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self));
    PRIV(self)->m_error_code = code;
}

RpParser*
rp_http1_connection_impl_get_parser(RpHttp1ConnectionImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self), NULL);
    return PRIV(self)->m_parser;
}

void
rp_http1_connection_impl_set_deferred_end_stream_headers(RpHttp1ConnectionImpl* self, bool v)
{
    LOGD("(%p, %u)", self, v);
    g_return_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self));
    PRIV(self)->m_deferred_end_stream_headers = v;
}

bool
rp_http1_connection_impl_get_deferred_end_stream_headers(RpHttp1ConnectionImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self), NULL);
    return PRIV(self)->m_deferred_end_stream_headers;
}

void
rp_http1_connection_impl_read_disable(RpHttp1ConnectionImpl* self, bool disable)
{
    LOGD("(%p, %u)", self, disable);
    g_return_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self));
    RpNetworkConnection* connection = PRIV(self)->m_connection;
    if (rp_network_connection_state(connection) == RpNetworkConnectionState_Open)
    {
        rp_network_connection_read_disable(connection, disable);
    }
}

evbuf_t*
rp_http1_connection_impl_buffer(RpHttp1ConnectionImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self), NULL);
    return PRIV(self)->m_output_buffer;
}

bool
rp_http1_connection_impl_processing_trailers(RpHttp1ConnectionImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self), NULL);
    return PRIV(self)->m_processing_trailers;
}

RpStatusCode_e
rp_http1_connection_impl_on_message_begin_impl(RpHttp1ConnectionImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self), RpStatusCode_Ok);
    return on_message_begin_impl(self);
}

evhtp_res
rp_http1_connection_impl_error_code_(RpHttp1ConnectionImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self), EVHTP_RES_BADREQ);
    return PRIV(self)->m_error_code;
}

evbuf_t*
rp_http1_connection_impl_empty_buffer_(RpHttp1ConnectionImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self), NULL);
    return ensure_empty_buffer(PRIV(self));
}

bool
rp_http1_connection_impl_handling_upgrade_(RpHttp1ConnectionImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self), false);
    return PRIV(self)->m_handling_upgrade;
}

void
rp_http1_connection_impl_set_handling_upgrade_(RpHttp1ConnectionImpl* self, bool val)
{
    LOGD("(%p, %u)", self, val);
    g_return_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(self));
    PRIV(self)->m_handling_upgrade = val;
}
