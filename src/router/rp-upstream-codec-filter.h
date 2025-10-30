/*
 * rp-upstream-codec-filter.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-factory-context.h"
#include "rp-http-conn-pool.h"
#include "rp-http-filter.h"

G_BEGIN_DECLS

/*
 * This is the last filter in the upstream HTTP filter chain.
 * It takes request headers/body/data from the filter manager and encodes them to the upstream
 * codec. It also registers the CodecBridge with the upstream stream, and takes response
 * headers/body/data from the upstream stream and sends them to the filter manager.
 */
#define RP_TYPE_UPSTREAM_CODEC_FILTER rp_upstream_codec_filter_get_type()
G_DECLARE_FINAL_TYPE(RpUpstreamCodecFilter, rp_upstream_codec_filter, RP, UPSTREAM_CODEC_FILTER, GObject)

RpFilterFactoryCb* rp_upstream_codec_filter_create_filter_factory(RpFactoryContext* context);
RpUpstreamTiming* rp_upstream_codec_filter_upstream_timing(RpUpstreamCodecFilter* self);
RpStreamDecoderFilterCallbacks* rp_upstream_codec_filter_callbacks_(RpUpstreamCodecFilter* self);
bool rp_upstream_codec_filter_calling_encode_headers_(RpUpstreamCodecFilter* self);

G_END_DECLS
