/*
 * rp-cb-guard.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cb_guard_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cb_guard_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "thread_local/rp-cb-guard.h"

struct _RpCbGuard {
    GObject parent_instance;

    RpThreadLocalInstanceImpl* m_parent;

    void (*m_cb)(gpointer);
    void (*m_all_threads_complete_cb)(gpointer);
    gpointer m_cb_arg;
    gpointer m_all_threads_complete_arg;
};

G_DEFINE_FINAL_TYPE(RpCbGuard, rp_cb_guard, G_TYPE_OBJECT)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpCbGuard* self = RP_CB_GUARD(obj);
    RpDispatcher* main_thread_dispatcher_ = rp_thread_local_instance_impl_main_thread_dispatcher_(self->m_parent);
    rp_dispatcher_base_post(RP_DISPATCHER_BASE(main_thread_dispatcher_),
                            self->m_all_threads_complete_cb,
                            self->m_all_threads_complete_arg);

    G_OBJECT_CLASS(rp_cb_guard_parent_class)->dispose(obj);
}

static void
rp_cb_guard_class_init(RpCbGuardClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_cb_guard_init(RpCbGuard* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpCbGuard*
rp_cb_guard_new(RpThreadLocalInstanceImpl* parent, void (*cb)(gpointer), gpointer cb_arg,
                    void (*all_threads_complete_cb)(gpointer), gpointer all_threads_complete_arg)
{
    LOGD("(%p, %p, %p, %p, %p)", parent, cb, cb_arg, all_threads_complete_cb, all_threads_complete_arg);

    g_return_val_if_fail(RP_IS_THREAD_LOCAL_INSTANCE_IMPL(parent), NULL);
    g_return_val_if_fail(all_threads_complete_cb != NULL, NULL);

    RpCbGuard* self = g_object_new(RP_TYPE_CB_GUARD, NULL);
    self->m_parent = parent;
    self->m_cb = cb;
    self->m_cb_arg = cb_arg;
    self->m_all_threads_complete_cb = all_threads_complete_cb;
    self->m_all_threads_complete_arg = all_threads_complete_arg;

    GList* itr = rp_thread_local_instance_impl_registered_threads_(parent);
    while (itr)
    {
        g_object_ref(self);
        itr = itr->next;
    }
    g_object_unref(self); // Account for the automatic setting to 1 on creation.
    return self;
}

void
rp_cb_guard_cb(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    RpCbGuard* self = RP_CB_GUARD(arg);
    self->m_cb(self->m_cb_arg);
    g_object_unref(self);
}
