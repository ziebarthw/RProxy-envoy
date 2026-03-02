/*
 * trafficstats.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdatomic.h>
#include <stdbool.h>
#include <glib-object.h>

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

#define stats_inc(x) atomic_fetch_add_explicit(&(x.v), 1ULL, memory_order_relaxed)
#define stats_dec(x) atomic_fetch_sub_explicit(&(x.v), 1ULL, memory_order_relaxed)
#define stats_read(x) atomic_load(&(x.v))

G_BEGIN_DECLS

typedef struct _RpCachelineAtomicUint64 RpCachelineAtomicUint64;
struct _RpCachelineAtomicUint64 {
    _Atomic guint64 v;
    /* pad out to a cache line boundary to reduce false sharing */
    guint8 pad[CACHE_LINE_SIZE - sizeof(_Atomic guint64)];
};

typedef struct traffic_stats traffic_stats_t;
struct traffic_stats {
    RpCachelineAtomicUint64 downstream_cx_total;
    RpCachelineAtomicUint64 downstream_cx_active;
    RpCachelineAtomicUint64 downstream_cx_destroy;
    RpCachelineAtomicUint64 downstream_rq_total;
    RpCachelineAtomicUint64 downstream_rq_active;
    RpCachelineAtomicUint64 upstream_cx_total;
    RpCachelineAtomicUint64 upstream_cx_active;
    RpCachelineAtomicUint64 upstream_rq_total;
    RpCachelineAtomicUint64 upstream_rq_active;
};

extern traffic_stats_t g_traffic_stats;

G_END_DECLS
