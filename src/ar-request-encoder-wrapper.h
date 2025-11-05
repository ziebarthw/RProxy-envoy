/*
 * ar-request-encoder-wrapper.h
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

#define AR_TYPE_REQUEST_ENCODER_WRAPPER ar_request_encoder_wrapper_get_type()
G_DECLARE_FINAL_TYPE(ArRequestEncoderWrapper, ar_request_encoder_wrapper, AR, REQUEST_ENCODER_WRAPPER, RpRequestEncoderWrapper)

ArRequestEncoderWrapper* ar_request_encoder_wrapper_new(RpRequestEncoder* inner,
                                                        RpCodecClient* parent,
                                                        gpointer arg);

G_END_DECLS
