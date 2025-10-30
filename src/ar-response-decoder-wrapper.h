/*
 * ar-response-decoder-wrapper.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-codec-wrappers.h"
#include "rp-codec-client.h"

G_BEGIN_DECLS

typedef struct _RpCodecClientActiveRequest RpCodecClientActiveRequest;

#define AR_TYPE_RESPONSE_DECODER_WRAPPER ar_response_decoder_wrapper_get_type()
G_DECLARE_FINAL_TYPE(ArResponseDecoderWrapper, ar_response_decoder_wrapper, AR, RESPONSE_DECODER_WRAPPER, RpResponseDecoderWrapper)

ArResponseDecoderWrapper* ar_response_decoder_wrapper_new(RpResponseDecoder* inner,
                                                            RpCodecClient* parent,
                                                            RpCodecClientActiveRequest* active_request);

G_END_DECLS
