/*
 * rp-codec.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-stream-info.h"
#include "rp-stream-reset-handler.h"

G_BEGIN_DECLS

#ifndef OVERRIDE
#define OVERRIDE static
#endif

typedef enum {
    RpCodecType_HTTP1,
    RpCodecType_HTTP2,
    RpCodecType_HTTP3
} RpCodecType_e;

GType RpCodecType_e_get_type(void) G_GNUC_CONST;
#define RP_TYPE_CODEC_TYPE (RpCodecType_e_get_type())


/**
 * Status codes for representing classes of Envoy errors.
 */
typedef enum {
    RpStatusCode_Ok = 0,
    RpStatusCode_CodecProtocolError = 1,
    RpStatusCode_BufferFloodError = 2,
    RpStatusCode_PrematureResponseError = 3,
    RpStatusCode_CodecClientError = 4,
    RpStatusCode_InboundFramesWithEmptyPayload = 5,
    RpStatusCode_EnvoyOverloadError = 6,
    RpStatusCode_GoAwayGracefulClose = 7,
} RpStatusCode_e;

#define RETURN_IF_ERROR(f) \
    do { \
        RpStatusCode_e status = (f); \
        if (status != RpStatusCode_Ok) return status; \
    } while (0)

/**
 * Callbacks that fire against a stream.
 */
#define RP_TYPE_STREAM_CALLBACKS rp_stream_callbacks_get_type()
G_DECLARE_INTERFACE(RpStreamCallbacks, rp_stream_callbacks, RP, STREAM_CALLBACKS, GObject)

struct _RpStreamCallbacksInterface {
    GTypeInterface parent_iface;

    void (*on_reset_stream)(RpStreamCallbacks*,
                            RpStreamResetReason_e,
                            const char*);
    void (*on_above_write_buffer_high_watermark)(RpStreamCallbacks*);
    void (*on_below_write_buffer_low_watermark)(RpStreamCallbacks*);
};

static inline void
rp_stream_callbacks_on_reset_stream(RpStreamCallbacks* self, RpStreamResetReason_e reason, const char* transport_failure_reason)
{
    if (RP_IS_STREAM_CALLBACKS(self))
    {
        RP_STREAM_CALLBACKS_GET_IFACE(self)->on_reset_stream(self,
                                                    reason,
                                                    transport_failure_reason);
    }
}
static inline void
rp_stream_callbacks_on_above_write_buffer_high_watermark(RpStreamCallbacks* self)
{
    if (RP_IS_STREAM_CALLBACKS(self))
    {
        RP_STREAM_CALLBACKS_GET_IFACE(self)->on_above_write_buffer_high_watermark(self);
    }
}
static inline void
rp_stream_callbacks_on_below_write_buffer_low_watermark(RpStreamCallbacks* self)
{
    if (RP_IS_STREAM_CALLBACKS(self))
    {
        RP_STREAM_CALLBACKS_GET_IFACE(self)->on_below_write_buffer_low_watermark(self);
    }
}


/**
 * Codec event callbacks for a given HTTP Stream.
 * This can be used to tightly couple an entity with a streams low-level events.
 */
#define RP_TYPE_CODEC_EVENT_CALLBACKS rp_codec_event_callbacks_get_type()
G_DECLARE_INTERFACE(RpCodecEventCallbacks, rp_codec_event_callbacks, RP, CODEC_EVENT_CALLBACKS, GObject)

struct _RpCodecEventCallbacksInterface {
    GTypeInterface parent_iface;

    void (*on_codec_encode_complete)(RpCodecEventCallbacks*);
    void (*on_codec_low_level_reset)(RpCodecEventCallbacks*);
};

static inline void
rp_codec_event_callbacks_on_codec_encode_complete(RpCodecEventCallbacks* self)
{
    if (RP_IS_CODEC_EVENT_CALLBACKS(self))
    {
        RP_CODEC_EVENT_CALLBACKS_GET_IFACE(self)->on_codec_encode_complete(self);
    }
}

