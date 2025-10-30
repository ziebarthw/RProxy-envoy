/*
 * rp-codec-read-filter.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-net-filter-impl.h"

G_BEGIN_DECLS

#define RP_TYPE_CODEC_READ_FILTER rp_codec_read_filter_get_type()
G_DECLARE_FINAL_TYPE(RpCodecReadFilter, rp_codec_read_filter, RP, CODEC_READ_FILTER, RpNetworkReadFilterBaseImpl)

RpCodecReadFilter* rp_codec_read_filter_new(void* arg);

G_END_DECLS
