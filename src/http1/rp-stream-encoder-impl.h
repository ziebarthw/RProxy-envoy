/*
 * rp-stream-encoder-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-http1-connection-impl.h"
#include "rp-codec-helper.h"

G_BEGIN_DECLS

/**
 * Base class for HTTP/1.1 request and response encoders.
 */
#define RP_TYPE_STREAM_ENCODER_IMPL rp_stream_encoder_impl_get_type()
G_DECLARE_DERIVABLE_TYPE(RpStreamEncoderImpl, rp_stream_encoder_impl, RP, STREAM_ENCODER_IMPL, RpStreamCallbackHelper)

struct _RpStreamEncoderImplClass {
    RpStreamCallbackHelperClass parent_class;

};

void rp_stream_encoder_impl_encode_headers_base(RpStreamEncoderImpl* self,
                                                evhtp_headers_t* headers,
                                                evhtp_res status,
                                                bool end_stream,
                                                bool bodiless_request);
void rp_stream_encoder_impl_encode_trailers_base(RpStreamEncoderImpl* self,
                                                    evhtp_headers_t* trailers);
void rp_stream_encoder_impl_set_connection_request(RpStreamEncoderImpl* self, bool v);
void rp_stream_encoder_impl_set_is_response_to_connect_request(RpStreamEncoderImpl* self, bool v);
void rp_stream_encoder_impl_set_is_response_to_head_request(RpStreamEncoderImpl* self, bool v);
bool rp_stream_encoder_impl_connect_request_(RpStreamEncoderImpl* self);
void rp_stream_encoder_impl_set_is_tcp_tunneling(RpStreamEncoderImpl* self, bool v);

G_END_DECLS