/**
 * An HTTP stream (request, response, and push).
 */
#define RP_TYPE_STREAM rp_stream_get_type()
G_DECLARE_INTERFACE(RpStream, rp_stream, RP, STREAM, RpStreamResetHandler)

struct _RpStreamInterface {
    RpStreamResetHandlerInterface parent_iface;

    void (*add_callbacks)(RpStream*, RpStreamCallbacks*);
    void (*remove_callbacks)(RpStream*, RpStreamCallbacks*);
    RpCodecEventCallbacks* (*register_codec_event_callbacks)(RpStream*,
                                                        RpCodecEventCallbacks*);
    void (*read_disable)(RpStream*, bool);
    guint32 (*buffer_limit)(RpStream*);
    const char* (*response_details)(RpStream*);
    RpConnectionInfoProvider* (*connection_info_provider)(RpStream*);
    void (*set_flush_timeout)(RpStream*, guint32);
};

static inline void
rp_stream_add_callbacks(RpStream* self, RpStreamCallbacks* callbacks)
{
    if (RP_IS_STREAM(self))
    {
        RP_STREAM_GET_IFACE(self)->add_callbacks(self, callbacks);
    }
}
static inline void
rp_stream_remove_callbacks(RpStream* self, RpStreamCallbacks* callbacks)
{
    if (RP_IS_STREAM(self))
    {
        RP_STREAM_GET_IFACE(self)->remove_callbacks(self, callbacks);
    }
}
static inline RpCodecEventCallbacks*
rp_stream_register_codec_event_callbacks(RpStream* self, RpCodecEventCallbacks* callbacks)
{
    return RP_IS_STREAM(self) ?
        RP_STREAM_GET_IFACE(self)->register_codec_event_callbacks(self, callbacks) :
        NULL;
}
static inline void
rp_stream_read_disable(RpStream* self, bool disable)
{
    if (RP_IS_STREAM(self))
    {
        RP_STREAM_GET_IFACE(self)->read_disable(self, disable);
    }
}
static inline guint32
rp_stream_buffer_limit(RpStream* self)
{
    return RP_IS_STREAM(self) ?
        RP_STREAM_GET_IFACE(self)->buffer_limit(self) : 0;
}
static inline RpConnectionInfoProvider*
rp_stream_connection_info_provider(RpStream* self)
{
    return RP_IS_STREAM(self) ?
        RP_STREAM_GET_IFACE(self)->connection_info_provider(self) : NULL;
}
static inline void
rp_stream_set_flush_timeout(RpStream* self, guint32 timeout_ms)
{
    if (RP_IS_STREAM(self))
    {
        RP_STREAM_GET_IFACE(self)->set_flush_timeout(self, timeout_ms);
    }
}


/**
 * Stream encoder options specific to HTTP/1.
 */
#define RP_TYPE_HTTP1_STREAM_ENCODER_OPTIONS rp_http1_stream_encoder_options_get_type()
G_DECLARE_INTERFACE(RpHttp1StreamEncoderOptions, rp_http1_stream_encoder_options, RP, HTTP1_STREAM_ENCODER_OPTIONS, GObject)

struct _RpHttp1StreamEncoderOptionsInterface {
    GTypeInterface parent_iface;

    void (*disable_chunk_encoding)(RpHttp1StreamEncoderOptions*);
};

static inline void
rp_http1_stream_encoder_options_disable_chunk_encoding(RpHttp1StreamEncoderOptions* self)
{
    if (RP_IS_HTTP1_STREAM_ENCODER_OPTIONS(self))
    {
        RP_HTTP1_STREAM_ENCODER_OPTIONS_GET_IFACE(self)->disable_chunk_encoding(self);
    }
}


/**
 * Encodes an HTTP stream. This interface contains methods common to both the request and response
 * path.
 * TODO(mattklein123): Consider removing the StreamEncoder interface entirely and just duplicating
 * the methods in both the request/response path for simplicity.
 */
