/*
 * rp-real-time-system.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-timer.h"

G_BEGIN_DECLS

// NOTE: This implements the TimeSource interface implemented by RealTimeSource
//       from common/utility.h in envoy.

/**
 * Real-world time implementation of TimeSystem.
 */
#define RP_TYPE_REAL_TIME_SYSTEM rp_real_time_system_get_type()
G_DECLARE_FINAL_TYPE(RpRealTimeSystem, rp_real_time_system, RP, REAL_TIME_SYSTEM, GObject)

RpRealTimeSystem* rp_real_time_system_new(void);

G_END_DECLS
