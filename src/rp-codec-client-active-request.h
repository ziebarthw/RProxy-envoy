/*
 * rp-codec-client-active-request.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-codec-client.h"
#include "rp-codec-wrappers.h"

G_BEGIN_DECLS

/**
 * Wrapper for an outstanding request. Designed for handling stream multiplexing.
 */
#define RP_TYPE_CODEC_CLIENT_ACTIVE_REQUEST rp_codec_client_active_request_get_type()
G_DECLARE_FINAL_TYPE(RpCodecClientActiveRequest, rp_codec_client_active_request, RP, CODEC_CLIENT_ACTIVE_REQUEST, GObject)

RpCodecClientActiveRequest* rp_codec_client_active_request_new(RpCodecClient* parent,
                                                                RpResponseDecoder* inner);
void rp_codec_client_active_request_set_encoder(RpCodecClientActiveRequest* self,
                                                RpRequestEncoder* encoder);
void rp_codec_client_active_request_set_decode_complete(RpCodecClientActiveRequest* self);
bool rp_codec_client_active_request_decode_complete_(RpCodecClientActiveRequest* self);
void rp_codec_client_active_request_set_encode_complete(RpCodecClientActiveRequest* self);
bool rp_codec_client_active_request_encode_complete_(RpCodecClientActiveRequest* self);
bool rp_codec_client_active_request_get_wait_encode_complete(RpCodecClientActiveRequest* self);
void rp_codec_client_active_request_remove_encoder_callbacks(RpCodecClientActiveRequest* self);

G_END_DECLS
