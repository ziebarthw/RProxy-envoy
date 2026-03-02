/*
 * rp-host-set-ptr-vector.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_host_set_ptr_vector_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_host_set_ptr_vector_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-host-set-ptr-vector.h"

struct _RpHostSetPtrVector {
  gatomicrefcount ref_count;
  GPtrArray* m_vector; /* elements: RpHostSetImpl* (GObject implementing RP_TYPE_HOST_SET_IMPL) */
};

RpHostSetPtrVector*
rp_host_set_ptr_vector_new(void)
{
    LOGD("()");
    RpHostSetPtrVector* self = g_new0(RpHostSetPtrVector, 1);
    g_atomic_ref_count_init(&self->ref_count);
    self->m_vector = g_ptr_array_new_with_free_func(g_object_unref);
    return self;
}

RpHostSetPtrVector*
rp_host_set_ptr_vector_ref(RpHostSetPtrVector* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(self != NULL, NULL);
    g_atomic_ref_count_inc(&self->ref_count);
    return self;
}

void
rp_host_set_ptr_vector_unref(RpHostSetPtrVector* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(self != NULL);
    if (g_atomic_ref_count_dec(&self->ref_count))
    {
        NOISY_MSG_("freeing %p", self);
        g_ptr_array_unref(self->m_vector);
        g_free(self);
    }
}

guint
rp_host_set_ptr_vector_size(const RpHostSetPtrVector* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(self != NULL, 0);
    return self->m_vector->len;
}

void
rp_host_set_ptr_vector_ensure_size(RpHostSetPtrVector* self, guint n)
{
  g_return_if_fail(self != NULL);
  while (self->m_vector->len < n) g_ptr_array_add(self->m_vector, NULL);
}

RpHostSetPtr
rp_host_set_ptr_vector_get(const RpHostSetPtrVector* self, guint idx)
{
    LOGD("(%p, %u)", self, idx);

    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(idx < self->m_vector->len, NULL);

    RpHostSetPtr host_set = g_ptr_array_index(self->m_vector, idx);
    g_return_val_if_fail(RP_IS_HOST_SET(host_set), NULL);
    return host_set; /* borrowed */
}

RpHostSetPtr
rp_host_set_ptr_vector_dup(const RpHostSetPtrVector* self, guint idx)
{
    LOGD("(%p, %u)", self, idx);

    g_return_val_if_fail(self != NULL, NULL);

    RpHostSetPtr host_set = rp_host_set_ptr_vector_get(self, idx);
    g_return_val_if_fail(RP_IS_HOST_SET(host_set), NULL);
    return g_object_ref(host_set); /* owned */
}

/* Keep mutation private to the owner (PrioritySetImpl) */
void
rp_host_set_ptr_vector_set(RpHostSetPtrVector* self, guint idx, RpHostSetPtr host_set_owned)
{
    LOGD("(%p, %u, %p)", self, idx, host_set_owned);

    g_return_if_fail(self != NULL);
    g_return_if_fail(RP_IS_HOST_SET(host_set_owned));

    /* ensure size, then replace with proper refcounting */
    while (self->m_vector->len <= idx) g_ptr_array_add(self->m_vector, NULL);

    gpointer old = self->m_vector->pdata[idx];
    if (old) g_object_unref(old);
    self->m_vector->pdata[idx] = g_steal_pointer(&host_set_owned);
}
