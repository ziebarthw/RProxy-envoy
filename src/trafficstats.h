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

#define stats_inc(x) atomic_fetch_add_explicit(&(x), 1ULL, memory_order_relaxed)
#define stats_dec(x) atomic_fetch_sub_explicit(&(x), 1ULL, memory_order_relaxed)
#define stats_read(x) atomic_load(&(x))

G_BEGIN_DECLS

typedef struct traffic_stats traffic_stats_t;
struct traffic_stats {
    _Atomic guint64 downstream_cx_total __attribute__((aligned(CACHE_LINE_SIZE)));
    _Atomic guint64 downstream_cx_active __attribute__((aligned(CACHE_LINE_SIZE)));
    _Atomic guint64 downstream_cx_destroy __attribute__((aligned(CACHE_LINE_SIZE)));
    _Atomic guint64 downstream_rq_total __attribute__((aligned(CACHE_LINE_SIZE)));
    _Atomic guint64 downstream_rq_active __attribute__((aligned(CACHE_LINE_SIZE)));
    _Atomic guint64 upstream_cx_total __attribute__((aligned(CACHE_LINE_SIZE)));
    _Atomic guint64 upstream_cx_active __attribute__((aligned(CACHE_LINE_SIZE)));
    _Atomic guint64 upstream_rq_total __attribute__((aligned(CACHE_LINE_SIZE)));
    _Atomic guint64 upstream_rq_active __attribute__((aligned(CACHE_LINE_SIZE)));
};

extern traffic_stats_t g_traffic_stats;

G_END_DECLS
