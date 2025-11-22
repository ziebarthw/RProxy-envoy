/*
 * rp-request-encoder-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_request_encoder_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_request_encoder_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-headers.h"
#include "rp-header-utility.h"
#include "rp-http-utility.h"
#include "rp-request-encoder-impl.h"

struct _RpRequestEncoderImpl {
    RpStreamEncoderImpl parent_instance;

    RpHttp1ConnectionImpl* m_connection;

    bool m_upgrade_request : 1;
    bool m_head_request : 1;
};

static void request_encoder_iface_init(RpRequestEncoderInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpRequestEncoderImpl, rp_request_encoder_impl, RP_TYPE_STREAM_ENCODER_IMPL,
    G_IMPLEMENT_INTERFACE(RP_TYPE_REQUEST_ENCODER, request_encoder_iface_init)
)

static RpStatusCode_e
encode_headers_i(RpRequestEncoder* self, evhtp_headers_t* request_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, request_headers, end_stream);

    RpRequestEncoderImpl* me = RP_REQUEST_ENCODER_IMPL(self);
    const char* method = evhtp_header_find(request_headers, RpHeaderValues.Method);
    const char* path = evhtp_header_find(request_headers, RpHeaderValues.Path);
    const char* host = evhtp_header_find(request_headers, RpHeaderValues.Host);
    bool is_connect = rp_header_utility_is_connect(request_headers);

    if (g_ascii_strcasecmp(method, RpHeaderValues.MethodValues.Head) == 0)
    {
        me->m_head_request = true;
    }
    else if (is_connect)
    {
        rp_http1_stream_encoder_options_disable_chunk_encoding(RP_HTTP1_STREAM_ENCODER_OPTIONS(self));
        rp_network_connection_enable_half_close(rp_http1_connection_impl_connection_(me->m_connection), true);
        rp_stream_encoder_impl_set_connection_request(RP_STREAM_ENCODER_IMPL(self), true);
    }
    if (http_utility_is_upgrade(request_headers))
    {
        me->m_upgrade_request = true;
        rp_http1_stream_encoder_options_disable_chunk_encoding(RP_HTTP1_STREAM_ENCODER_OPTIONS(self));
    }

    //TODO:...

    evbuf_t* output = rp_http1_connection_impl_buffer(me->m_connection);
    evbuffer_add(output, method, strlen(method));
    evbuffer_add(output, " ", 1);
    is_connect ? evbuffer_add(output, host, strlen(host)) : evbuffer_add(output, path, strlen(path));
    evbuffer_add(output, " HTTP/1.1\r\n", 11);

NOISY_MSG_("calling rp_stream_encoder_impl_encode_headers_base(%p, %p, 0, %u, 1)", self, request_headers, end_stream);
    rp_stream_encoder_impl_encode_headers_base(RP_STREAM_ENCODER_IMPL(self),
                                                request_headers,
                                                0,
                                                end_stream,
                                                rp_header_utility_request_should_have_no_body(request_headers));
    return RpStatusCode_Ok;
}

static void
encode_trailers_i(RpRequestEncoder* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    rp_stream_encoder_impl_encode_trailers_base(RP_STREAM_ENCODER_IMPL(self), trailers);
}

static void
enable_tcp_tunneling_i(RpRequestEncoder* self)
{
    NOISY_MSG_("(%p)", self);
    rp_stream_encoder_impl_set_is_tcp_tunneling(RP_STREAM_ENCODER_IMPL(self), true);
}

static void
request_encoder_iface_init(RpRequestEncoderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->encode_headers = encode_headers_i;
    iface->encode_trailers = encode_trailers_i;
    iface->enable_tcp_tunneling = enable_tcp_tunneling_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_request_encoder_impl_parent_class)->dispose(obj);
}

static void
rp_request_encoder_impl_class_init(RpRequestEncoderImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_request_encoder_impl_init(RpRequestEncoderImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpRequestEncoderImpl*
rp_request_encoder_impl_new(RpHttp1ConnectionImpl* connection)
{
    LOGD("(%p)", connection);
    g_return_val_if_fail(RP_IS_HTTP1_CONNECTION_IMPL(connection), NULL);
    RpRequestEncoderImpl* self = g_object_new(RP_TYPE_REQUEST_ENCODER_IMPL,
                                                "connection", connection,
                                                NULL);
    self->m_connection = connection;
    return self;
}

bool
rp_request_encoder_impl_head_request(RpRequestEncoderImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_REQUEST_ENCODER_IMPL(self), false);
    return self->m_head_request;
}

bool
rp_request_encoder_impl_connect_request(RpRequestEncoderImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_REQUEST_ENCODER_IMPL(self), false);
    return rp_stream_encoder_impl_connect_request_(RP_STREAM_ENCODER_IMPL(self));
}