#define RP_TYPE_STREAM_ENCODER rp_stream_encoder_get_type()
G_DECLARE_INTERFACE(RpStreamEncoder, rp_stream_encoder, RP, STREAM_ENCODER, GObject)

struct _RpStreamEncoderInterface {
    GTypeInterface parent_iface;

    void (*encode_data)(RpStreamEncoder*, evbuf_t*, bool);
    RpStream* (*get_stream)(RpStreamEncoder*);
};

static inline void
rp_stream_encoder_encode_data(RpStreamEncoder* self, evbuf_t* data, bool end_stream)
{
    if (RP_IS_STREAM_ENCODER(self))
    {
        RP_STREAM_ENCODER_GET_IFACE(self)->encode_data(self, data, end_stream);
    }
}
static inline RpStream*
rp_stream_encoder_get_stream(RpStreamEncoder* self)
{
    return RP_IS_STREAM_ENCODER(self) ?
        RP_STREAM_ENCODER_GET_IFACE(self)->get_stream(self) : NULL;
}

/**
 * Stream encoder used for sending a request (client to server). Virtual inheritance is required
 * due to a parallel implementation split between the shared base class and the derived class.
 */
#define RP_TYPE_REQUEST_ENCODER rp_request_encoder_get_type()
G_DECLARE_INTERFACE(RpRequestEncoder, rp_request_encoder, RP, REQUEST_ENCODER, RpStreamEncoder)

struct _RpRequestEncoderInterface {
    RpStreamEncoderInterface parent_iface;

    RpStatusCode_e (*encode_headers)(RpRequestEncoder*, evhtp_headers_t*, bool);
    void (*encode_trailers)(RpRequestEncoder*, evhtp_headers_t*);
    void (*enable_tcp_tunneling)(RpRequestEncoder*);
};

static inline RpStatusCode_e
rp_request_encoder_encode_headers(RpRequestEncoder* self, evhtp_headers_t* request_headers, bool end_stream)
{
    return RP_IS_REQUEST_ENCODER(self) ?
        RP_REQUEST_ENCODER_GET_IFACE(self)->encode_headers(self, request_headers, end_stream) :
        RpStatusCode_EnvoyOverloadError;
}
static inline void
rp_request_encoder_encode_trailers(RpRequestEncoder* self, evhtp_headers_t* trailers)
{
    if (RP_IS_REQUEST_ENCODER(self)) \
        RP_REQUEST_ENCODER_GET_IFACE(self)->encode_trailers(self, trailers);
}
static inline void
rp_request_encoder_enable_tcp_tunneling(RpRequestEncoder* self)
{
    if (RP_IS_REQUEST_ENCODER(self)) \
        RP_REQUEST_ENCODER_GET_IFACE(self)->enable_tcp_tunneling(self);
}

/**
 * Stream encoder used for sending a response (server to client). Virtual inheritance is required
 * due to a parallel implementation split between the shared base class and the derived class.
 */
#define RP_TYPE_RESPONSE_ENCODER rp_response_encoder_get_type()
G_DECLARE_INTERFACE(RpResponseEncoder, rp_response_encoder, RP, RESPONSE_ENCODER, RpStreamEncoder)

struct _RpResponseEncoderInterface {
    RpStreamEncoderInterface parent_iface;

    void (*encode_1xx_headers)(RpResponseEncoder*, evhtp_headers_t*);
    void (*encode_headers)(RpResponseEncoder*, evhtp_headers_t*, bool);
    void (*encode_trailers)(RpResponseEncoder*, evhtp_headers_t*);
};

