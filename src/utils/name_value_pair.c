/*
 * name_value_pair.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "name_value_pair.h"

void
name_value_pair_dtor(name_value_pair self)
{
    g_return_if_fail(self != NULL);
    g_clear_pointer(&self->name, g_free);
    g_clear_pointer(&self->value, g_free);
}

name_value_pair
name_value_pair_new(char* name, char* value)
{
    g_return_val_if_fail(name != NULL, NULL);
    g_return_val_if_fail(name[0], NULL);
    name_value_pair self = g_new0(struct name_value_pair_s, 1);
    *self = name_value_pair_ctor(name, value);
    return self;
}

void
name_value_pair_free(name_value_pair self)
{
    g_return_if_fail(self != NULL);
    name_value_pair_dtor(self);
    g_free(self);
}
