/*
 * tokenizer.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdbool.h>
#include "tokenizer.h"

static inline bool
isWhitespace(tokenizer self, char ch)
{
    return ch == self->SP || ch == self->HT || ch == self->CR || ch == self->LF;
}

void
tokenizer_skip_whitespace(tokenizer self, const char* buf, tokenizer_cursor cursor)
{
    int pos = tokenizer_cursor_get_pos(cursor);
    int indexFrom = pos;
    int indexTo = tokenizer_cursor_get_upper_bound(cursor);
    for (int i = indexFrom; i < indexTo; ++i)
    {
        char ch = buf[i];
        if (!isWhitespace(self, ch))
        {
            break;
        }
        ++pos;
    }
    tokenizer_cursor_set_pos(cursor, pos);
}

static void
copy_content(tokenizer self, const char* buf, tokenizer_cursor cursor, delims delims, GString* dst)
{
    int pos = tokenizer_cursor_get_pos(cursor);
    int indexFrom = pos;
    int indexTo = tokenizer_cursor_get_upper_bound(cursor);
    for (int i = indexFrom; i < indexTo; ++i)
    {
        char ch = buf[i];
        if ((delims && delims->map[(int)ch]) || isWhitespace(self, ch))
        {
            break;
        }
        ++pos;
        g_string_append_c(dst, ch);
    }
    tokenizer_cursor_set_pos(cursor, pos);
}

static void
copy_unquoted_content(tokenizer self, const char* buf, tokenizer_cursor cursor, delims delims, GString* dst)
{
    int pos = tokenizer_cursor_get_pos(cursor);
    int indexFrom = pos;
    int indexTo = tokenizer_cursor_get_upper_bound(cursor);
    for (int i = indexFrom; i < indexTo; ++i)
    {
        char ch = buf[i];
        if ((delims && delims->map[(int)ch]) || isWhitespace(self, ch) || ch == self->DQUOTE)
        {
            break;
        }
        ++pos;
        g_string_append_c(dst, ch);
    }
    tokenizer_cursor_set_pos(cursor, pos);
}

static void
copy_quoted_content(tokenizer self, const char* buf, tokenizer_cursor cursor, GString* dst)
{
    if (tokenizer_cursor_at_end(cursor))
    {
        return;
    }
    int pos = tokenizer_cursor_get_pos(cursor);
    int indexFrom = pos;
    int indexTo = tokenizer_cursor_get_upper_bound(cursor);
    char ch = buf[pos];
    if (ch != self->DQUOTE)
    {
        return;
    }
    ++pos;
    ++indexFrom;
    bool escaped = false;
    for (int i = indexFrom; i < indexTo; ++i, ++pos)
    {
        ch = buf[i];
        if (escaped)
        {
            if (ch != self->DQUOTE && ch != self->ESCAPE)
            {
                g_string_append_c(dst, self->ESCAPE);
            }
            g_string_append_c(dst, ch);
            escaped = false;
        }
        else
        {
            if (ch == self->DQUOTE)
            {
                ++pos;
                break;
            }
            if (ch == self->ESCAPE)
            {
                escaped = true;
            }
            else if (ch != self->CR && ch != self->LF)
            {
                g_string_append_c(dst, ch);
            }
        }
    }
    tokenizer_cursor_set_pos(cursor, pos);
}

GString*
tokenizer_parse_token(tokenizer self, const char* buf, tokenizer_cursor cursor, delims delims)
{
    GString* dst = g_string_new(NULL);
    bool whitespace = false;
    while (!tokenizer_cursor_at_end(cursor))
    {
        char ch = buf[tokenizer_cursor_get_pos(cursor)];
        if (delims && delims->map[(int)ch])
        {
            break;
        }
        else if (isWhitespace(self, ch))
        {
            tokenizer_skip_whitespace(self, buf, cursor);
            whitespace = true;
        }
        else
        {
            if (whitespace && dst->len > 0)
            {
                g_string_append_c(dst, ' ');
            }
            copy_content(self, buf, cursor, delims, dst);
            whitespace = false;
        }
    }
    return dst;
}

GString*
tokenizer_parse_value(tokenizer self, const char* buf, tokenizer_cursor cursor, delims delims)
{
    GString* dst = g_string_new(NULL);
    bool whitespace = false;
    while (!tokenizer_cursor_at_end(cursor))
    {
        char ch = buf[tokenizer_cursor_get_pos(cursor)];
        if (delims && delims->map[(int)ch])
        {
            break;
        }
        else if (isWhitespace(self, ch))
        {
            tokenizer_skip_whitespace(self, buf, cursor);
            whitespace = true;
        }
        else if (ch == self->DQUOTE)
        {
            if (whitespace && dst->len > 0)
            {
                g_string_append_c(dst, ' ');
            }
            copy_quoted_content(self, buf, cursor, dst);
            whitespace = false;
        }
        else
        {
            if (whitespace && dst->len > 0)
            {
                g_string_append_c(dst, ' ');
            }
            copy_unquoted_content(self, buf, cursor, delims, dst);
            whitespace = false;
        }
    }
    return dst;
}
