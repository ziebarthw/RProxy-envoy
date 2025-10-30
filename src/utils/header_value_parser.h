/*
 * header_value_parser.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <glib-object.h>
#include "header_element.h"
#include "tokenizer.h"

G_BEGIN_DECLS

typedef struct header_value_parser_s * header_value_parser;
struct header_value_parser_s {
    struct tokenizer_s tokenizer;
    char PARAM_DELIMITER;
    char ELEM_DELIMITER;
    struct delims_s TOKEN_DELIMITER;
    struct delims_s VALUE_DELIMITER;
};

struct header_value_parser_s header_value_parser_ctor(void);
struct name_value_pair_s header_value_parser_parse_name_value_pair(header_value_parser self,
                                                                    const char* buf,
                                                                    tokenizer_cursor cursor);
GList* header_value_parser_parse_parameters(header_value_parser self,
                                            const char* buf,
                                            tokenizer_cursor cursor);
struct header_element_s header_value_parser_parse_header_element(header_value_parser self,
                                                                    const char* buf,
                                                                    tokenizer_cursor cursor);
bool header_value_parser_has_header_element(header_value_parser self,
                                            const char* buf,
                                            const char* name);

G_END_DECLS
