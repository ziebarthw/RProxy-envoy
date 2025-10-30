/*
 * header_element.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <glib-object.h>
#include "name_value_pair.h"

G_BEGIN_DECLS

typedef struct header_element_s * header_element;
struct header_element_s {
    char* name;
    char* value;
    GList* parameters;
};

static inline struct header_element_s
header_element_ctor(char* name, char* value, GList* parameters)
{
    struct header_element_s self = {
        .name = name,
        .value = value,
        .parameters = parameters
    };
    return self;
}

void header_element_dtor(header_element self);

static inline char*
header_element_get_name(header_element self)
{
    return self ? self->name : NULL;
}

static inline char*
header_element_get_value(header_element self)
{
    return self ? self->value : NULL;
}

static inline GList*
header_element_get_parameters(header_element self)
{
    return self ? self->parameters : NULL;
}

static inline int
header_element_get_parameter_count(header_element self)
{
    return self ? g_list_length(self->parameters) : 0;
}

static inline name_value_pair
header_element_get_parameter(header_element self, int index)
{
    return self && index < g_list_length(self->parameters) ?
            (name_value_pair)g_list_nth_data(self->parameters, index) : NULL;
}

name_value_pair header_element_get_parameter_by_name(header_element self,
                                                        const char* name);

G_END_DECLS
