/*
 * rp-response-decoder-wrapper.h
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
 * Wrapper for ResponseDecoder that just forwards to an "inner" decoder.
 */
#define RP_TYPE_RESPONSE_DECODER_WRAPPER rp_response_decoder_wrapper_get_type()
G_DECLARE_FINAL_TYPE(RpResponseDecoderWrapper, rp_response_decoder_wrapper, RP, RESPONSE_DECODER_WRAPPER, GObject)

RpResponseDecoderWrapper* rp_response_decoder_wrapper_new(RpResponseDecoder* inner,
                                                            void (*on_pre_decode_complete)(GObject*),
                                                            void (*on_decode_complete)(GObject*),
                                                            GObject* arg);

G_END_DECLS
