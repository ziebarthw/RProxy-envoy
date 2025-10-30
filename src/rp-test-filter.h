/*
 * rp-test-filter.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <glib-object.h>
#include "rproxy.h"
#include "rp-pass-through-filter.h"

G_BEGIN_DECLS

#define RP_TYPE_TEST_FILTER (rp_test_filter_get_type())
G_DECLARE_FINAL_TYPE(RpTestFilter, rp_test_filter, RP, TEST_FILTER, RpPassThroughFilter)

RpTestFilter* rp_test_filter_new(request_t* request);

G_END_DECLS
