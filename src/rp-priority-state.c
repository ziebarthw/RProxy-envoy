/*
 * rp-priority-state.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_priority_state_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_priority_state_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-upstream.h"

struct _RpPriorityState {
    gatomicrefcount ref_count;

    GPtrArray* m_host_lists; // std::vector<HostListPtr>
};

static void
rp_host_vector_destroy_notify(gpointer p)
{
    NOISY_MSG_("(%p)", p);
    if (p)
    {
        NOISY_MSG_("calling rp_host_vector_unref(%p)", p);
        rp_host_vector_unref(p);
    }
}

RpPriorityStatePtr
rp_priority_state_new(void)
{
    LOGD("()");
    RpPriorityState* self = g_new0(RpPriorityState, 1);
    self->m_host_lists = g_ptr_array_new_with_free_func(rp_host_vector_destroy_notify);
    g_atomic_ref_count_init(&self->ref_count);
    return self;
}

static inline void
rp_priority_state_free(RpPriorityStatePtr self)
{
    LOGD("(%p)", self);
    g_return_if_fail(self != NULL);
    g_clear_pointer(&self->m_host_lists, g_ptr_array_unref);
    g_free(self);
}

void
rp_priority_state_unref(RpPriorityStatePtr self)
{
    g_return_if_fail(self != NULL);
    if (g_atomic_ref_count_dec(&self->ref_count))
    {
        NOISY_MSG_("freeing %p", self);
        rp_priority_state_free(self);
    }
}

guint
rp_priority_state_size(const RpPriorityStatePtr self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(self != NULL, 0);
    return self->m_host_lists->len;
}

void
rp_priority_state_ensure(RpPriorityStatePtr self, guint32 priority)
{
    LOGD("(%p, %u)", self, priority);

    g_return_if_fail(self);

    while (self->m_host_lists->len <= priority)
    {
        /* Add NULL slots; free_func will ignore NULL */
        g_ptr_array_add(self->m_host_lists, NULL);
    }
}

void
rp_priority_state_set_host_list(RpPriorityStatePtr self, guint32 priority, RpHostListPtr host_list)  // consumes
{
    LOGD("(%p, %u, %p)", self, priority, host_list);

    g_return_if_fail(self);

    while (self->m_host_lists->len <= priority)
    {
        g_ptr_array_add(self->m_host_lists, NULL);
    }

    RpHostVector** slot =
        (RpHostVector**)&g_ptr_array_index(self->m_host_lists, priority);

    if (*slot)
    {
        NOISY_MSG_("calling rp_host_vector_unref(%p)", *slot);
        rp_host_vector_unref(*slot);
    }

    *slot = host_list;  // ownership transferred
}

RpHostListPtr
rp_priority_state_steal_host_list(RpPriorityStatePtr self, guint32 priority)
{
    LOGD("(%p, %u)", self, priority);

    g_return_val_if_fail(self, NULL);

    if (priority >= self->m_host_lists->len)
    {
        LOGE("priority %u is out of range!", priority);
        return NULL;
    }

    RpHostVector** slot =
        (RpHostVector**)&g_ptr_array_index(self->m_host_lists, priority);

    return g_steal_pointer(slot);   // <-- this is std::move
}

RpHostListPtr
rp_priority_state_peek_host_list(RpPriorityStatePtr self, guint32 priority)
{
    LOGD("(%p, %u)", self, priority);

    g_return_val_if_fail(self, NULL);

    if (priority >= self->m_host_lists->len)
    {
        LOGE("priority %u is out of range!", priority);
        return NULL;
    }
    return (RpHostListPtr)g_ptr_array_index(self->m_host_lists, priority);
}