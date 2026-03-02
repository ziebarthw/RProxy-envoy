/*
 * rp-host-vector.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "macrologger.h"

#if (defined(rp_host_vector_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_host_vector_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-upstream.h"

struct _RpHostVector {
    gatomicrefcount ref_count;

    GPtrArray* m_host_vector; // std::vector<HostSharedPtr>
};

RpHostVector*
rp_host_vector_new(void)
{
    LOGD("()");
    RpHostVector* self = g_new0(RpHostVector, 1);
    self->m_host_vector = g_ptr_array_new_with_free_func(g_object_unref);
    g_atomic_ref_count_init(&self->ref_count);
    return self;
}

RpHostVector*
rp_host_vector_copy(const RpHostVector* src)
{
    LOGD("(%p)", src);
    g_return_val_if_fail(src, NULL);
    RpHostVector* self = rp_host_vector_new();
    for (guint i = 0; i < src->m_host_vector->len; ++i)
    {
        RpHost* host = g_ptr_array_index(src->m_host_vector, i);
        rp_host_vector_add(self, host);
    }
    return self;
}

static inline void
rp_host_vector_free(RpHostVector* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(self != NULL);
    g_clear_pointer(&self->m_host_vector, g_ptr_array_unref);
    g_free(self);
}

RpHostVector*
rp_host_vector_ref(const RpHostVector* self)
{
    LOGD("(%p(%u))", self, self->ref_count);
    g_return_val_if_fail(self != NULL, NULL);
    g_atomic_ref_count_inc((gatomicrefcount*)&self->ref_count);
    return (RpHostVector*)self;
}

void
rp_host_vector_unref(const RpHostVector* self)
{
    LOGD("(%p(%u))", self, self->ref_count);
    g_return_if_fail(self != NULL);
    if (g_atomic_ref_count_dec((gatomicrefcount*)&self->ref_count))
    {
        NOISY_MSG_("freeing %p", self);
        rp_host_vector_free((RpHostVector*)self);
    }
}

gint
rp_host_vector_ref_count(const RpHostVector* self)
{
    LOGD("(%p)", self);
    return self ? self->ref_count : 0;
}

void
rp_host_vector_add_take(RpHostVector* self, RpHost* host)
{
    LOGD("(%p, %p)", self, host);
    g_return_if_fail(self != NULL);
    g_return_if_fail(RP_IS_HOST(host));
    // self must unref elements; this transfers ownership of hosts's current ref into self
    g_ptr_array_add(self->m_host_vector, host);
}

void
rp_host_vector_add(RpHostVector* self, RpHost* host)
{
    LOGD("(%p, %p)", self, host);
    g_return_if_fail(self != NULL);
    g_return_if_fail(RP_IS_HOST(host));
    rp_host_vector_add_take(self, g_object_ref(host));
}

bool
rp_host_vector_is_empty(const RpHostVector* self)
{
    return !self || !self->m_host_vector->len;
}

RpHost*
rp_host_vector_get(const RpHostVector* self, guint index)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(index < self->m_host_vector->len, NULL);
    return g_ptr_array_index(self->m_host_vector, index); // Borrowed.
}

RpHost*
rp_host_vector_dup(const RpHostVector* self, guint index)
{
    RpHost* host = rp_host_vector_get(self, index);
    return host ? g_object_ref(host) : NULL;              // Owned.
}

guint
rp_host_vector_len(const RpHostVector* self)
{
    return self ? self->m_host_vector->len : 0;
}
