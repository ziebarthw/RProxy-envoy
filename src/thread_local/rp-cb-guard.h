/*
 * rp-cb-guard.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "thread_local/rp-thread-local-impl.h"

G_BEGIN_DECLS

#define RP_TYPE_CB_GUARD rp_cb_guard_get_type()
G_DECLARE_FINAL_TYPE(RpCbGuard, rp_cb_guard, RP, CB_GUARD, GObject)

RpCbGuard* rp_cb_guard_new(RpThreadLocalInstanceImpl* parent,
                            void (*cb)(gpointer),
                            gpointer cb_arg,
                            void (*all_threads_complete_cb)(gpointer),
                            gpointer all_threads_complete_arg);
void rp_cb_guard_cb(gpointer arg);

G_END_DECLS
