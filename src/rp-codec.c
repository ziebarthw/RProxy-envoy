/*
 * rp-codec.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#include "rp-codec.h"

GType
RpCodecType_e_get_type(void)
{
    static gsize type_id = 0;

    if (g_once_init_enter(&type_id))
    {
        static const GEnumValue CodecType_values[] = {
            {RpCodecType_HTTP1, "HTTP1 Codec Type", "HTTP1"},
            {RpCodecType_HTTP2, "HTTP2 Codec Type", "HTTP2"},
            {RpCodecType_HTTP3, "HTTP3 Codec Type", "HTTP3"},
            {0, NULL, NULL},
        };
        GType RpCodecType_e_type = g_enum_register_static("RpCodecType", CodecType_values);
        g_once_init_leave(&type_id, RpCodecType_e_type);
    }
    return (GType)type_id;
}

struct RpHttp1Settings_s RpHttp1Settings = {
    .m_allow_absolute_url = true,/*TODO...Should be conditional based on whether or not an rproxy server is an outgoing proxy*/
    .m_accept_http_10 = false,
    .m_default_host_for_http_10 = NULL,
    .m_enable_trailers = false,
    .m_allow_chunked_length = false,
    .m_header_key_format = RpHeaderKeyFormat_Default,
    .m_stream_error_on_invalid_http_message = false,
    .m_validate_sheme = false,
    .m_send_fully_qualified_url = false,
    .m_use_balsa_parser = false,
    .m_allow_custom_methods = false
};

G_DEFINE_INTERFACE(RpStreamEncoder, rp_stream_encoder, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpRequestEncoder, rp_request_encoder, RP_TYPE_STREAM_ENCODER)
G_DEFINE_INTERFACE(RpResponseEncoder, rp_response_encoder, RP_TYPE_STREAM_ENCODER)
G_DEFINE_INTERFACE(RpStreamDecoder, rp_stream_decoder, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpRequestDecoder, rp_request_decoder, RP_TYPE_STREAM_DECODER)
G_DEFINE_INTERFACE(RpResponseDecoder, rp_response_decoder, RP_TYPE_STREAM_DECODER)
G_DEFINE_INTERFACE(RpStreamCallbacks, rp_stream_callbacks, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpCodecEventCallbacks, rp_codec_event_callbacks, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpStream, rp_stream, RP_TYPE_STREAM_RESET_HANDLER)
G_DEFINE_INTERFACE(RpReceivedSettings, rp_received_settings, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpHttpConnection, rp_http_connection, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpHttpConnectionCallbacks, rp_http_connection_callbacks, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpHttpServerConnectionCallbacks, rp_http_server_connection_callbacks, RP_TYPE_HTTP_CONNECTION_CALLBACKS)
G_DEFINE_INTERFACE(RpHttpServerConnection, rp_http_server_connection, RP_TYPE_HTTP_CONNECTION)
G_DEFINE_INTERFACE(RpHttpClientConnection, rp_http_client_connection, RP_TYPE_HTTP_CONNECTION)
G_DEFINE_INTERFACE(RpHttp1StreamEncoderOptions, rp_http1_stream_encoder_options, G_TYPE_OBJECT)

G_DEFINE_INTERFACE(RpDownstreamWatermarkCallbacks, rp_downstream_watermark_callbacks, G_TYPE_OBJECT)
static void
rp_downstream_watermark_callbacks_default_init(RpDownstreamWatermarkCallbacksInterface* iface G_GNUC_UNUSED)
{}

static void
stream_encoder_encode_data_(RpStreamEncoder* self, evbuf_t* data, bool end_stream)
{
}
static void
rp_stream_encoder_default_init(RpStreamEncoderInterface* iface)
{
    iface->encode_data = stream_encoder_encode_data_;
}

static RpStatusCode_e
request_encoder_encode_headers_(RpRequestEncoder* self, evhtp_headers_t* request_headers, bool end_stream)
{
    return RpStatusCode_Ok;
}
static void
request_encoder_encode_trailers_(RpRequestEncoder* self, evhtp_headers_t* trailers)
{
}
static void
rp_request_encoder_default_init(RpRequestEncoderInterface* iface)
{
    iface->encode_headers = request_encoder_encode_headers_;
    iface->encode_trailers = request_encoder_encode_trailers_;
}

static void
response_encoder_encode_1xx_headers_(RpResponseEncoder* self, evhtp_headers_t* response_headers)
{
}
static void
response_encoder_encode_headers_(RpResponseEncoder* self, evhtp_headers_t* response_headers, bool end_stream)
{
}
static void
response_encoder_encode_trailers_(RpResponseEncoder* self, evhtp_headers_t* trailers)
{
}
static void
rp_response_encoder_default_init(RpResponseEncoderInterface* iface)
{
    iface->encode_1xx_headers = response_encoder_encode_1xx_headers_;
    iface->encode_headers = response_encoder_encode_headers_;
    iface->encode_trailers = response_encoder_encode_trailers_;
}

static void
stream_decoder_decode_data_(RpStreamDecoder* self, evbuf_t* data, bool end_stream)
{
}
static void
rp_stream_decoder_default_init(RpStreamDecoderInterface* iface)
{
    iface->decode_data = stream_decoder_decode_data_;
}

