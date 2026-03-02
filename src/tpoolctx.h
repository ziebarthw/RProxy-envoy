/*
 * tpoolctx.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdatomic.h>
#include <stdbool.h>
#include <glib-object.h>
#include "rp-thread-local.h"

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

G_BEGIN_DECLS

#define n_processing_inc(x) atomic_fetch_add_explicit(&(x.v), 1, memory_order_relaxed)
#define n_processing_dec(x) atomic_fetch_sub_explicit(&(x.v), 1, memory_order_relaxed)
#define n_processing_read(x) atomic_load(&(x.v))

typedef struct rproxy rproxy_t;
typedef struct server_cfg server_cfg_t;
typedef struct rproxy_hooks rproxy_hooks_t;

typedef struct _RpCachelineAtomicInt RpCachelineAtomicInt;
struct _RpCachelineAtomicInt {
    _Atomic gint v;
    /* pad out to a cache line boundary to reduce false sharing */
    guint8 pad[CACHE_LINE_SIZE - sizeof(_Atomic gint)];
};

/**
 * Simple per-threadpool context shared among the threads servicing a single
 * listener (i.e. server_cfg).
 */
struct tpool_ctx {
    rproxy_t* m_parent;
    server_cfg_t* m_server_cfg;
    const rproxy_hooks_t* m_hooks;
    RpThreadLocalInstance* m_tls;

    RpCachelineAtomicInt n_processing;
};

G_END_DECLS
