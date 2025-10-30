/*
 * rp-stream-wrapper.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-codec.h"
#include "rp-response-decoder-wrapper.h"

G_BEGIN_DECLS

typedef struct _RpHttp1CpActiveClient* RpHttp1CpActiveClientPtr;

#define RP_TYPE_HTTP1_STREAM_WRAPPER rp_http1_stream_wrapper_get_type()
G_DECLARE_FINAL_TYPE(RpHttp1StreamWrapper, rp_http1_stream_wrapper, RP, HTTP1_STREAM_WRAPPER, RpResponseDecoderWrapper)

RpHttp1StreamWrapper* rp_http1_stream_wrapper_new(RpResponseDecoder* response_decoder,
                                                    RpHttp1CpActiveClientPtr parent);
void rp_http1_stream_wrapper_encode_complete(RpHttp1StreamWrapper* self);
bool rp_http1_stream_wrapper_decode_complete_(RpHttp1StreamWrapper* self);

G_END_DECLS