static void
request_decoder_decode_headers_(RpRequestDecoder* self, evhtp_headers_t* request_headers, bool end_stream)
{
}
static void
request_decoder_decode_trailers_(RpRequestDecoder* self, evhtp_headers_t* trailers)
{
}
static void
request_decoder_send_local_reply_(RpRequestDecoder* self, evhtp_res code, evbuf_t* body, modify_headers_cb modify_headers, const char* details, void* arg)
{
    LOGD("(%p, %d, %p(%zu), %p, %p, %p)",
        self, code, body, evbuffer_get_length(body), modify_headers, details, arg);
}
static void
rp_request_decoder_default_init(RpRequestDecoderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->decode_headers = request_decoder_decode_headers_;
    iface->decode_trailers = request_decoder_decode_trailers_;
    iface->send_local_reply = request_decoder_send_local_reply_;
}

static void
response_decoder_decode_1xx_headers_(RpResponseDecoder* self, evhtp_headers_t* response_headers)
{
}
static void
response_decoder_decode_headers_(RpResponseDecoder* self, evhtp_headers_t* response_headers, bool end_stream)
{
}
static void
response_decoder_decode_trailers_(RpResponseDecoder* self, evhtp_headers_t* trailers)
{
}
static void
rp_response_decoder_default_init(RpResponseDecoderInterface* iface)
{
    iface->decode_1xx_headers = response_decoder_decode_1xx_headers_;
    iface->decode_headers = response_decoder_decode_headers_;
    iface->decode_trailers = response_decoder_decode_trailers_;
}

static void
stream_callbacks_on_reset_stream_(RpStreamCallbacks* self, RpStreamResetReason_e reason, const char* transport_failure_reason)
{
}
static void
rp_stream_callbacks_default_init(RpStreamCallbacksInterface* iface)
{
    iface->on_reset_stream = stream_callbacks_on_reset_stream_;
}

static void
codec_event_callbacks_on_codec_encode_complete_(RpCodecEventCallbacks* self)
{
}
static void
rp_codec_event_callbacks_default_init(RpCodecEventCallbacksInterface* iface)
{
    iface->on_codec_encode_complete = codec_event_callbacks_on_codec_encode_complete_;
}

static void
stream_add_callbacks_(RpStream* self, RpStreamCallbacks* callbacks)
{
}
static void
stream_remove_callbacks_(RpStream* self, RpStreamCallbacks* callbacks)
{
}
static RpCodecEventCallbacks*
stream_register_codec_event_callbacks_(RpStream* self, RpCodecEventCallbacks* codec_callbacks)
{
    return NULL;
}
static void
stream_read_disable_(RpStream* self, bool disable)
{
}
static guint32
stream_buffer_limit_(RpStream* self)
{
    return 0;
}
static const char*
stream_response_details_(RpStream* self)
{
    return "";
}
static void
stream_set_flush_timeout_(RpStream* self, guint32 timeout_ms)
{
}
static void
rp_stream_default_init(RpStreamInterface* iface)
{
    iface->add_callbacks = stream_add_callbacks_;
    iface->remove_callbacks = stream_remove_callbacks_;
    iface->register_codec_event_callbacks = stream_register_codec_event_callbacks_;
    iface->read_disable = stream_read_disable_;
    iface->buffer_limit = stream_buffer_limit_;
    iface->response_details = stream_response_details_;
    iface->set_flush_timeout = stream_set_flush_timeout_;
}

static guint32
received_settings_max_concurrent_streams_(RpReceivedSettings* self)
{
    return 0;
}
static void
rp_received_settings_default_init(RpReceivedSettingsInterface* iface)
{
    iface->max_concurrent_streams = received_settings_max_concurrent_streams_;
}

static void
on_go_away_(RpHttpConnectionCallbacks* self, RpGoAwayErrorCode_e error_code)
{
}
static void
on_settings_(RpHttpConnectionCallbacks* self, RpReceivedSettings* settings)
{
}
static void
on_max_streams_changed_(RpHttpConnectionCallbacks* self, guint32 num_streams)
{
}
static void
rp_http_connection_callbacks_default_init(RpHttpConnectionCallbacksInterface* iface)
{
    iface->on_go_away = on_go_away_;
    iface->on_settings = on_settings_;
    iface->on_max_streams_changed = on_max_streams_changed_;
}

static RpStatusCode_e
dispatch_(RpHttpConnection* self, evbuf_t* data)
{
    return RpStatusCode_Ok;
}
static void
go_away_(RpHttpConnection* self)
{
}
static enum evhtp_proto
protocol_(RpHttpConnection* self)
{
    return EVHTP_PROTO_INVALID;
}
static void
shutdown_notice_(RpHttpConnection* self)
{
}
static bool
wants_to_write_(RpHttpConnection* self)
{
    return false;
}
static void
rp_http_connection_default_init(RpHttpConnectionInterface* iface)
{
    iface->dispatch = dispatch_;
    iface->go_away = go_away_;
    iface->protocol = protocol_;
    iface->shutdown_notice = shutdown_notice_;
    iface->wants_to_write = wants_to_write_;
}

static RpRequestDecoder*
http_server_connection_new_stream_(RpHttpServerConnectionCallbacks* self, RpResponseEncoder* response_encoder, bool is_internally_created)
{
    return NULL;
}
static void
rp_http_server_connection_callbacks_default_init(RpHttpServerConnectionCallbacksInterface* iface)
{
    iface->new_stream = http_server_connection_new_stream_;
}

static void
rp_http_server_connection_default_init(RpHttpServerConnectionInterface* iface)
{}

static RpRequestEncoder*
http_client_connection_new_stream_(RpHttpClientConnection* self, RpResponseDecoder* reponse_decoder)
{
    return NULL;
}
static void
rp_http_client_connection_default_init(RpHttpClientConnectionInterface* iface)
{
    iface->new_stream = http_client_connection_new_stream_;
}

static void
rp_http1_stream_encoder_options_default_init(RpHttp1StreamEncoderOptionsInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
