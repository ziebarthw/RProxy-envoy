/*
 * delims.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdarg.h>
#include <stdlib.h>
#include "delims.h"

struct delims_s
delims_ctor(int nargs, ...)
{
    va_list args;
    struct delims_s self;
    memset(&self, 0, sizeof(self));
    va_start(args, nargs);
    for (int i=0; i < nargs; ++i) self.map[va_arg(args, int)] = 1;
    va_end(args);
    return self;
}
