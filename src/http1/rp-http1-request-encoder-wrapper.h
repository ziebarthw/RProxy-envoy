/*
 * rp-http1-request-encoder-wrapper.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-request-encoder-wrapper.h"

G_BEGIN_DECLS

typedef struct _RpHttp1StreamWrapper RpHttp1StreamWrapper;

#define RP_TYPE_HTTP1_REQUEST_ENCODER_WRAPPER rp_http1_request_encoder_wrapper_get_type()
G_DECLARE_FINAL_TYPE(RpHttp1RequestEncoderWrapper, rp_http1_request_encoder_wrapper, RP, HTTP1_REQUEST_ENCODER_WRAPPER, RpRequestEncoderWrapper)

RpHttp1RequestEncoderWrapper* rp_http1_request_encoder_wrapper_new(RpHttp1StreamWrapper* parent,
                                                                    RpRequestEncoder* encoder);

G_END_DECLS