static inline void
rp_response_encoder_encode_1xx_headers(RpResponseEncoder* self, evhtp_headers_t* response_headers)
{
    if (RP_IS_RESPONSE_ENCODER(self))
    {
        RP_RESPONSE_ENCODER_GET_IFACE(self)->encode_1xx_headers(self, response_headers);
    }
}
static inline void
rp_response_encoder_encode_headers(RpResponseEncoder* self, evhtp_headers_t* response_headers, bool end_stream)
{
    if (RP_IS_RESPONSE_ENCODER(self))
    {
        RP_RESPONSE_ENCODER_GET_IFACE(self)->encode_headers(self,
                                                            response_headers,
                                                            end_stream);
    }
}
static inline void
rp_response_encoder_encode_trailers(RpResponseEncoder* self, evhtp_headers_t* trailers)
{
    if (RP_IS_RESPONSE_ENCODER(self))
    {
        RP_RESPONSE_ENCODER_GET_IFACE(self)->encode_trailers(self, trailers);
    }
}

/**
 * Decodes an HTTP stream. These are callbacks fired into a sink. This interface contains methods
 * common to both the request and response path.
 * TODO(mattklein123): Consider removing the StreamDecoder interface entirely and just duplicating
 * the methods in both the request/response path for simplicity.
 */
#define RP_TYPE_STREAM_DECODER rp_stream_decoder_get_type()
G_DECLARE_INTERFACE(RpStreamDecoder, rp_stream_decoder, RP, STREAM_DECODER, GObject)

typedef void (*modify_headers_cb)(evhtp_headers_t*, void*);

struct _RpStreamDecoderInterface {
    GTypeInterface parent_iface;

    void (*decode_data)(RpStreamDecoder*, evbuf_t*, bool);
};

static inline void
rp_stream_decoder_decode_data(RpStreamDecoder* self, evbuf_t* data, bool end_stream)
{
    if (RP_IS_STREAM_DECODER(self))
    {
        RP_STREAM_DECODER_GET_IFACE(self)->decode_data(self, data, end_stream);
    }
}

/**
 * Stream decoder used for receiving a request (client to server). Virtual inheritance is required
 * due to a parallel implementation split between the shared base class and the derived class.
 */
#define RP_TYPE_REQUEST_DECODER rp_request_decoder_get_type()
G_DECLARE_INTERFACE(RpRequestDecoder, rp_request_decoder, RP, REQUEST_DECODER, RpStreamDecoder)

struct _RpRequestDecoderInterface {
    RpStreamDecoderInterface parent_iface;

    void (*decode_headers)(RpRequestDecoder*, evhtp_headers_t*, bool);
    void (*decode_trailers)(RpRequestDecoder*, evhtp_headers_t*);
    void (*send_local_reply)(RpRequestDecoder*,
                                evhtp_res,
                                evbuf_t*,
                                modify_headers_cb,
                                const char*,
                                void*);
};

static inline void
rp_request_decoder_decode_headers(RpRequestDecoder* self, evhtp_headers_t* request_headers, bool end_stream)
{
    if (RP_IS_REQUEST_DECODER(self))
    {
        RP_REQUEST_DECODER_GET_IFACE(self)->decode_headers(self,
                                                            request_headers,
                                                            end_stream);
    }
}
static inline void
rp_request_decoder_decode_trailers(RpRequestDecoder* self, evhtp_headers_t* trailers)
{
    if (RP_IS_REQUEST_DECODER(self))
    {
        RP_REQUEST_DECODER_GET_IFACE(self)->decode_trailers(self, trailers);
    }
}
static inline void
rp_request_decoder_send_local_reply(RpRequestDecoder* self, evhtp_res code, evbuf_t* body, modify_headers_cb modify_headers, const char* details, void* arg)
{
    if (RP_IS_REQUEST_DECODER(self))
    {
        RP_REQUEST_DECODER_GET_IFACE(self)->send_local_reply(self,
                                                                code,
                                                                body,
                                                                modify_headers,
                                                                details,
                                                                arg);
    }
}

/**
 * Stream decoder used for receiving a response (server to client). Virtual inheritance is required
 * due to a parallel implementation split between the shared base class and the derived class.
 */
#define RP_TYPE_RESPONSE_DECODER rp_response_decoder_get_type()
G_DECLARE_INTERFACE(RpResponseDecoder, rp_response_decoder, RP, RESPONSE_DECODER, RpStreamDecoder)

