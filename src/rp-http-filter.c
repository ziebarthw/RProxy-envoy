/*
 * rp-http-filter.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "rp-http-filter.h"

G_DEFINE_INTERFACE(RpStreamFilterCallbacks, rp_stream_filter_callbacks, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpStreamFilterBase, rp_stream_filter_base, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpStreamDecoderFilterCallbacks, rp_stream_decoder_filter_callbacks, RP_TYPE_STREAM_FILTER_CALLBACKS)
G_DEFINE_INTERFACE(RpStreamDecoderFilter, rp_stream_decoder_filter, RP_TYPE_STREAM_FILTER_BASE)
G_DEFINE_INTERFACE(RpStreamEncoderFilterCallbacks, rp_stream_encoder_filter_callbacks, RP_TYPE_STREAM_FILTER_CALLBACKS)
G_DEFINE_INTERFACE(RpStreamEncoderFilter, rp_stream_encoder_filter, RP_TYPE_STREAM_FILTER_BASE)
G_DEFINE_INTERFACE(RpStreamFilter, rp_stream_filter, RP_TYPE_STREAM_FILTER_BASE)
G_DEFINE_INTERFACE(RpFilterChainFactoryCallbacks, rp_filter_chain_factory_callbacks, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpDownstreamStreamFilterCallbacks, rp_downstream_stream_filter_callbacks, G_TYPE_OBJECT)

G_DEFINE_INTERFACE(RpUpstreamCallbacks, rp_upstream_callbacks, G_TYPE_OBJECT)
static void
rp_upstream_callbacks_default_init(RpUpstreamCallbacksInterface* iface G_GNUC_UNUSED)
{}

G_DEFINE_INTERFACE(RpUpstreamStreamFilterCallbacks, rp_upstream_stream_filter_callbacks, G_TYPE_OBJECT)
static void
rp_upstream_stream_filter_callbacks_default_init(RpUpstreamStreamFilterCallbacksInterface* iface G_GNUC_UNUSED)
{}

G_DEFINE_INTERFACE(RpRouteCache, rp_route_cache, G_TYPE_OBJECT)
static void
rp_route_cache_default_init(RpRouteCacheInterface* iface G_GNUC_UNUSED)
{}

static void
rp_downstream_stream_filter_callbacks_default_init(RpDownstreamStreamFilterCallbacksInterface* iface G_GNUC_UNUSED)
{}

static const char*
filter_config_name_(RpStreamFilterCallbacks* self) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
    return NULL;
}

static evhtp_headers_t*
request_headers_(RpStreamFilterCallbacks* self) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
    return NULL;
}

static evhtp_headers_t*
request_trailers_(RpStreamFilterCallbacks* self) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
    return NULL;
}

static evhtp_headers_t*
response_headers_(RpStreamFilterCallbacks* self) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
    return NULL;
}

static evhtp_headers_t*
response_trailers_(RpStreamFilterCallbacks* self) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
    return NULL;
}

static void
reset_idle_timer_(RpStreamFilterCallbacks* self) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static RpUpstreamStreamFilterCallbacks*
upstream_callbacks_(RpStreamFilterCallbacks* self G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
    return NULL;
}

static RpDownstreamStreamFilterCallbacks*
downstream_callbacks_(RpStreamFilterCallbacks* self G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
    return NULL;
}

static void
rp_stream_filter_callbacks_default_init(RpStreamFilterCallbacksInterface* iface) {
    iface->filter_config_name = filter_config_name_;
    iface->request_headers = request_headers_;
    iface->request_trailers = request_trailers_;
    iface->response_headers = response_headers_;
    iface->response_trailers = response_trailers_;
    iface->reset_idle_timer = reset_idle_timer_;
    iface->upstream_callbacks = upstream_callbacks_;
    iface->downstream_callbacks = downstream_callbacks_;
}

const char*
rp_stream_filter_callbacks_filter_config_name(RpStreamFilterCallbacks* self) {
    g_return_val_if_fail(RP_IS_STREAM_FILTER_CALLBACKS(self), NULL);
    return RP_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->filter_config_name(self);
}

evhtp_headers_t*
rp_stream_filter_callbacks_request_headers(RpStreamFilterCallbacks* self) {
    g_return_val_if_fail(RP_IS_STREAM_FILTER_CALLBACKS(self), NULL);
    return RP_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->request_headers(self);
}

evhtp_headers_t*
rp_stream_filter_callbacks_response_headers(RpStreamFilterCallbacks* self) {
    g_return_val_if_fail(RP_IS_STREAM_FILTER_CALLBACKS(self), NULL);
    return RP_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->response_headers(self);
}

void
rp_stream_filter_callbacks_reset_idle_timer(RpStreamFilterCallbacks* self) {
    g_return_if_fail(RP_IS_STREAM_FILTER_CALLBACKS(self));
    RP_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->reset_idle_timer(self);
}

static void
rp_stream_filter_base_default_on_stream_complete(RpStreamFilterBase* self G_GNUC_UNUSED) {
    g_debug("%s:%s(%p) - not implemented [%d]", __FILE__, __func__, self, __LINE__);
}

static void
rp_stream_filter_base_default_on_destroy(RpStreamFilterBase* self) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static RpLocalErrorStatus_e
rp_stream_filter_base_default_on_local_reply(RpStreamFilterBase* self G_GNUC_UNUSED, const struct RpStreamFilterBase_LocalReplyData* data G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
    return RpLocalErrorStatus_Continue;
}

static void
rp_stream_filter_base_default_init(RpStreamFilterBaseInterface* iface) {
    iface->on_stream_complete = rp_stream_filter_base_default_on_stream_complete;
    iface->on_destroy = rp_stream_filter_base_default_on_destroy;
    iface->on_local_reply = rp_stream_filter_base_default_on_local_reply;
}

void
rp_stream_filter_base_on_stream_complete(RpStreamFilterBase* self) {
    g_return_if_fail(RP_IS_STREAM_FILTER_BASE(self));
    RP_STREAM_FILTER_BASE_GET_IFACE(self)->on_stream_complete(self);
}

void
rp_stream_filter_base_on_destroy(RpStreamFilterBase* self) {
    g_return_if_fail(RP_IS_STREAM_FILTER_BASE(self));
    RP_STREAM_FILTER_BASE_GET_IFACE(self)->on_destroy(self);
}

static void
rp_stream_decoder_filter_callbacks_default_continue_decoding(RpStreamDecoderFilterCallbacks* self G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static const evbuf_t *
rp_stream_decoder_filter_callbacks_default_decoding_buffer(RpStreamDecoderFilterCallbacks* self G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
    return NULL;
}

static void
rp_stream_decoder_filter_callbacks_default_modify_decoding_buffer(RpStreamDecoderFilterCallbacks* self G_GNUC_UNUSED, void (*callback)(evbuf_t *) G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static void
rp_stream_decoder_filter_callbacks_default_add_decoded_data(RpStreamDecoderFilterCallbacks* self G_GNUC_UNUSED, evbuf_t* data G_GNUC_UNUSED, bool streaming_filter G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static void
rp_stream_decoder_filter_callbacks_default_inject_decoded_data_to_filter_chain(RpStreamDecoderFilterCallbacks* self G_GNUC_UNUSED, evbuf_t* data G_GNUC_UNUSED, bool end_stream G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static evhtp_headers_t*
rp_stream_decoder_filter_callbacks_default_add_decoded_trailers(RpStreamDecoderFilterCallbacks* self G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
    return NULL;
}

static void
rp_stream_decoder_filter_callbacks_default_send_local_reply(RpStreamDecoderFilterCallbacks* self G_GNUC_UNUSED, evhtp_res response_code G_GNUC_UNUSED, evbuf_t* body_text G_GNUC_UNUSED, modify_headers_cb modify_headers G_GNUC_UNUSED, const char* details G_GNUC_UNUSED, void* arg G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static void
rp_stream_decoder_filter_callbacks_default_endode_headers(RpStreamDecoderFilterCallbacks* self G_GNUC_UNUSED, evhtp_headers_t* response_headers G_GNUC_UNUSED, bool end_stream G_GNUC_UNUSED, const char* details G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static void
rp_stream_decoder_filter_callbacks_default_endode_data(RpStreamDecoderFilterCallbacks* self G_GNUC_UNUSED, evbuf_t* data G_GNUC_UNUSED, bool end_stream G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static void
rp_stream_decoder_filter_callbacks_default_endode_trailers(RpStreamDecoderFilterCallbacks* self G_GNUC_UNUSED, evhtp_headers_t* trailers G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static void
rp_stream_decoder_filter_callbacks_default_init(RpStreamDecoderFilterCallbacksInterface* iface) {
    iface->continue_decoding = rp_stream_decoder_filter_callbacks_default_continue_decoding;
    iface->decoding_buffer = rp_stream_decoder_filter_callbacks_default_decoding_buffer;
    iface->modify_decoding_buffer = rp_stream_decoder_filter_callbacks_default_modify_decoding_buffer;
    iface->add_decoded_data = rp_stream_decoder_filter_callbacks_default_add_decoded_data;
    iface->inject_decoded_data_to_filter_chain = rp_stream_decoder_filter_callbacks_default_inject_decoded_data_to_filter_chain;
    iface->add_decoded_trailers = rp_stream_decoder_filter_callbacks_default_add_decoded_trailers;
    iface->send_local_reply = rp_stream_decoder_filter_callbacks_default_send_local_reply;
    iface->encode_headers = rp_stream_decoder_filter_callbacks_default_endode_headers;
    iface->encode_data = rp_stream_decoder_filter_callbacks_default_endode_data;
    iface->encode_trailers = rp_stream_decoder_filter_callbacks_default_endode_trailers;
}

void
rp_stream_decoder_filter_callbacks_continue_decoding(RpStreamDecoderFilterCallbacks* self) {
    g_return_if_fail(RP_IS_STREAM_DECODER_FILTER_CALLBACKS(self));
    RP_STREAM_DECODER_FILTER_CALLBACKS_GET_IFACE(self)->continue_decoding(self);
}

const evbuf_t*
rp_stream_decoder_filter_callbacks_decoding_buffer(RpStreamDecoderFilterCallbacks* self) {
    g_return_val_if_fail(RP_IS_STREAM_DECODER_FILTER_CALLBACKS(self), NULL);
    return RP_STREAM_DECODER_FILTER_CALLBACKS_GET_IFACE(self)->decoding_buffer(self);
}

void
rp_stream_decoder_filter_callbacks_modify_decoding_buffer(RpStreamDecoderFilterCallbacks* self, void (*callback)(evbuf_t *)) {
    g_return_if_fail(RP_IS_STREAM_DECODER_FILTER_CALLBACKS(self));
    RP_STREAM_DECODER_FILTER_CALLBACKS_GET_IFACE(self)->modify_decoding_buffer(self, callback);
}

void
rp_stream_decoder_filter_callbacks_add_decoded_data(RpStreamDecoderFilterCallbacks* self, evbuf_t* data, bool streaming_filter) {
    g_return_if_fail(RP_IS_STREAM_DECODER_FILTER_CALLBACKS(self));
    RP_STREAM_DECODER_FILTER_CALLBACKS_GET_IFACE(self)->add_decoded_data(self, data, streaming_filter);
}

void
rp_stream_decoder_filter_callbacks_inject_decoded_data_to_filter_chain(RpStreamDecoderFilterCallbacks* self, evbuf_t* data, bool end_stream) {
    g_return_if_fail(RP_IS_STREAM_DECODER_FILTER_CALLBACKS(self));
    RP_STREAM_DECODER_FILTER_CALLBACKS_GET_IFACE(self)->inject_decoded_data_to_filter_chain(self, data, end_stream);
}

void
rp_stream_decoder_filter_callbacks_send_local_reply(RpStreamDecoderFilterCallbacks* self, evhtp_res response_code, evbuf_t* body_text, modify_headers_cb modify_headers, const char* details, void* arg) {
    g_return_if_fail(RP_IS_STREAM_DECODER_FILTER_CALLBACKS(self));
    RP_STREAM_DECODER_FILTER_CALLBACKS_GET_IFACE(self)->send_local_reply(self, response_code, body_text, modify_headers, details, arg);
}

static RpFilterHeadersStatus_e
rp_stream_decoder_filter_default_decode_headers(RpStreamDecoderFilter* self G_GNUC_UNUSED, evhtp_headers_t* request_headers G_GNUC_UNUSED, bool end_stream G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
    return RpFilterHeadersStatus_Continue;
}

static RpFilterDataStatus_e
rp_stream_decoder_filter_default_decode_data(RpStreamDecoderFilter* self G_GNUC_UNUSED, evbuf_t* data G_GNUC_UNUSED, bool end_stream G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
    return RpFilterDataStatus_Continue;
}

static void
rp_stream_decoder_filter_default_set_decoder_filter_callbacks(RpStreamDecoderFilter* self G_GNUC_UNUSED, RpStreamDecoderFilterCallbacks* callbacks G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static void
rp_stream_decoder_filter_default_decode_complete(RpStreamDecoderFilter* self G_GNUC_UNUSED) {
    g_debug("%s:%s(%p) - not implemented [%d]", __FILE__, __func__, self, __LINE__);
}

static void
rp_stream_decoder_filter_default_init(RpStreamDecoderFilterInterface* iface) {
    iface->decode_headers = rp_stream_decoder_filter_default_decode_headers;
    iface->decode_data = rp_stream_decoder_filter_default_decode_data;
    iface->set_decoder_filter_callbacks = rp_stream_decoder_filter_default_set_decoder_filter_callbacks;
    iface->decode_complete = rp_stream_decoder_filter_default_decode_complete;
}

RpFilterHeadersStatus_e
rp_stream_decoder_filter_decode_headers(RpStreamDecoderFilter* self, evhtp_headers_t* request_headers, bool end_stream) {
    g_return_val_if_fail(RP_IS_STREAM_DECODER_FILTER(self), RpFilterHeadersStatus_Continue);
    return RP_STREAM_DECODER_FILTER_GET_IFACE(self)->decode_headers(self, request_headers, end_stream);
}

RpFilterDataStatus_e
rp_stream_decoder_filter_decode_data(RpStreamDecoderFilter* self, evbuf_t* data, bool end_stream) {
    g_return_val_if_fail(RP_IS_STREAM_DECODER_FILTER(self), RpFilterDataStatus_Continue);
    g_return_val_if_fail(data != NULL, RpFilterDataStatus_StopIterationNoBuffer);
    return RP_STREAM_DECODER_FILTER_GET_IFACE(self)->decode_data(self, data, end_stream);
}

void
rp_stream_decoder_filter_set_decoder_filter_callbacks(RpStreamDecoderFilter* self, RpStreamDecoderFilterCallbacks* callbacks) {
    g_return_if_fail(RP_IS_STREAM_DECODER_FILTER(self));
    RP_STREAM_DECODER_FILTER_GET_IFACE(self)->set_decoder_filter_callbacks(self, callbacks);
}

void
rp_stream_decoder_filter_decode_complete(RpStreamDecoderFilter* self) {
    g_return_if_fail(RP_IS_STREAM_DECODER_FILTER(self));
    RP_STREAM_DECODER_FILTER_GET_IFACE(self)->decode_complete(self);
}

static void
rp_stream_encoder_filter_callbacks_default_continue_encoding(RpStreamEncoderFilterCallbacks* self G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static const evbuf_t*
rp_stream_encoder_filter_callbacks_default_encoding_buffer(RpStreamEncoderFilterCallbacks* self G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
    return NULL;
}

static void
rp_stream_encoder_filter_callbacks_default_modify_encoding_buffer(RpStreamEncoderFilterCallbacks* self G_GNUC_UNUSED, void (*callback)(evbuf_t *) G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static void
rp_stream_encoder_filter_callbacks_default_add_encoded_data(RpStreamEncoderFilterCallbacks* self G_GNUC_UNUSED, evbuf_t* data G_GNUC_UNUSED, bool streaming_filter G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static void
rp_stream_encoder_filter_callbacks_default_inject_encoded_data_to_filter_chain(RpStreamEncoderFilterCallbacks* self G_GNUC_UNUSED, evbuf_t* data G_GNUC_UNUSED, bool end_stream G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static void
rp_stream_encoder_filter_callbacks_default_send_local_reply(RpStreamEncoderFilterCallbacks* self G_GNUC_UNUSED, evhtp_res response_code G_GNUC_UNUSED, evbuf_t* body_text G_GNUC_UNUSED, void (*modify_headers)(evhtp_headers_t *) G_GNUC_UNUSED, const char* details G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static void
rp_stream_encoder_filter_callbacks_default_init(RpStreamEncoderFilterCallbacksInterface* iface) {
    iface->continue_encoding = rp_stream_encoder_filter_callbacks_default_continue_encoding;
    iface->encoding_buffer = rp_stream_encoder_filter_callbacks_default_encoding_buffer;
    iface->modify_encoding_buffer = rp_stream_encoder_filter_callbacks_default_modify_encoding_buffer;
    iface->add_encoded_data = rp_stream_encoder_filter_callbacks_default_add_encoded_data;
    iface->inject_encoded_data_to_filter_chain = rp_stream_encoder_filter_callbacks_default_inject_encoded_data_to_filter_chain;
    iface->send_local_reply = rp_stream_encoder_filter_callbacks_default_send_local_reply;
}

void
rp_stream_encoder_filter_callbacks_continue_encoding(RpStreamEncoderFilterCallbacks* self) {
    g_return_if_fail(RP_IS_STREAM_ENCODER_FILTER_CALLBACKS(self));
    RP_STREAM_ENCODER_FILTER_CALLBACKS_GET_IFACE(self)->continue_encoding(self);
}

const evbuf_t*
rp_stream_encoder_filter_callbacks_encoding_buffer(RpStreamEncoderFilterCallbacks* self) {
    g_return_val_if_fail(RP_IS_STREAM_ENCODER_FILTER_CALLBACKS(self), NULL);
    return RP_STREAM_ENCODER_FILTER_CALLBACKS_GET_IFACE(self)->encoding_buffer(self);
}

void
rp_stream_encoder_filter_callbacks_modify_decoding_buffer(RpStreamEncoderFilterCallbacks* self, void (*callback)(evbuf_t *)) {
    g_return_if_fail(RP_IS_STREAM_ENCODER_FILTER_CALLBACKS(self));
    RP_STREAM_ENCODER_FILTER_CALLBACKS_GET_IFACE(self)->modify_encoding_buffer(self, callback);
}

void
rp_stream_encoder_filter_callbacks_add_decoded_data(RpStreamEncoderFilterCallbacks* self, evbuf_t* data, bool streaming_filter) {
    g_return_if_fail(RP_IS_STREAM_ENCODER_FILTER_CALLBACKS(self));
    RP_STREAM_ENCODER_FILTER_CALLBACKS_GET_IFACE(self)->add_encoded_data(self, data, streaming_filter);
}

void
rp_stream_encoder_filter_callbacks_inject_decoded_data_to_filter_chain(RpStreamEncoderFilterCallbacks* self, evbuf_t* data, bool end_stream) {
    g_return_if_fail(RP_IS_STREAM_ENCODER_FILTER_CALLBACKS(self));
    RP_STREAM_ENCODER_FILTER_CALLBACKS_GET_IFACE(self)->inject_encoded_data_to_filter_chain(self, data, end_stream);
}

void
rp_stream_encoder_filter_callbacks_send_local_reply(RpStreamEncoderFilterCallbacks* self, evhtp_res response_code, evbuf_t* body_text, void (*modify_headers)(evhtp_headers_t *), const char* details) {
    g_return_if_fail(RP_IS_STREAM_ENCODER_FILTER_CALLBACKS(self));
    RP_STREAM_ENCODER_FILTER_CALLBACKS_GET_IFACE(self)->send_local_reply(self, response_code, body_text, modify_headers, details);
}

static RpFilterHeadersStatus_e
rp_stream_encoder_filter_default_encode_headers(RpStreamEncoderFilter* self G_GNUC_UNUSED, evhtp_headers_t* response_headers G_GNUC_UNUSED, bool end_stream G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
    return RpFilterHeadersStatus_Continue;
}

static RpFilterDataStatus_e
rp_stream_encoder_filter_default_encode_data(RpStreamEncoderFilter* self G_GNUC_UNUSED, evbuf_t* data G_GNUC_UNUSED, bool end_stream G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
    return RpFilterDataStatus_Continue;
}

static RpFilterTrailerStatus_e
rp_stream_encoder_filter_default_encode_trailers(RpStreamEncoderFilter* self G_GNUC_UNUSED, evhtp_headers_t* trailers G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
    return RpFilterTrailerStatus_Continue;
}

static void
rp_stream_encoder_filter_default_set_encoder_filter_callbacks(RpStreamEncoderFilter* self G_GNUC_UNUSED, RpStreamEncoderFilterCallbacks* callbacks G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static void
rp_stream_encoder_filter_default_encode_complete(RpStreamEncoderFilter* self G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static void
rp_stream_encoder_filter_default_init(RpStreamEncoderFilterInterface* iface) {
    iface->encode_headers = rp_stream_encoder_filter_default_encode_headers;
    iface->encode_data = rp_stream_encoder_filter_default_encode_data;
    iface->encode_trailers = rp_stream_encoder_filter_default_encode_trailers;
    iface->set_encoder_filter_callbacks = rp_stream_encoder_filter_default_set_encoder_filter_callbacks;
    iface->encode_complete = rp_stream_encoder_filter_default_encode_complete;
}

RpFilterHeadersStatus_e
rp_stream_encoder_filter_encode_headers(RpStreamEncoderFilter* self, evhtp_headers_t* response_headers, bool end_stream) {
    g_return_val_if_fail(RP_IS_STREAM_ENCODER_FILTER(self), RpFilterHeadersStatus_Continue);
    return RP_STREAM_ENCODER_FILTER_GET_IFACE(self)->encode_headers(self, response_headers, end_stream);
}

RpFilterDataStatus_e
rp_stream_encoder_filter_encode_data(RpStreamEncoderFilter* self, evbuf_t* data, bool end_stream) {
    g_return_val_if_fail(RP_IS_STREAM_ENCODER_FILTER(self), RpFilterDataStatus_Continue);
    return RP_STREAM_ENCODER_FILTER_GET_IFACE(self)->encode_data(self, data, end_stream);
}

void
rp_stream_encoder_filter_set_encoder_filter_callbacks(RpStreamEncoderFilter* self, RpStreamEncoderFilterCallbacks* callbacks) {
    g_return_if_fail(RP_IS_STREAM_ENCODER_FILTER(self));
    RP_STREAM_ENCODER_FILTER_GET_IFACE(self)->set_encoder_filter_callbacks(self, callbacks);
}

void
rp_stream_encoder_filter_encode_complete(RpStreamEncoderFilter* self) {
    g_return_if_fail(RP_IS_STREAM_ENCODER_FILTER(self));
    RP_STREAM_ENCODER_FILTER_GET_IFACE(self)->encode_complete(self);
}

static void
rp_stream_filter_default_init(RpStreamFilterInterface* iface G_GNUC_UNUSED) {
    // Nothing required here, as it just inherits other interfaces.
}

static void
add_stream_encoder_filter_(RpFilterChainFactoryCallbacks* self G_GNUC_UNUSED, RpStreamEncoderFilter* filter G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static void
add_stream_decoder_filter_(RpFilterChainFactoryCallbacks* self G_GNUC_UNUSED, RpStreamDecoderFilter* filter G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static void
add_stream_filter_(RpFilterChainFactoryCallbacks* self G_GNUC_UNUSED, RpStreamFilter* filter G_GNUC_UNUSED) {
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static void
rp_filter_chain_factory_callbacks_default_init(RpFilterChainFactoryCallbacksInterface* iface) {
    iface->add_stream_encoder_filter = add_stream_encoder_filter_;
    iface->add_stream_decoder_filter = add_stream_decoder_filter_;
    iface->add_stream_filter = add_stream_filter_;
}

void
rp_filter_chain_factory_callbacks_add_stream_encoder_filter(RpFilterChainFactoryCallbacks* self, RpStreamEncoderFilter* filter) {
    g_return_if_fail(RP_IS_FILTER_CHAIN_FACTORY_CALLBACKS(self));
    RP_FILTER_CHAIN_FACTORY_CALLBACKS_GET_IFACE(self)->add_stream_encoder_filter(self, filter);
}

void
rp_filter_chain_factory_callbacks_add_stream_decoder_filter(RpFilterChainFactoryCallbacks* self, RpStreamDecoderFilter* filter) {
    g_return_if_fail(RP_IS_FILTER_CHAIN_FACTORY_CALLBACKS(self));
    RP_FILTER_CHAIN_FACTORY_CALLBACKS_GET_IFACE(self)->add_stream_decoder_filter(self, filter);
}

void
rp_filter_chain_factory_callbacks_add_stream_filter(RpFilterChainFactoryCallbacks* self, RpStreamFilter* filter) {
    g_return_if_fail(RP_IS_FILTER_CHAIN_FACTORY_CALLBACKS(self));
    RP_FILTER_CHAIN_FACTORY_CALLBACKS_GET_IFACE(self)->add_stream_filter(self, filter);
}
