/*
 * rp-active-client-stream-wrapper.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "http1/rp-http1-conn-pool.h"
#include "rp-response-decoder-wrapper.h"

G_BEGIN_DECLS

#define RP_TYPE_ACTIVE_CLIENT_STREAM_WRAPPER rp_active_client_stream_wrapper_get_type()
G_DECLARE_FINAL_TYPE(RpActiveClientStreamWrapper, rp_active_client_stream_wrapper, RP, ACTIVE_CLIENT_STREAM_WRAPPER, GObject)

RpActiveClientStreamWrapper* rp_active_client_stream_wrapper_new(RpResponseDecoder* response_decoder,
                                                                    RpHttp1CpActiveClient* parent);
bool rp_active_client_stream_wrapper_decode_complete_(RpActiveClientStreamWrapper* self);

G_END_DECLS
