/*
 * rp-filter-state-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-stream-info.h"

G_BEGIN_DECLS

#define RP_TYPE_FILTER_STATE_IMPL rp_filter_state_impl_get_type()
G_DECLARE_FINAL_TYPE(RpFilterStateImpl, rp_filter_state_impl, RP, FILTER_STATE_IMPL, GObject)

RpFilterStateImpl* rp_filter_state_impl_new(RpFilterState* ancestor_filter_state,
                                            RpFilterStateLifeSpan_e life_span);
//TODO...setData(name, data, ...);

G_END_DECLS