struct _RpResponseDecoderInterface {
    RpStreamDecoderInterface parent_iface;

    void (*decode_1xx_headers)(RpResponseDecoder*, evhtp_headers_t*);
    void (*decode_headers)(RpResponseDecoder*, evhtp_headers_t*, bool);
    void (*decode_trailers)(RpResponseDecoder*, evhtp_headers_t*);
};

static inline void
rp_response_decoder_decode_1xx_headers(RpResponseDecoder* self, evhtp_headers_t* response_headers)
{
    if (RP_IS_RESPONSE_DECODER(self))
    {
        RP_RESPONSE_DECODER_GET_IFACE(self)->decode_1xx_headers(self,
                                                                response_headers);
    }
}
static inline void
rp_response_decoder_decode_headers(RpResponseDecoder* self, evhtp_headers_t* response_headers, bool end_stream)
{
    if (RP_IS_RESPONSE_DECODER(self))
    {
        RP_RESPONSE_DECODER_GET_IFACE(self)->decode_headers(self,
                                                            response_headers,
                                                            end_stream);
    }
}
static inline void
rp_response_decoder_decode_trailers(RpResponseDecoder* self, evhtp_headers_t* trailers)
{
    if (RP_IS_RESPONSE_DECODER(self))
    {
        RP_RESPONSE_DECODER_GET_IFACE(self)->decode_trailers(self, trailers);
    }
}

/**
 * A class for sharing what HTTP/2 SETTINGS were received from the peer.
 */
#define RP_TYPE_RECEIVED_SETTINGS rp_received_settings_get_type()
G_DECLARE_INTERFACE(RpReceivedSettings, rp_received_settings, RP, RECEIVED_SETTINGS, GObject)

struct _RpReceivedSettingsInterface {
    GTypeInterface parent_iface;

    guint32 (*max_concurrent_streams)(RpReceivedSettings*);
};

/**
 * Connection level callbacks.
 */
#define RP_TYPE_HTTP_CONNECTION_CALLBACKS rp_http_connection_callbacks_get_type()
G_DECLARE_INTERFACE(RpHttpConnectionCallbacks, rp_http_connection_callbacks, RP, HTTP_CONNECTION_CALLBACKS, GObject)

typedef enum {
    RpGoAwayErrorCode_NoError,
    RpGoAwayErrorCode_Other,
} RpGoAwayErrorCode_e;

struct _RpHttpConnectionCallbacksInterface {
    GTypeInterface parent_iface;

    void (*on_go_away)(RpHttpConnectionCallbacks*, RpGoAwayErrorCode_e);
    void (*on_settings)(RpHttpConnectionCallbacks*, RpReceivedSettings*);
    void (*on_max_streams_changed)(RpHttpConnectionCallbacks*, guint32);
};

static inline void
rp_http_connection_callbacks_on_go_away(RpHttpConnectionCallbacks* self, RpGoAwayErrorCode_e error_code)
{
    if (RP_IS_HTTP_CONNECTION_CALLBACKS(self))
    {
        RP_HTTP_CONNECTION_CALLBACKS_GET_IFACE(self)->on_go_away(self, error_code);
    }
}
static inline void
rp_http_connection_callbacks_on_settings(RpHttpConnectionCallbacks* self, RpReceivedSettings* settings)
{
    if (RP_IS_HTTP_CONNECTION_CALLBACKS(self))
    {
        RP_HTTP_CONNECTION_CALLBACKS_GET_IFACE(self)->on_settings(self, settings);
    }
}
static inline void
rp_http_connection_callbacks_on_max_streams_changed(RpHttpConnectionCallbacks* self, guint32 num_streams)
{
    if (RP_IS_HTTP_CONNECTION_CALLBACKS(self))
    {
        RP_HTTP_CONNECTION_CALLBACKS_GET_IFACE(self)->on_max_streams_changed(self, num_streams);
    }
}

