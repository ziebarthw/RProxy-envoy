/*
 * name_value_pair.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct name_value_pair_s * name_value_pair;
struct name_value_pair_s {
    char* name;
    char* value;
};

static inline struct name_value_pair_s
name_value_pair_ctor(char* name, char* value)
{
    struct name_value_pair_s self = {
        .name = name,
        .value = value
    };
    return self;
}

void name_value_pair_dtor(name_value_pair self);
name_value_pair name_value_pair_new(char* name, char* value);
void name_value_pair_free(name_value_pair self);

static inline char*
name_value_pair_get_name(name_value_pair self)
{
    return self ? self->name : NULL;
}

static inline char*
name_value_pair_get_value(name_value_pair self)
{
    return self ? self->value : NULL;
}

G_END_DECLS
