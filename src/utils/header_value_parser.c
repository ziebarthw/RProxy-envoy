/*
 * header_value_parser.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "header_value_parser.h"

struct header_value_parser_s
header_value_parser_ctor(void)
{
    struct header_value_parser_s self = {
        .tokenizer = tokenizer_ctor(),
        .PARAM_DELIMITER = ';',
        .ELEM_DELIMITER = ',',
        .TOKEN_DELIMITER = delims_ctor(3, '=', self.PARAM_DELIMITER, self.ELEM_DELIMITER),
        .VALUE_DELIMITER = delims_ctor(2, self.PARAM_DELIMITER, self.ELEM_DELIMITER)
    };
    return self;
}

struct name_value_pair_s
header_value_parser_parse_name_value_pair(header_value_parser self, const char* buf, tokenizer_cursor cursor)
{
    GString* name = tokenizer_parse_token(&self->tokenizer, buf, cursor, &self->TOKEN_DELIMITER);
    if (tokenizer_cursor_at_end(cursor)) return name_value_pair_ctor(g_string_free_and_steal(name), NULL);
    char delim = buf[tokenizer_cursor_get_pos(cursor)];
    if (delim != '=') return name_value_pair_ctor(g_string_free_and_steal(name), NULL);
    tokenizer_cursor_set_pos(cursor, tokenizer_cursor_get_pos(cursor) + 1);
    GString* value = tokenizer_parse_value(&self->tokenizer, buf, cursor, &self->VALUE_DELIMITER);
    return name_value_pair_ctor(g_string_free_and_steal(name), g_string_free_and_steal(value));
}

GList*
header_value_parser_parse_parameters(header_value_parser self, const char* buf, tokenizer_cursor cursor)
{
    tokenizer_skip_whitespace(&self->tokenizer, buf, cursor);
    GList* params = NULL;
    while (!tokenizer_cursor_at_end(cursor))
    {
        struct name_value_pair_s param = header_value_parser_parse_name_value_pair(self, buf, cursor);
        params = g_list_append(params, name_value_pair_new(name_value_pair_get_name(&param), name_value_pair_get_value(&param)));
        if (!tokenizer_cursor_at_end(cursor))
        {
            char ch = buf[tokenizer_cursor_get_pos(cursor)];
            if (ch == self->PARAM_DELIMITER)
            {
                tokenizer_cursor_set_pos(cursor, tokenizer_cursor_get_pos(cursor) + 1);
            }
            if (ch == self->ELEM_DELIMITER)
            {
                break;
            }
        }
    }
    return params;
}

struct header_element_s
header_value_parser_parse_header_element(header_value_parser self, const char* buf, tokenizer_cursor cursor)
{
    struct name_value_pair_s nvp = header_value_parser_parse_name_value_pair(self, buf, cursor);
    GList* params = NULL;
    if (!tokenizer_cursor_at_end(cursor))
    {
        char ch = buf[tokenizer_cursor_get_pos(cursor)];
        if (ch == self->PARAM_DELIMITER || ch == self->ELEM_DELIMITER)
        {
            tokenizer_cursor_set_pos(cursor, tokenizer_cursor_get_pos(cursor) + 1);
        }
        if (ch != self->ELEM_DELIMITER)
        {
            params = header_value_parser_parse_parameters(self, buf, cursor);
        }
    }
    return header_element_ctor(name_value_pair_get_name(&nvp), name_value_pair_get_value(&nvp), params);
}

bool
header_value_parser_has_header_element(header_value_parser self, const char* buf, const char* name)
{
    struct tokenizer_cursor_s cursor = tokenizer_cursor_ctor(0, strlen(buf));
    while (!tokenizer_cursor_at_end(&cursor))
    {
        struct header_element_s element = header_value_parser_parse_header_element(self, buf, &cursor);
        char* element_name = header_element_get_name(&element);
        header_element_dtor(&element);
        if (element_name && g_ascii_strcasecmp(element_name, name) == 0)
        {
            return true;
        }
    }
    return false;
}
