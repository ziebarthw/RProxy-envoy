/*
 * rp-active-stream-decoder-filter.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <glib-object.h>
#include "rp-active-stream-filter-base.h"
#include "rp-filter-factory.h"

G_BEGIN_DECLS

typedef struct _RpFilterManager RpFilterManager;

/**
 * Wrapper for a stream decoder filter.
 */
#define RP_TYPE_ACTIVE_STREAM_DECODER_FILTER (rp_active_stream_decoder_filter_get_type())
G_DECLARE_FINAL_TYPE(RpActiveStreamDecoderFilter, rp_active_stream_decoder_filter, RP, ACTIVE_STREAM_DECODER_FILTER, RpActiveStreamFilterBase)

RpActiveStreamDecoderFilter* rp_active_stream_decoder_filter_new(RpFilterManager* parent,
                                                                    RpStreamDecoderFilter* filter,
                                                                    const struct RpFilterContext* filter_context);
GList* rp_active_stream_decoder_get_entry(RpActiveStreamDecoderFilter* self);
void rp_active_stream_decoder_set_entry(RpActiveStreamDecoderFilter* self,
                                        GList* entry);
RpFilterHeadersStatus_e rp_active_stream_decoder_decode_headers(RpActiveStreamDecoderFilter* self,
                                                                evhtp_headers_t* request_headers,
                                                                bool end_stream);
RpStreamDecoderFilter* rp_active_stream_decoder_handle(RpActiveStreamDecoderFilter* self);

G_END_DECLS