typedef enum {
    RpHeaderKeyFormat_Default,
    RpHeaderKeyFormat_ProperCase,
    RpHeaderKeyFormat_StatefulFormatter
} RpHeaderKeyFormat_e;

/**
 * HTTP/1.* Codec settings
 */
struct RpHttp1Settings_s {
    bool m_allow_absolute_url;//false
    bool m_accept_http_10;//false
    char* m_default_host_for_http_10;
    bool m_enable_trailers;//false
    bool m_allow_chunked_length;//false
    RpHeaderKeyFormat_e m_header_key_format;//Default
    bool m_stream_error_on_invalid_http_message;//false
    bool m_validate_sheme;//false
    bool m_send_fully_qualified_url;//false
    bool m_use_balsa_parser;//false
    bool m_allow_custom_methods;//false
};
extern struct RpHttp1Settings_s RpHttp1Settings;

/**
 * A connection (client or server) that owns multiple streams.
 */
#define RP_TYPE_HTTP_CONNECTION rp_http_connection_get_type()
G_DECLARE_INTERFACE(RpHttpConnection, rp_http_connection, RP, HTTP_CONNECTION, GObject)

struct _RpHttpConnectionInterface {
    GTypeInterface parent_iface;

    RpStatusCode_e (*dispatch)(RpHttpConnection*, evbuf_t*);
    void (*go_away)(RpHttpConnection*);
    enum evhtp_proto (*protocol)(RpHttpConnection*);
    void (*shutdown_notice)(RpHttpConnection*);
    bool (*wants_to_write)(RpHttpConnection*);
    void (*on_underlying_connection_above_write_buffer_high_watermark)(RpHttpConnection*);
    void (*on_underlying_connection_below_write_buffer_lo_watermark)(RpHttpConnection*);
    bool (*should_keep_alive)(RpHttpConnection*);
};

static inline void
rp_http_connection_go_away(RpHttpConnection* self)
{
    if (RP_IS_HTTP_CONNECTION(self))
    {
        RP_HTTP_CONNECTION_GET_IFACE(self)->go_away(self);
    }
}
static inline evhtp_proto
rp_http_connection_protocol(RpHttpConnection* self)
{
    return RP_IS_HTTP_CONNECTION(self) ?
            RP_HTTP_CONNECTION_GET_IFACE(self)->protocol(self) : EVHTP_PROTO_INVALID;
}
static inline bool
rp_http_connection_should_keep_alive(RpHttpConnection* self)
{
    return RP_IS_HTTP_CONNECTION(self) ?
            RP_HTTP_CONNECTION_GET_IFACE(self)->should_keep_alive(self) : false;
}
static inline RpStatusCode_e
rp_http_connection_dispatch(RpHttpConnection* self, evbuf_t* data)
{
    return RP_IS_HTTP_CONNECTION(self) ?
            RP_HTTP_CONNECTION_GET_IFACE(self)->dispatch(self, data) :
            RpStatusCode_CodecClientError;
}
static inline bool
rp_http_connection_wants_to_write(RpHttpConnection* self)
{
    return RP_IS_HTTP_CONNECTION(self) ?
        RP_HTTP_CONNECTION_GET_IFACE(self)->wants_to_write(self) : false;
}


/**
 * Callbacks for downstream connection watermark limits.
 */
#define RP_TYPE_DOWNSTREAM_WATERMARK_CALLBACKS rp_downstream_watermark_callbacks_get_type()
G_DECLARE_INTERFACE(RpDownstreamWatermarkCallbacks, rp_downstream_watermark_callbacks, RP, DOWNSTREAM_WATERMARK_CALLBACKS, GObject)

struct _RpDownstreamWatermarkCallbacksInterface {
    GTypeInterface parent_iface;

    void (*on_above_write_buffer_high_watermark)(RpDownstreamWatermarkCallbacks*);
    void (*on_below_write_buffer_low_watermark)(RpDownstreamWatermarkCallbacks*);
};

