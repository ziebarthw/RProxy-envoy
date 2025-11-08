/*
 * rp-request-encoder-wrapper.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-codec.h"

G_BEGIN_DECLS

/**
 * Wrapper for RequestEncoder that just forwards to an "inner" encoder.
 */
#define RP_TYPE_REQUEST_ENCODER_WRAPPER rp_request_encoder_wrapper_get_type()
G_DECLARE_FINAL_TYPE(RpRequestEncoderWrapper, rp_request_encoder_wrapper, RP, REQUEST_ENCODER_WRAPPER, GObject)

RpRequestEncoderWrapper* rp_request_encoder_wrapper_new(RpRequestEncoder* inner,
                                                        void (*on_encode_complete)(GObject*),
                                                        GObject* arg);
void rp_request_encoder_wrapper_set_inner_encoder_(RpRequestEncoderWrapper* self,
                                                    RpRequestEncoder* inner);

G_END_DECLS
