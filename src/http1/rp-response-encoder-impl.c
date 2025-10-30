/*
 * rp-response-encoder-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#include "rp-headers.h"
#include "rp-http-utility.h"
#include "rp-response-encoder-impl.h"

#if (defined(rp_response_encoder_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_response_encoder_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

struct _RpResponseEncoderImpl {
    RpStreamEncoderImpl parent_instance;

    RpHttp1ConnectionImpl* m_connection;

    bool m_started_response : 1;
    bool m_stream_error_on_invalid_http_message : 1;
};

static void stream_reset_handler_iface_init(RpStreamResetHandlerInterface* iface);
static void response_encoder_iface_init(RpResponseEncoderInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpResponseEncoderImpl, rp_response_encoder_impl, RP_TYPE_STREAM_ENCODER_IMPL,
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_RESET_HANDLER, stream_reset_handler_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_RESPONSE_ENCODER, response_encoder_iface_init)
)

static void
encode_1xx_headers_i(RpResponseEncoder* self, evhtp_headers_t* response_headers)
{
    NOISY_MSG_("(%p, %p)", self, response_headers);
    rp_response_encoder_encode_headers(self, response_headers, false);
    RP_RESPONSE_ENCODER_IMPL(self)->m_started_response = true;
}

static const char* RESPONSE_PREFIX = "HTTP/1.1 ";
static const char* HTTP_10_RESPONSE_PREFIX = "HTTP/1.0 ";

static void
encode_headers_i(RpResponseEncoder* self, evhtp_headers_t* response_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, response_headers, end_stream);

    RpResponseEncoderImpl* me = RP_RESPONSE_ENCODER_IMPL(self);

    me->m_started_response = true;
    evhtp_res numeric_status = http_utility_get_response_status(response_headers);

    const char* response_prefix;
    if (rp_http_connection_protocol(RP_HTTP_CONNECTION(me->m_connection)) == EVHTP_PROTO_10 &&
        rp_http1_connection_impl_supports_http_10(me->m_connection))
    {
        response_prefix = HTTP_10_RESPONSE_PREFIX;
    }
    else
    {
        response_prefix = RESPONSE_PREFIX;
    }

    char status_code[256];
    int status_code_len = snprintf(status_code, sizeof(status_code), "%d", numeric_status);
    const char* reason_phrase = evhtp_get_status_code_str(numeric_status);

    evbuf_t* output = rp_http1_connection_impl_buffer(me->m_connection);
    evbuffer_add(output, response_prefix, strlen(response_prefix));
    evbuffer_add(output, status_code, status_code_len);
    evbuffer_add(output, " ", 1);
    evbuffer_add(output, reason_phrase, strlen(reason_phrase));
    evbuffer_add(output, "\r\n", 2);

    if (numeric_status >= 300)
    {
        // Don't do special CONNECT logic if the CONNECT was rejected.
        rp_stream_encoder_impl_set_is_response_to_connect_request(RP_STREAM_ENCODER_IMPL(self), false);
    }

    rp_stream_encoder_impl_encode_headers_base(RP_STREAM_ENCODER_IMPL(self),
                                                response_headers,
                                                numeric_status,
                                                end_stream,
                                                false);
}

static void
encode_trailers_i(RpResponseEncoder* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    rp_stream_encoder_impl_encode_trailers_base(RP_STREAM_ENCODER_IMPL(self), trailers);
}

static void
response_encoder_iface_init(RpResponseEncoderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->encode_1xx_headers = encode_1xx_headers_i;
    iface->encode_headers = encode_headers_i;
    iface->encode_trailers = encode_trailers_i;
}

OVERRIDE void
dispose(GObject* object)
{
    NOISY_MSG_("(%p)", object);
    G_OBJECT_CLASS(rp_response_encoder_impl_parent_class)->dispose(object);
}

static void
reset_stream_i(RpStreamResetHandler* self, RpStreamResetReason_e reason)
{
    NOISY_MSG_("(%p, %d)", self, reason);
    //TODO...
    RpStreamResetHandlerInterface* iface = g_type_interface_peek_parent(RP_STREAM_RESET_HANDLER_GET_IFACE(self));
    if (iface && iface->reset_stream)
    {
        NOISY_MSG_("calling parent reset_stream()");
        iface->reset_stream(self, reason);
    }
}

static inline void
stream_reset_handler_iface_init(RpStreamResetHandlerInterface* iface)
{
    LOGD("(%p)", iface);
    iface->reset_stream = reset_stream_i;
}

static void
rp_response_encoder_impl_class_init(RpResponseEncoderImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_response_encoder_impl_init(RpResponseEncoderImpl* self G_GNUC_UNUSED)
{
    LOGD("(%p)", self);
}

RpResponseEncoderImpl*
rp_response_encoder_impl_new(RpHttp1ConnectionImpl* connection, bool stream_error_on_invalid_http_message)
{
    LOGD("(%p, %u)", connection, stream_error_on_invalid_http_message);
    g_return_val_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(connection), NULL);
    RpResponseEncoderImpl* self = g_object_new(RP_TYPE_RESPONSE_ENCODER_IMPL,
                                                "connection", connection,
                                                NULL);
    self->m_stream_error_on_invalid_http_message = stream_error_on_invalid_http_message;
    self->m_connection = connection;
    return self;
}

bool
rp_response_encoder_impl_started_reponse(RpResponseEncoderImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_RESPONSE_ENCODER_IMPL(self), false);
    return self->m_started_response;
}
