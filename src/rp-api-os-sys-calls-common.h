/*
 * rp-api-os-sys-calls-common.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef const char* _c_char_ptr_;
typedef int _os_fd_t_;

#define SYSCALL_TYPES \
    X(int, Int) \
    X(gssize, Size) \
    X(gpointer, Ptr) \
    X(_c_char_ptr_, String) \
    X(bool, Bool) \
    X(_os_fd_t_, Socket)

#define X(type, Cap) \
    struct RpSysCallResult_##type { type m_return_value; int m_errno; }; \
    typedef struct RpSysCallResult_##type RpSysCall##Cap##Result; \
    static inline RpSysCall##Cap##Result \
    rp_sys_call_##type##_ctor(type v, int e){return (RpSysCall##Cap##Result){v,e};}

SYSCALL_TYPES
#undef X

G_END_DECLS
