/*
 * tokenizer.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "delims.h"

G_BEGIN_DECLS

typedef struct tokenizer_cursor_s * tokenizer_cursor;
struct tokenizer_cursor_s {
    int m_lower_bound;
    int m_upper_bound;
    int m_pos;
};

static inline struct tokenizer_cursor_s
tokenizer_cursor_ctor(int lower_bound, int upper_bound)
{
    struct tokenizer_cursor_s self = {
        .m_lower_bound = lower_bound,
        .m_upper_bound = upper_bound,
        .m_pos = lower_bound
    };
    return self;
}

static inline int
tokenizer_cursor_get_lower_bound(tokenizer_cursor self)
{
    return self ? self->m_lower_bound : -1;
}

static inline int
tokenizer_cursor_get_upper_bound(tokenizer_cursor self)
{
    return self ? self->m_upper_bound : -1;
}

static inline int
tokenizer_cursor_get_pos(tokenizer_cursor self)
{
    return self ? self->m_pos : -1;
}

static inline void
tokenizer_cursor_set_pos(tokenizer_cursor self, int pos)
{
    if (self) self->m_pos = pos;
}

static inline bool
tokenizer_cursor_at_end(tokenizer_cursor self)
{
    return self ? self->m_pos >= self->m_upper_bound : true;
}

typedef struct tokenizer_s * tokenizer;
struct tokenizer_s {
    char DQUOTE;
    char ESCAPE;
    int CR;
    int LF;
    int SP;
    int HT;
};

static inline struct tokenizer_s
tokenizer_ctor(void)
{
    struct tokenizer_s self = {
        .DQUOTE = '"',
        .ESCAPE = '\\',
        .CR = '\r',
        .LF = '\n',
        .SP = ' ',
        .HT = '\t'
    };
    return self;
}

void tokenizer_skip_whitespace(tokenizer self,
                                const char* buf,
                                tokenizer_cursor cursor);
GString* tokenizer_parse_token(tokenizer self,
                                const char* buf,
                                tokenizer_cursor cursor,
                                delims delims);
GString* tokenizer_parse_value(tokenizer self,
                                const char* buf,
                                tokenizer_cursor cursor,
                                delims delims);

G_END_DECLS
