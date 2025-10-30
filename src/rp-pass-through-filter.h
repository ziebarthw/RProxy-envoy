/*
 * rp-pass-through-filter.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-http-filter.h"

G_BEGIN_DECLS

#define RP_TYPE_PASS_THROUGH_FILTER rp_pass_through_filter_get_type()
G_DECLARE_DERIVABLE_TYPE(RpPassThroughFilter, rp_pass_through_filter, RP, PASS_THROUGH_FILTER, GObject)

struct _RpPassThroughFilterClass {
    GObjectClass parent_class;

};

RpStreamEncoderFilterCallbacks* rp_pass_through_filter_encoder_callbacks_(RpPassThroughFilter* self);
RpStreamDecoderFilterCallbacks* rp_pass_through_filter_decoder_callbacks_(RpPassThroughFilter* self);

G_END_DECLS
