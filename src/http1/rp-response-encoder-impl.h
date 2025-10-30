/*
 * rp-response-encoder-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-stream-encoder-impl.h"

G_BEGIN_DECLS

/**
 * HTTP/1.1 response encoder.
 */
#define RP_TYPE_RESPONSE_ENCODER_IMPL rp_response_encoder_impl_get_type()
G_DECLARE_FINAL_TYPE(RpResponseEncoderImpl, rp_response_encoder_impl, RP, RESPONSE_ENCODER_IMPL, RpStreamEncoderImpl)

RpResponseEncoderImpl* rp_response_encoder_impl_new(RpHttp1ConnectionImpl* connection,
                                    bool stream_error_on_invalid_http_message);
bool rp_response_encoder_impl_started_reponse(RpResponseEncoderImpl* self);

G_END_DECLS
