/*
 * rp-file-event.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
    RpFileReadyType_Read = 0x1,
    RpFileReadyType_Write = 0x2,
    RpFileReadyType_Closed = 0x4
} RpFileReadyType_e;

typedef void (*RpFileReadyCb)(void*, guint32);

G_END_DECLS
