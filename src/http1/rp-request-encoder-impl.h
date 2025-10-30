/*
 * rp-request-encoder-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-codec.h"
#include "rp-stream-encoder-impl.h"
#include "rproxy.h"

G_BEGIN_DECLS

/**
 * HTTP/1.1 request encoder.
 */
#define RP_TYPE_REQUEST_ENCODER_IMPL rp_request_encoder_impl_get_type()
G_DECLARE_FINAL_TYPE(RpRequestEncoderImpl, rp_request_encoder_impl, RP, REQUEST_ENCODER_IMPL, RpStreamEncoderImpl)

RpRequestEncoderImpl* rp_request_encoder_impl_new(RpHttp1ConnectionImpl* connection);
bool rp_request_encoder_impl_head_request(RpRequestEncoderImpl* self);
bool rp_request_encoder_impl_connect_request(RpRequestEncoderImpl* self);

G_END_DECLS
