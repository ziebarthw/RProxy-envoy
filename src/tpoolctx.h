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

typedef struct rproxy rproxy_t;
typedef struct server_cfg server_cfg_t;
typedef struct rproxy_hooks rproxy_hooks_t;

/**
 * Simple per-threadpool context shared among the threads servicing a single
 * listener (i.e. server_cfg).
 */
struct tpool_ctx {
    rproxy_t* m_parent;
    server_cfg_t* m_server_cfg;
    const rproxy_hooks_t* m_hooks;
    RpThreadLocalInstance* m_tls;
    _Atomic gint n_processing __attribute__((aligned(CACHE_LINE_SIZE))); /**< number of in-flight requests */
};

G_END_DECLS
