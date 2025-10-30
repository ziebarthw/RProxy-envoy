/*
 * rp-post-io-action.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * Action that should occur on a connection after I/O.
 */
typedef enum {
    RpPostIoAction_None,
    RpPostIoAction_Close,
    RpPostIoAction_KeepOpen
} RpPostIoAction_e;

G_END_DECLS