static inline void
rp_downstream_watermark_callbacks_on_above_write_buffer_high_watermark(RpDownstreamWatermarkCallbacks* self)
{
    if (RP_IS_DOWNSTREAM_WATERMARK_CALLBACKS(self)) \
        RP_DOWNSTREAM_WATERMARK_CALLBACKS_GET_IFACE(self)->on_above_write_buffer_high_watermark(self);
}
static inline void
rp_downstream_watermark_callbacks_on_below_write_buffer_low_watermark(RpDownstreamWatermarkCallbacks* self)
{
    if (RP_IS_DOWNSTREAM_WATERMARK_CALLBACKS(self)) \
        RP_DOWNSTREAM_WATERMARK_CALLBACKS_GET_IFACE(self)->on_below_write_buffer_low_watermark(self);
}


/**
 * Callbacks for server connections.
 */
#define RP_TYPE_HTTP_SERVER_CONNECTION_CALLBACKS rp_http_server_connection_callbacks_get_type()
G_DECLARE_INTERFACE(RpHttpServerConnectionCallbacks, rp_http_server_connection_callbacks, RP, HTTP_SERVER_CONNECTION_CALLBACKS, RpHttpConnectionCallbacks)

struct _RpHttpServerConnectionCallbacksInterface {
    RpHttpConnectionCallbacksInterface parent_iface;

    /**
     * Invoked when a new request stream is initiated by the remote.
     * @param response_encoder supplies the encoder to use for creating the response. The request and
     *                         response are backed by the same Stream object.
     * @param is_internally_created indicates if this stream was originated by a
     *   client, or was created by Envoy, by example as part of an internal redirect.
     * @return RequestDecoder& supplies the decoder callbacks to fire into for stream decoding
     *   events.
     */
    RpRequestDecoder* (*new_stream)(RpHttpServerConnectionCallbacks*,
                                    RpResponseEncoder*,
                                    bool);
};

static inline RpRequestDecoder*
rp_http_server_connection_callbacks_new_stream(RpHttpServerConnectionCallbacks* self, RpResponseEncoder* response_encoder, bool is_internally_created)
{
    return RP_IS_HTTP_SERVER_CONNECTION_CALLBACKS(self) ?
        RP_HTTP_SERVER_CONNECTION_CALLBACKS_GET_IFACE(self)->new_stream(self,
                                                        response_encoder,
                                                        is_internally_created) :
        NULL;
}

/**
 * A server side HTTP connection.
 */
#define RP_TYPE_HTTP_SERVER_CONNECTION rp_http_server_connection_get_type()
G_DECLARE_INTERFACE(RpHttpServerConnection, rp_http_server_connection, RP, HTTP_SERVER_CONNECTION, RpHttpConnection)

struct _RpHttpServerConnectionInterface {
    RpHttpConnectionInterface parent_iface;

};

/**
 * A client side HTTP connection.
 */
#define RP_TYPE_HTTP_CLIENT_CONNECTION rp_http_client_connection_get_type()
G_DECLARE_INTERFACE(RpHttpClientConnection, rp_http_client_connection, RP, HTTP_CLIENT_CONNECTION, RpHttpConnection)

struct _RpHttpClientConnectionInterface {
    RpHttpConnectionInterface parent_iface;

    /**
     * Create a new outgoing request stream.
     * @param response_decoder supplies the decoder callbacks to fire response events into.
     * @return RequestEncoder& supplies the encoder to write the request into.
     */
    RpRequestEncoder* (*new_stream)(RpHttpClientConnection*, RpResponseDecoder*);
};

static inline RpRequestEncoder*
rp_http_client_connection_new_stream(RpHttpClientConnection* self, RpResponseDecoder* response_decoder)
{
    return RP_IS_HTTP_CLIENT_CONNECTION(self) ?
        RP_HTTP_CLIENT_CONNECTION_GET_IFACE(self)->new_stream(self, response_decoder) :
        NULL;
}

G_END_DECLS
