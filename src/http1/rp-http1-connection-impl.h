/*
 * rp-http1-connection-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <evhtp.h>
#include "rp-codec.h"
#include "rp-net-connection.h"
#include "rp-parser.h"
#include "rp-stream-reset-handler.h"

G_BEGIN_DECLS

/**
 * Base class for HTTP/1.1 client and server connections.
 * Handles the callbacks of http_parser with its own base routine and then
 * virtual dispatches to its subclasses.
 */
// https://github.com/envoyproxy/envoy/blob/main/source/common/http/http1/codec_impl.h#L213
#define RP_TYPE_HTTP1_CONNECTION_IMPL rp_http1_connection_impl_get_type()
G_DECLARE_DERIVABLE_TYPE(RpHttp1ConnectionImpl, rp_http1_connection_impl, RP, HTTP1_CONNECTION_IMPL, GObject)

struct _RpHttp1ConnectionImplClass {
    GObjectClass parent_class;

    evhtp_headers_t* (*headers_or_trailers)(RpHttp1ConnectionImpl*);
    evhtp_headers_t* (*request_or_response_headers)(RpHttp1ConnectionImpl*);
    void (*alloc_headers)(RpHttp1ConnectionImpl*);
    void (*alloc_trailers)(RpHttp1ConnectionImpl*);

    void (*on_encode_complete)(RpHttp1ConnectionImpl*);
    bool (*supports_http_10)(RpHttp1ConnectionImpl*);
    void (*maybe_add_sentinel_buffer_fragment)(RpHttp1ConnectionImpl*, evbuf_t*);

    RpStatusCode_e (*on_message_begin_base)(RpHttp1ConnectionImpl*);
    RpStatusCode_e (*on_url_base)(RpHttp1ConnectionImpl*, const char*, size_t);
    RpStatusCode_e (*on_status_base)(RpHttp1ConnectionImpl*);
    RpStatusCode_e (*on_headers_complete_base)(RpHttp1ConnectionImpl*);
    RpCallbackResult_e (*on_message_complete_base)(RpHttp1ConnectionImpl*);

    void (*on_body)(RpHttp1ConnectionImpl*, evbuf_t*);
    void (*on_reset_stream)(RpHttp1ConnectionImpl*, RpStreamResetReason_e);
    RpStatusCode_e (*send_protocol_error)(RpHttp1ConnectionImpl*, const char*);
    void (*on_above_high_watermark)(RpHttp1ConnectionImpl*);
    void (*on_below_low_watermark)(RpHttp1ConnectionImpl*);
};

