/*
 * rp-decompressor-filter.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <glib-object.h>
#include "rp-factory-context.h"
#include "rp-pass-through-filter.h"

G_BEGIN_DECLS

#define RP_TYPE_DECOMPRESSOR_FILTER (rp_decompressor_filter_get_type())
G_DECLARE_FINAL_TYPE(RpDecompressorFilter, rp_decompressor_filter, RP, DECOMPRESSOR_FILTER, RpPassThroughFilter)

RpFilterFactoryCb* rp_decompressor_filter_create_filter_factory(RpFactoryContext* context);

G_END_DECLS
