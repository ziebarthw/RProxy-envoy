/*
 * header_element.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "header_element.h"

void
header_element_dtor(header_element self)
{
    g_return_if_fail(self != NULL);
    g_clear_pointer(&self->name, g_free);
    g_clear_pointer(&self->value, g_free);
    g_list_free_full(self->parameters, (GDestroyNotify)name_value_pair_free);
    self->parameters = NULL;
}

name_value_pair
header_element_get_parameter_by_name(header_element self, const char* name)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(name != NULL, NULL);
    g_return_val_if_fail(name[0], NULL);
    for (int i=0; ; ++i)
    {
        name_value_pair param = header_element_get_parameter(self, i);
        if (!param)
            return NULL;
        else if (g_ascii_strcasecmp(name_value_pair_get_name(param), name) == 0)
            return param;
    }
}
