/*
 * rp-codec-bridge.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-http-conn-pool.h"
#include "rp-http-filter.h"

G_BEGIN_DECLS

typedef struct _RpUpstreamCodecFilter RpUpstreamCodeFilter;

/*
 * This bridge connects the upstream stream to the filter manager.
 */
#define RP_TYPE_CODEC_BRIDGE rp_codec_bridge_get_type()
G_DECLARE_FINAL_TYPE(RpCodecBridge, rp_codec_bridge, RP, CODEC_BRIDGE, GObject)

RpCodecBridge* rp_codec_bridge_new(RpUpstreamCodeFilter* filter);

G_END_DECLS
