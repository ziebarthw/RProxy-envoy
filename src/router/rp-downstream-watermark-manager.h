/*
 * rp-downstream-watermark-manager.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "router/rp-upstream-request.h"
#include "rp-codec.h"

G_BEGIN_DECLS

#define RP_TYPE_DOWNSTREAM_WATERMARK_MANAGER rp_downstream_watermark_manager_get_type()
G_DECLARE_FINAL_TYPE(RpDownstreamWatermarkManager, rp_downstream_watermark_manager, RP, DOWNSTREAM_WATERMARK_MANAGER, GObject)

RpDownstreamWatermarkManager* rp_downstream_watermark_manager_new(RpUpstreamRequest* parent);

G_END_DECLS
