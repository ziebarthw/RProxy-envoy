/*
 * gduration.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

/* Define any missing time conversion macros */
#define G_NSEC_PER_USEC 1000
#define G_NSEC_PER_MSEC 1000000
#define G_USEC_PER_MSEC 1000
#define G_MSEC_PER_SEC 1000

/* Macros for basic unit conversions */
#define GD_SEC_TO_USEC(s)    ((GTimeSpan)((s) * G_USEC_PER_SEC))
#define GD_MSEC_TO_USEC(ms)  ((GTimeSpan)((ms) * G_USEC_PER_MSEC))
#define GD_NSEC_TO_USEC(ns)  ((GTimeSpan)((ns) / G_NSEC_PER_USEC))

#define GD_USEC_TO_SEC(us)   ((gdouble)(us) / G_USEC_PER_SEC)
#define GD_USEC_TO_MSEC(us)  ((gdouble)(us) / G_USEC_PER_MSEC)
#define GD_USEC_TO_NSEC(us)  ((gint64)(us) * G_NSEC_PER_USEC)

/* Optional inline functions for semantic clarity */
static inline GTimeSpan gd_seconds(gdouble sec) {
    return (GTimeSpan)(sec * G_USEC_PER_SEC);
}

static inline GTimeSpan gd_milliseconds(gint64 ms) {
    return ms * G_USEC_PER_MSEC;
}

static inline GTimeSpan gd_nanoseconds(gint64 ns) {
    return ns / G_NSEC_PER_USEC;
}

static inline gdouble gd_to_seconds(GTimeSpan ts) {
    return (gdouble)ts / G_USEC_PER_SEC;
}

static inline gint64 gd_to_milliseconds(GTimeSpan ts) {
    return ts / G_USEC_PER_MSEC;
}

static inline gint64 gd_to_nanoseconds(GTimeSpan ts) {
    return ts * G_NSEC_PER_USEC;
}

G_END_DECLS
