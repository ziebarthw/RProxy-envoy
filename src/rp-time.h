/*
 * rp-time.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * Less typing for common system time and steady time type.
 *
 * SystemTime should be used when getting a time to present to the user, e.g. for logging.
 * MonotonicTime should be used when tracking time for computing an interval.
 */
typedef guint64 RpSeconds;
typedef guint64 RpSystemTime;
typedef gint64 RpMonotonicTime;

/**
 * Captures a system-time source, capable of computing both monotonically increasing
 * and real time.
 */
#define RP_TYPE_TIME_SOURCE rp_time_source_get_type()
G_DECLARE_INTERFACE(RpTimeSource, rp_time_source, RP, TIME_SOURCE, GObject)

struct _RpTimeSourceInterface {
    GTypeInterface parent_iface;

    /**
     * @return the current system time; not guaranteed to be monotonically increasing.
     */
    RpSystemTime (*system_time)(RpTimeSource*);
    /**
     * @return the current monotonic time.
     */
    RpMonotonicTime (*monotonic_time)(RpTimeSource*);
};

static inline RpSystemTime
rp_time_source_system_time(RpTimeSource* self)
{
    return RP_IS_TIME_SOURCE(self) ?
        RP_TIME_SOURCE_GET_IFACE(self)->system_time(self) : 0;
}
static inline RpMonotonicTime
rp_time_source_monotonic_time(RpTimeSource* self)
{
    return RP_IS_TIME_SOURCE(self) ?
        RP_TIME_SOURCE_GET_IFACE(self)->monotonic_time(self) : -1;
}

G_END_DECLS
