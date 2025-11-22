/*
 * rp-http1-client-connection-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_http1_client_connection_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_http1_client_connection_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-header-utility.h"
#include "rp-headers.h"
#include "rp-request-encoder-impl.h"
#include "rp-http1-client-connection-impl.h"

typedef struct _PendingResponse PendingResponse;
struct _PendingResponse {
    RpRequestEncoderImpl* m_encoder;
    RpResponseDecoder* m_decoder;
};

static inline PendingResponse*
PendingResponse_new(RpHttp1ConnectionImpl* connection, RpResponseDecoder* decoder)
{
    NOISY_MSG_("(%p, %p)", connection, decoder);
    PendingResponse* self = g_new0(struct _PendingResponse, 1);
    self->m_encoder = rp_request_encoder_impl_new(connection);
    self->m_decoder = g_object_ref(decoder);
    return self;
}

static inline void
PendingResponse_free(PendingResponse* self)
{
    NOISY_MSG_("(%p)", self);
    g_clear_object(&self->m_encoder);
    g_clear_object(&self->m_decoder);
    g_free(self);
}

struct _RpHttp1ClientConnectionImpl {
    RpHttp1ConnectionImpl parent_instance;

//    RpConnectionCallbacks* m_callbacks;
    //evbuf_t* owned_output_buffer_;
    PendingResponse* m_pending_response;
    evhtp_headers_t* m_headers_or_trailers;
    bool m_pending_response_done : 1;
    bool m_ignore_message_complete_for_1xx : 1;
    bool m_passing_through_proxy : 1;
    bool m_force_reset_on_premature_upstream_half_close : 1;
    bool m_encode_complete : 1;
};

static void http_client_connection_iface_init(RpHttpClientConnectionInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpHttp1ClientConnectionImpl, rp_http1_client_connection_impl, RP_TYPE_HTTP1_CONNECTION_IMPL,
    G_IMPLEMENT_INTERFACE(RP_TYPE_HTTP_CLIENT_CONNECTION, http_client_connection_iface_init)
)

static RpRequestEncoder*
new_stream_i(RpHttpClientConnection* self, RpResponseDecoder* response_decoder)
{
    NOISY_MSG_("(%p, %p)", self, response_decoder);
    RpHttp1ClientConnectionImpl* me = RP_HTTP1_CLIENT_CONNECTION_IMPL(self);
    g_assert(rp_network_connection_read_enabled(rp_http1_connection_impl_connection_(RP_HTTP1_CONNECTION_IMPL(self))));
    g_assert(!me->m_pending_response);
    g_assert(me->m_pending_response_done);
    me->m_pending_response = PendingResponse_new(RP_HTTP1_CONNECTION_IMPL(self), response_decoder);
    me->m_pending_response_done = false;
    return RP_REQUEST_ENCODER(me->m_pending_response->m_encoder);
}

static void
http_client_connection_iface_init(RpHttpClientConnectionInterface* iface)
{
    LOGD("(%p)", iface);
    iface->new_stream = new_stream_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpHttp1ClientConnectionImpl* self = RP_HTTP1_CLIENT_CONNECTION_IMPL(obj);
    g_clear_pointer(&self->m_pending_response, PendingResponse_free);

    G_OBJECT_CLASS(rp_http1_client_connection_impl_parent_class)->dispose(obj);
}

OVERRIDE void
on_encode_complete(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    RP_HTTP1_CLIENT_CONNECTION_IMPL(self)->m_encode_complete = true;
}

OVERRIDE void
alloc_headers(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    RP_HTTP1_CLIENT_CONNECTION_IMPL(self)->m_headers_or_trailers = evhtp_headers_new();
}

OVERRIDE RpStatusCode_e
on_message_begin_base(RpHttp1ConnectionImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return RpStatusCode_Ok;
}

OVERRIDE evhtp_headers_t*
headers_or_trailers(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HTTP1_CLIENT_CONNECTION_IMPL(self)->m_headers_or_trailers;
}

OVERRIDE evhtp_headers_t*
request_or_response_headers(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    return headers_or_trailers(self);
}

static inline void
headers_set_status(evhtp_headers_t* headers, evhtp_res status_code)
{
    NOISY_MSG_("(%p, %d)", headers, status_code);
    char buf[1278];
    sprintf(buf, "%d", status_code);
    evhtp_headers_add_header(headers,
        evhtp_header_new(RpHeaderValues.Status, buf, false, true));
}

OVERRIDE RpStatusCode_e
on_headers_complete_base(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);

    RpHttp1ClientConnectionImpl* me = RP_HTTP1_CLIENT_CONNECTION_IMPL(self);
    if (!me->m_pending_response && !rp_http1_connection_impl_reset_stream_called(self))
    {
//TODO...
NOISY_MSG_("No pending response and reset stream not called - premature response?");
return RpStatusCode_PrematureResponseError;
    }
    else if (me->m_pending_response)
    {
g_assert(!me->m_pending_response_done);
        RpParser* parser_ = rp_http1_connection_impl_get_parser(self);
        evhtp_res status_code = rp_parser_status_code(parser_);
        evhtp_headers_t* headers = me->m_headers_or_trailers;
        headers_set_status(headers, status_code);

        if (status_code >= EVHTP_RES_OK &&
            status_code < EVHTP_RES_MCHOICE &&
            rp_request_encoder_impl_connect_request(me->m_pending_response->m_encoder))
        {
            NOISY_MSG_("codec entering upgrade mode for CONNECT response.");
            rp_http1_connection_impl_set_handling_upgrade_(self, true);
        }

        if (status_code < EVHTP_RES_OK || status_code == EVHTP_RES_NOCONTENT)
        {
            if (evhtp_header_find(headers, RpHeaderValues.TransferEncoding))
            {
                RETURN_IF_ERROR(
                    rp_http1_connection_impl_send_protocol_error(self, "transfer_encoding_not_allowed"));
                return RpStatusCode_CodecProtocolError;
            }

            evhtp_header_t* content_length = evhtp_headers_find_header(headers, RpHeaderValues.ContentLength);
            if (content_length)
            {
                if (content_length->vlen != 1 || content_length->val[0] != '0')
                {
                    RETURN_IF_ERROR(
                        rp_http1_connection_impl_send_protocol_error(self, "content_length_not_allowed"));
                    return RpStatusCode_CodecProtocolError;
                }

                evhtp_header_rm_and_free(headers, content_length);
            }
        }

        if (rp_header_utility_is_special_1xx(headers))
        {
            rp_response_decoder_decode_1xx_headers(me->m_pending_response->m_decoder, headers);
        }
        else if (rp_http1_client_connection_impl_cannot_have_body(me) && !rp_http1_connection_impl_handling_upgrade_(self))
        {
            rp_http1_connection_impl_set_deferred_end_stream_headers(self, true);
        }
        else
        {
            rp_response_decoder_decode_headers(me->m_pending_response->m_decoder, headers, false);
        }

        //TODO...
    }

    return RpStatusCode_Ok;
}

OVERRIDE void
on_body(RpHttp1ConnectionImpl* self, evbuf_t* data)
{
    NOISY_MSG_("(%p, %p(%zu))", self, data, evbuf_length(data));
    RpHttp1ClientConnectionImpl* me = RP_HTTP1_CLIENT_CONNECTION_IMPL(self);
    if (me->m_pending_response)
    {
        rp_stream_decoder_decode_data(RP_STREAM_DECODER(me->m_pending_response->m_decoder), data, false);
    }
}

OVERRIDE RpCallbackResult_e
on_message_complete_base(RpHttp1ConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttp1ClientConnectionImpl* me = RP_HTTP1_CLIENT_CONNECTION_IMPL(self);
    if (me->m_ignore_message_complete_for_1xx)
    {
        me->m_ignore_message_complete_for_1xx = false;
        return RpCallbackResult_Success;
    }
    if (me->m_pending_response)
    {
        PendingResponse* response = me->m_pending_response;
        me->m_pending_response_done = true;

        if (rp_http1_connection_impl_get_deferred_end_stream_headers(self))
        {
            rp_response_decoder_decode_headers(response->m_decoder, me->m_headers_or_trailers, true);
            rp_http1_connection_impl_set_deferred_end_stream_headers(self, false);
        }
        else if (rp_http1_connection_impl_processing_trailers(self))
        {
            rp_response_decoder_decode_trailers(response->m_decoder, me->m_headers_or_trailers);
        }
        else
        {
            evbuf_t* buffer = rp_http1_connection_impl_empty_buffer_(self);
            rp_stream_decoder_decode_data(RP_STREAM_DECODER(response->m_decoder), buffer, true);
        }

        if (me->m_force_reset_on_premature_upstream_half_close && !me->m_encode_complete)
        {
            LOGD("Resetting stream due to premature H/1 upstream close.");
            rp_stream_callback_helper_run_reset_callbacks(RP_STREAM_CALLBACK_HELPER(response->m_encoder), RpStreamResetReason_Http1PrematureUpstreamHalfClose, "");
        }

        g_clear_pointer(&me->m_pending_response, PendingResponse_free);
        me->m_headers_or_trailers = NULL;
    }

    return rp_parser_pause(rp_http1_connection_impl_get_parser(self));
}

OVERRIDE void
on_reset_stream(RpHttp1ConnectionImpl* self, RpStreamResetReason_e reason)
{
    NOISY_MSG_("(%p, %d)", self, reason);
    RpHttp1ClientConnectionImpl* me = RP_HTTP1_CLIENT_CONNECTION_IMPL(self);
    if (me->m_pending_response && !me->m_pending_response_done)
    {
        NOISY_MSG_("Running reset callbacks for pending response.");
        rp_stream_callback_helper_run_reset_callbacks(RP_STREAM_CALLBACK_HELPER(me->m_pending_response->m_encoder), reason, "");
        me->m_pending_response_done = true;
        g_clear_pointer(&me->m_pending_response, PendingResponse_free);
    }
}

static void
http1_connection_impl_class_init(RpHttp1ConnectionImplClass* klass)
{
    NOISY_MSG_("(%p)", klass);
    klass->on_encode_complete = on_encode_complete;
    klass->alloc_headers = alloc_headers;
    klass->on_message_begin_base = on_message_begin_base;
    klass->headers_or_trailers = headers_or_trailers;
    klass->request_or_response_headers = request_or_response_headers;
    klass->on_headers_complete_base = on_headers_complete_base;
    klass->on_body = on_body;
    klass->on_message_complete_base = on_message_complete_base;
    klass->on_reset_stream = on_reset_stream;
}

static void
rp_http1_client_connection_impl_class_init(RpHttp1ClientConnectionImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    http1_connection_impl_class_init(RP_HTTP1_CONNECTION_IMPL_CLASS(klass));
}

static void
rp_http1_client_connection_impl_init(RpHttp1ClientConnectionImpl* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_pending_response_done = true;
}

RpHttp1ClientConnectionImpl*
rp_http1_client_connection_impl_new(RpNetworkConnection* connection,
                                RpHttpConnectionCallbacks* callbacks G_GNUC_UNUSED,
                                const struct RpHttp1Settings_s* settings,
                                guint16 max_response_headers_kb,
                                guint32 max_response_headers_count,
                                bool passing_through_proxy)
{
    LOGD("(%p, %p, %p, %u, %u, %u)",
        connection, callbacks, settings, max_response_headers_kb, max_response_headers_count, passing_through_proxy);
    g_return_val_if_fail(RP_IS_NETWORK_CONNECTION(connection), NULL);
    g_return_val_if_fail(settings != NULL, NULL);
    RpHttp1ClientConnectionImpl* self = g_object_new(RP_TYPE_HTTP1_CLIENT_CONNECTION_IMPL,
                                                        "connection", connection,
                                                        "codec-settings", settings,
                                                        "message-type", RpMessageType_Response,
                                                        "max-headers-kb", max_response_headers_kb,
                                                        "max-headers-count", max_response_headers_count,
                                                        NULL);
    self->m_passing_through_proxy = passing_through_proxy;
    return self;
}

static inline RpParser*
parser(RpHttp1ClientConnectionImpl* self)
{
    return rp_http1_connection_impl_get_parser(RP_HTTP1_CONNECTION_IMPL(self));
}
static inline evhtp_res
parser_status_code(RpHttp1ClientConnectionImpl* self)
{
    return rp_parser_status_code(parser(self));
}
static inline guint64
parser_content_length(RpHttp1ClientConnectionImpl* self)
{
    return rp_parser_content_length(parser(self));
}
static inline bool
parser_is_chunked(RpHttp1ClientConnectionImpl* self)
{
    return rp_parser_is_chunked(parser(self));
}

bool
rp_http1_client_connection_impl_cannot_have_body(RpHttp1ClientConnectionImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP1_CLIENT_CONNECTION_IMPL(self), true);
    evhtp_res status_code;
    PendingResponse* pending_response = self->m_pending_response;
    if (pending_response && rp_request_encoder_impl_head_request(pending_response->m_encoder))
    {
        return true;
    }
    else if ((status_code = parser_status_code(self)) == EVHTP_RES_NOCONTENT ||
        status_code == EVHTP_RES_NOTMOD ||
        (status_code >= EVHTP_RES_OK &&
            (parser_content_length(self) == 0) &&
            !parser_is_chunked(self)))
    {
        return true;
    }
    else
    {
        return false;
    }
}
