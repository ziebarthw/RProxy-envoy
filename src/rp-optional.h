/*
 * rp-optional.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define RP_OPTIONAL_TYPE(type) \
    typedef struct { bool has_value; type value; } RpOptional_##type; \
static inline RpOptional_##type \
RpOptional_ ##type## _init(void) \
{ \
    return (RpOptional_##type){ .has_value = false }; \
} \
static inline RpOptional_##type \
RpOptional_ ##type## _ctor( ##type value) \
{ \
    return (RpOptional_##type){ .has_value = true, .value = (value) }; \
}

/*
Example usage:

{
    RP_OPTIONAL_TYPE(int);
    RpOptional_int x = RpOptional_int_ctor();
}

...

RP_OPTIONAL_TYPE(guint64);

RpOptional_guint64
test_guint64_optional_rval(bool v)
{
    return v ? (RpOptional_guint64){ .has_value = true, .value = 124 } :
                RpOptional_guint64_ctor();
}
*/

RP_OPTIONAL_TYPE(bool);
RP_OPTIONAL_TYPE(gboolean);
RP_OPTIONAL_TYPE(int);
RP_OPTIONAL_TYPE(gint);
RP_OPTIONAL_TYPE(gint16);
RP_OPTIONAL_TYPE(gint32);
RP_OPTIONAL_TYPE(gint64);
RP_OPTIONAL_TYPE(long);

G_END_DECLS
