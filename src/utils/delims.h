/*
 * delims.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct delims_s * delims;
struct delims_s {
    char map[256];
};

struct delims_s delims_ctor(int nargs, ...);

G_END_DECLS