static inline evhtp_headers_t*
rp_http1_connection_impl_headers_or_trailers(RpHttp1ConnectionImpl* self)
{
    return RP_IS_HTTP1_CONNECTION_IMPL(self) ?
        RP_HTTP1_CONNECTION_IMPL_GET_CLASS(self)->headers_or_trailers(self) : NULL;
}
static inline evhtp_headers_t*
rp_http1_connection_impl_request_or_response_headers(RpHttp1ConnectionImpl* self)
{
    return RP_IS_HTTP1_CONNECTION_IMPL(self) ?
        RP_HTTP1_CONNECTION_IMPL_GET_CLASS(self)->request_or_response_headers(self) : NULL;
}
static inline void
rp_http1_connection_impl_alloc_headers(RpHttp1ConnectionImpl* self)
{
    if (RP_IS_HTTP1_CONNECTION_IMPL(self))
    {
        RP_HTTP1_CONNECTION_IMPL_GET_CLASS(self)->alloc_headers(self);
    }
}
static inline void
rp_http1_connection_impl_alloc_trailers(RpHttp1ConnectionImpl* self)
{
    if (RP_IS_HTTP1_CONNECTION_IMPL(self))
    {
        RP_HTTP1_CONNECTION_IMPL_GET_CLASS(self)->alloc_trailers(self);
    }
}
static inline void
rp_http1_connection_impl_on_encode_complete(RpHttp1ConnectionImpl* self)
{
    if (RP_IS_HTTP1_CONNECTION_IMPL(self))
    {
        RP_HTTP1_CONNECTION_IMPL_GET_CLASS(self)->on_encode_complete(self);
    }
}
static inline bool
rp_http1_connection_impl_supports_http_10(RpHttp1ConnectionImpl* self)
{
    return RP_IS_HTTP1_CONNECTION_IMPL(self) ?
                RP_HTTP1_CONNECTION_IMPL_GET_CLASS(self)->supports_http_10(self) : false;
}
static inline void
rp_http1_connection_impl_maybe_add_sentinel_buffer_fragment(RpHttp1ConnectionImpl* self, evbuf_t* output_buffer)
{
    if (RP_IS_HTTP1_CONNECTION_IMPL(self))
    {
        RP_HTTP1_CONNECTION_IMPL_GET_CLASS(self)->maybe_add_sentinel_buffer_fragment(self, output_buffer);
    }
}
static inline void
rp_http1_connection_impl_on_body(RpHttp1ConnectionImpl* self, evbuf_t* data)
{
    if (RP_IS_HTTP1_CONNECTION_IMPL(self))
    {
        RP_HTTP1_CONNECTION_IMPL_GET_CLASS(self)->on_body(self, data);
    }
}
static inline void
rp_http1_connection_impl_on_reset_stream(RpHttp1ConnectionImpl* self, RpStreamResetReason_e reason)
{
    if (RP_IS_HTTP1_CONNECTION_IMPL(self))
    {
        RP_HTTP1_CONNECTION_IMPL_GET_CLASS(self)->on_reset_stream(self, reason);
    }
}
static inline RpStatusCode_e
rp_http1_connection_impl_send_protocol_error(RpHttp1ConnectionImpl* self, const char* details)
{
    return RP_IS_HTTP1_CONNECTION_IMPL(self) ?
        RP_HTTP1_CONNECTION_IMPL_GET_CLASS(self)->send_protocol_error(self, details) :
        RpStatusCode_Ok;
}
RpNetworkConnection* rp_http1_connection_impl_connection_(RpHttp1ConnectionImpl* self);
guint64 rp_http1_connection_impl_flush_output(RpHttp1ConnectionImpl* self, bool end_encode/*=false*/);
void rp_http1_connection_impl_buffer_add(RpHttp1ConnectionImpl* self, const char* data, size_t len);
void rp_http1_connection_impl_buffer_move(RpHttp1ConnectionImpl* self, evbuf_t* data);
bool rp_http1_connection_impl_enable_trailers(RpHttp1ConnectionImpl* self);
void rp_http1_connection_impl_on_reset_stream_base(RpHttp1ConnectionImpl* self,
                                                RpStreamResetReason_e reason);
bool rp_http1_connection_impl_reset_stream_called(RpHttp1ConnectionImpl* self);
void rp_http1_connection_impl_set_error_code(RpHttp1ConnectionImpl* self, evhtp_res code);
RpParser* rp_http1_connection_impl_get_parser(RpHttp1ConnectionImpl* self);
bool rp_http1_connection_impl_get_deferred_end_stream_headers(RpHttp1ConnectionImpl* self);
void rp_http1_connection_impl_set_deferred_end_stream_headers(RpHttp1ConnectionImpl* self, bool v);
void rp_http1_connection_impl_read_disable(RpHttp1ConnectionImpl* self, bool disable);
evbuf_t* rp_http1_connection_impl_buffer(RpHttp1ConnectionImpl* self);
bool rp_http1_connection_impl_processing_trailers(RpHttp1ConnectionImpl* self);
RpStatusCode_e rp_http1_connection_impl_on_message_begin_impl(RpHttp1ConnectionImpl* self);
evhtp_res rp_http1_connection_impl_error_code_(RpHttp1ConnectionImpl* self);
evbuf_t* rp_http1_connection_impl_empty_buffer_(RpHttp1ConnectionImpl* self);
bool rp_http1_connection_impl_handling_upgrade_(RpHttp1ConnectionImpl* self);
void rp_http1_connection_impl_set_handling_upgrade_(RpHttp1ConnectionImpl* self,
                                                    bool val);

G_END_DECLS
