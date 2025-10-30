/*
 * rp-active-stream-encoder-filter.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <glib-object.h>
#include "rp-active-stream-filter-base.h"

G_BEGIN_DECLS

typedef struct _RpFilterManager RpFilterManager;

/**
 * Wrapper for a stream encoder filter.
 */
#define RP_TYPE_ACTIVE_STREAM_ENCODER_FILTER rp_active_stream_encoder_filter_get_type()
G_DECLARE_FINAL_TYPE(RpActiveStreamEncoderFilter, rp_active_stream_encoder_filter, RP, ACTIVE_STREAM_ENCODER_FILTER, RpActiveStreamFilterBase)

RpActiveStreamEncoderFilter* rp_active_stream_encoder_filter_new(RpFilterManager* parent,
                                                                    RpStreamEncoderFilter* filter,
                                                                    const struct RpFilterContext* filter_context);
GList* rp_active_stream_encoder_get_entry(RpActiveStreamEncoderFilter* self);
void rp_active_stream_encoder_set_entry(RpActiveStreamEncoderFilter* self,
                                        GList* entry);
RpStreamEncoderFilter* rp_active_stream_encoder_handle(RpActiveStreamEncoderFilter* self);

G_END_DECLS
