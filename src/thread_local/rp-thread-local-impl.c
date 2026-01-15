/*
 * rp-thread-local-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_thread_local_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_thread_local_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "event/rp-dispatcher-impl.h"
#include "thread_local/rp-cb-guard.h"
#include "thread_local/rp-slot-impl.h"
#include "thread_local/rp-thread-local-impl.h"

__thread RpThreadLocalData thread_local_data_;

struct _RpThreadLocalInstanceImpl {
    GObject parent_instance;

    UNIQUE_PTR(GPtrArray) m_slots;
    UNIQUE_PTR(GArray) m_free_slot_indexes;
    UNIQUE_PTR(GList) m_registerd_threads;
    SHARED_PTR(RpDispatcher) m_main_thread_dispatcher;
    GMutex m_register_lock;
    volatile gint m_shutdown;
};

static void slot_allocator_iface_init(RpSlotAllocatorInterface* iface);
static void thread_local_instance_iface_init(RpThreadLocalInstanceInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpThreadLocalInstanceImpl, rp_thread_local_instance_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_SLOT_ALLOCATOR, slot_allocator_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_THREAD_LOCAL_INSTANCE, thread_local_instance_iface_init)
)

static bool
is_shutdown(RpThreadLocalInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return g_atomic_int_get(&RP_THREAD_LOCAL_INSTANCE_IMPL(self)->m_shutdown) != 0;
}

static void
remove_cb(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    guint32 slot = *((guint32*)arg);
    if (slot < thread_local_data_.m_data->len)
    {
        g_clear_object(&thread_local_data_.m_data->pdata[slot]);
    }
    g_free((gpointer)arg);
}

typedef struct _RpCbGuardCbCtx RpCbGuardCbCtx;
struct _RpCbGuardCbCtx {
    GDestroyNotify destroy_cb;
    gpointer destroy_arg;
    void (*complete_cb)(gpointer);
    gpointer complete_arg;
};

static inline RpCbGuardCbCtx
rp_cb_guard_cb_ctx_ctor(GDestroyNotify destroy_cb, gpointer destroy_arg, void(*complete_cb)(gpointer), gpointer complete_arg)
{
    RpCbGuardCbCtx self = {
        .destroy_cb = destroy_cb,
        .destroy_arg = destroy_arg,
        .complete_cb = complete_cb,
        .complete_arg = complete_arg
    };
    return self;
}

static inline RpCbGuardCbCtx*
rp_cb_guard_cb_ctx_new(GDestroyNotify destroy_cb, gpointer destroy_arg, void(*complete_cb)(gpointer), gpointer complete_arg)
{
    RpCbGuardCbCtx* self = g_new(RpCbGuardCbCtx, 1);
    *self = rp_cb_guard_cb_ctx_ctor(destroy_cb, destroy_arg, complete_cb, complete_arg);
    return self;
}

// This is only called once when CbGuard is destroyed.
static void
internal_all_threads_complete_cb(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    RpCbGuardCbCtx* ctx = arg;
    ctx->complete_cb(ctx->complete_arg);
    ctx->destroy_cb(ctx->destroy_arg);
    g_free(ctx);
}

static RpSlotPtr
allocate_slot_i(RpSlotAllocator* self)
{
    NOISY_MSG_("(%p)", self);
    RpThreadLocalInstanceImpl* me = RP_THREAD_LOCAL_INSTANCE_IMPL(self);
    if (!me->m_free_slot_indexes->len) // empty()
    {
        RpSlotPtr slot = RP_SLOT(rp_slot_impl_new(me, me->m_slots->len));
        g_ptr_array_add(me->m_slots, slot);
        return slot;
    }
    guint32* idx = &g_array_index(me->m_free_slot_indexes, guint32, 0);
    me->m_free_slot_indexes = g_array_remove_index_fast(me->m_free_slot_indexes, 0);
    g_assert(*idx < me->m_slots->len);
    RpSlotPtr slot = RP_SLOT(rp_slot_impl_new(me, *idx));
    me->m_slots->pdata[*idx] = slot;
    return slot;
}

static void
slot_allocator_iface_init(RpSlotAllocatorInterface* iface)
{
    LOGD("(%p)", iface);
    iface->allocate_slot = allocate_slot_i;
}

static void
register_cb(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    thread_local_data_.m_dispatcher = RP_DISPATCHER((gpointer)arg);
    thread_local_data_.m_data = g_ptr_array_new_full(16, NULL);
}

static void
register_thread_i(RpThreadLocalInstance* self, RpDispatcher* dispatcher, bool main_thread)
{
    NOISY_MSG_("(%p, %p, %u)", self, dispatcher, main_thread);
    RpThreadLocalInstanceImpl* me = RP_THREAD_LOCAL_INSTANCE_IMPL(self);
    G_MUTEX_AUTO_LOCK(&me->m_register_lock, locker);
    if (main_thread)
    {
        me->m_main_thread_dispatcher = dispatcher;
        register_cb(dispatcher);
    }
    else
    {
        me->m_registerd_threads = g_list_append(me->m_registerd_threads, dispatcher);
#       ifdef WITH_START_WORKERS_IN_MAIN
            rp_dispatcher_base_post(RP_DISPATCHER_BASE(dispatcher), register_cb, dispatcher);
#       else
            register_cb(dispatcher);
#       endif
    }
}

static void
shutdown_global_threading_i(RpThreadLocalInstance* self)
{
    NOISY_MSG_("(%p)", self);
    g_assert(!RP_THREAD_LOCAL_INSTANCE_IMPL(self)->m_shutdown);
    g_atomic_int_set(&RP_THREAD_LOCAL_INSTANCE_IMPL(self)->m_shutdown, true);
}

static void
shutdown_thread_i(RpThreadLocalInstance* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    g_assert(RP_THREAD_LOCAL_INSTANCE_IMPL(self)->m_shutdown);
    NOISY_MSG_("%u elements", thread_local_data_.m_data->len);
    for (gint i=thread_local_data_.m_data->len - 1; i >= 0; --i)
    {
        NOISY_MSG_("%i) obj %p, %u", i, thread_local_data_.m_data->pdata[i], RP_IS_THREAD_LOCAL_OBJECT(thread_local_data_.m_data->pdata[i]));
        g_clear_object(&thread_local_data_.m_data->pdata[i]);
    }
    g_ptr_array_free(g_steal_pointer(&thread_local_data_.m_data), true);
}

static RpDispatcher*
dispatcher_i(RpThreadLocalInstance* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    g_assert(thread_local_data_.m_dispatcher != NULL);
    return thread_local_data_.m_dispatcher;
}

static void
thread_local_instance_iface_init(RpThreadLocalInstanceInterface* iface)
{
    LOGD("(%p)", iface);
    iface->register_thread = register_thread_i;
    iface->is_shutdown = is_shutdown;
    iface->shutdown_global_threading = shutdown_global_threading_i;
    iface->shutdown_thread = shutdown_thread_i;
    iface->dispatcher = dispatcher_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpThreadLocalInstanceImpl* self = RP_THREAD_LOCAL_INSTANCE_IMPL(obj);
    g_assert(self->m_shutdown);
    g_array_free(g_steal_pointer(&self->m_free_slot_indexes), true);
    g_ptr_array_free(g_steal_pointer(&self->m_slots), true);

    G_OBJECT_CLASS(rp_thread_local_instance_impl_parent_class)->dispose(obj);
}

static void
rp_thread_local_instance_impl_class_init(RpThreadLocalInstanceImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_thread_local_instance_impl_init(RpThreadLocalInstanceImpl* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_free_slot_indexes = g_array_new(false, true, sizeof(guint32));
    self->m_slots = g_ptr_array_new_full(16, g_object_unref);
    g_mutex_init(&self->m_register_lock);
}

RpThreadLocalInstanceImpl*
rp_thread_local_instance_impl_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_THREAD_LOCAL_INSTANCE_IMPL, NULL);
}

void
rp_thread_local_instance_impl_run_on_all_threads(RpThreadLocalInstanceImpl* self, void (*cb)(gpointer), gpointer arg)
{
    LOGD("(%p, %p, %p)", self, cb, arg);

    g_return_if_fail(RP_IS_THREAD_LOCAL_INSTANCE_IMPL(self));
    g_return_if_fail(cb != NULL);

    for (GList* itr = self->m_registerd_threads; itr; itr = itr->next)
    {
        RpDispatcherBase* dispatcher = RP_DISPATCHER_BASE(itr->data);
        rp_dispatcher_base_post(dispatcher, cb, arg);
    }

    // Handle main thread.
    cb(arg);
}

void
rp_thread_local_instance_impl_run_on_all_threads_completed(RpThreadLocalInstanceImpl* self, void (*cb)(gpointer), gpointer cb_arg,
                                                            void (*all_threads_complete_cb)(gpointer), gpointer all_threads_complete_arg,
                                                            GDestroyNotify destroy_cb)
{
    LOGD("(%p, %p, %p, %p, %p)", self, cb, cb_arg, all_threads_complete_cb, all_threads_complete_arg);

    g_return_if_fail(RP_IS_THREAD_LOCAL_INSTANCE_IMPL(self));
    g_return_if_fail(cb != NULL);
    g_return_if_fail(all_threads_complete_cb != NULL);

    cb(cb_arg);

//TODO...std::shared_ptr<std::function<void()>> cb_guard(...)
    RpCbGuardCbCtx* ctx = rp_cb_guard_cb_ctx_new(destroy_cb, cb_arg, all_threads_complete_cb, all_threads_complete_arg);
    RpCbGuard* cb_guard = rp_cb_guard_new(self, cb, cb_arg, internal_all_threads_complete_cb, ctx);

    for (GList* itr = self->m_registerd_threads; itr; itr = itr->next)
    {
        RpDispatcherBase* dispatcher = RP_DISPATCHER_BASE(itr->data);
        rp_dispatcher_base_post(dispatcher, rp_cb_guard_cb, cb_guard);
    }
}

void
rp_thread_local_instance_impl_remove_slot(RpThreadLocalInstanceImpl* self, guint32 index)
{
    LOGD("(%p, %u)", self, index);
    g_return_if_fail(RP_IS_THREAD_LOCAL_INSTANCE_IMPL(self));
    if (g_atomic_int_get(&self->m_shutdown))
    {
        NOISY_MSG_("shut down");
        return;
    }

    g_clear_object(&self->m_slots->pdata[index]);
//    self->m_slots->pdata[index] = NULL;
    self->m_free_slot_indexes = g_array_append_val(self->m_free_slot_indexes, index);
    rp_thread_local_instance_impl_run_on_all_threads(self, remove_cb, g_memdup2(&index, sizeof(index)));
}

GList*
rp_thread_local_instance_impl_registered_threads_(RpThreadLocalInstanceImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_THREAD_LOCAL_INSTANCE_IMPL(self), NULL);
    return self->m_registerd_threads;
}

RpDispatcher*
rp_thread_local_instance_impl_dispatcher_(RpThreadLocalInstanceImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_THREAD_LOCAL_INSTANCE_IMPL(self), NULL);
    return self->m_main_thread_dispatcher;
}

RpDispatcher*
rp_thread_local_instance_impl_main_thread_dispatcher_(RpThreadLocalInstanceImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_THREAD_LOCAL_INSTANCE_IMPL(self), NULL);
    return self->m_main_thread_dispatcher;
}

bool
rp_thread_local_instance_impl_shutdown_(RpThreadLocalInstanceImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_THREAD_LOCAL_INSTANCE_IMPL(self), true);
    return g_atomic_int_get(&self->m_shutdown) != 0;
}

RpDispatcher*
rp_thread_local_instance_impl_get_dispatcher(void)
{
    NOISY_MSG_("()");
    g_assert(thread_local_data_.m_dispatcher != NULL);
    return thread_local_data_.m_dispatcher;
}
