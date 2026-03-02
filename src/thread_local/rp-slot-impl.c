/*
 * rp-slot-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_slot_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_slot_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "thread_local/rp-thread-local-impl.h"
#include "thread_local/rp-slot-impl.h"

extern __thread RpThreadLocalData thread_local_data_;

struct _RpSlotImpl {
    GObject parent_instance;

    GWeakRef m_parent_ref;
    guint32 m_index;
};

static void slot_iface_init(RpSlotInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpSlotImpl, rp_slot_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_SLOT, slot_iface_init)
)

static inline RpThreadLocalObjectSharedPtr
get_worker(guint32 index)
{
    NOISY_MSG_("(%u) %p", index, g_ptr_array_index(thread_local_data_.m_data, index));
    return g_ptr_array_index(thread_local_data_.m_data, index);
}

typedef struct _RpDataCallbackCbCtx RpDataCallbackCbCtx;
struct _RpDataCallbackCbCtx {
    guint32 index;
    RpUpdateCb cb;
    gpointer arg;
};

static inline RpDataCallbackCbCtx
rp_data_callback_cb_ctx_ctor(guint32 index, RpUpdateCb cb, gpointer arg)
{
    RpDataCallbackCbCtx self = {
        .index = index,
        .cb = cb,
        .arg = arg
    };
    return self;
}

static inline RpDataCallbackCbCtx*
rp_data_callback_cb_ctx_new(guint32 index, RpUpdateCb cb, gpointer arg)
{
    NOISY_MSG_("(%u, %p, %p)", index, cb, arg);
    RpDataCallbackCbCtx* self = g_new(RpDataCallbackCbCtx, 1);
    *self = rp_data_callback_cb_ctx_ctor(index, cb, arg);
    return self;
}

static void
rp_data_callback_cb_ctx_free(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    g_free(arg);
}

static void
data_callback_cb(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    RpDataCallbackCbCtx* ctx = arg;
    ctx->cb(get_worker(ctx->index), ctx->arg);
}

static inline void
set_thread_local(guint32 index, RpThreadLocalObjectSharedPtr obj)
{
    NOISY_MSG_("(%u, %p)", index, obj);
    while (thread_local_data_.m_data->len <= index)
    {
        NOISY_MSG_("expanding array");
        g_ptr_array_add(thread_local_data_.m_data, NULL);
    }
    thread_local_data_.m_data->pdata[index] = obj;
}

typedef struct _RpSetCbCtx RpSetCbCtx;
struct _RpSetCbCtx {
    guint index;
    RpDispatcher* dispatcher;
    RpInitializeCb cb;
    gpointer arg;
};

static inline RpSetCbCtx
rp_set_cb_ctx_ctor(guint index, RpDispatcher* dispatcher, RpInitializeCb cb, gpointer arg)
{
    RpSetCbCtx self = {
        .index = index,
        .dispatcher = dispatcher,
        .cb = cb,
        .arg = arg
    };
    return self;
}

static inline RpSetCbCtx*
rp_set_cb_ctx_new(guint index, RpDispatcher* dispatcher, RpInitializeCb cb, gpointer arg)
{
    RpSetCbCtx* self = g_new0(RpSetCbCtx, 1);
    *self = rp_set_cb_ctx_ctor(index, dispatcher, cb, arg);
    return self;
}

static void
set_cb(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    const RpSetCbCtx* ctx = arg;
    set_thread_local(ctx->index, ctx->cb(ctx->dispatcher, ctx->arg));
    g_free((gpointer)arg);
}

typedef struct _RpRemoveSlotCbCtx RpRemoveSlotCbCtx;
struct _RpRemoveSlotCbCtx {
    guint32 index;
    RpThreadLocalInstanceImpl* parent;
};

static inline RpRemoveSlotCbCtx
rp_remove_slot_cb_ctx_ctor(guint32 index, RpThreadLocalInstanceImpl* parent)
{
    RpRemoveSlotCbCtx self = {
        .index = index,
        .parent = parent
    };
    return self;
}

static inline RpRemoveSlotCbCtx*
rp_remove_slot_cb_ctx_new(guint32 index, RpThreadLocalInstanceImpl* parent)
{
    RpRemoveSlotCbCtx* self = g_new0(RpRemoveSlotCbCtx, 1);
    *self = rp_remove_slot_cb_ctx_ctor(index, parent);
    return self;
}

static void
remove_slot_cb(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    const RpRemoveSlotCbCtx* ctx = arg;
    rp_thread_local_instance_impl_remove_slot(ctx->parent, ctx->index);
    g_free((gpointer)ctx);
}

static inline bool
is_shutdown_impl(RpSlotImpl* self)
{
    NOISY_MSG_("(%p)", self);
    g_autoptr(RpThreadLocalInstanceImpl) parent = g_weak_ref_get(&self->m_parent_ref);
    return parent ? rp_thread_local_instance_impl_shutdown_(parent) : true;
}

static inline bool
current_thread_registered_worker(guint32 index)
{
    NOISY_MSG_("(%u)", index);
    return thread_local_data_.m_data->len > index;
}

static bool
current_thread_registered_i(RpSlot* self)
{
    NOISY_MSG_("(%p)", self);
    return current_thread_registered_worker(RP_SLOT_IMPL(self)->m_index);
}

static RpThreadLocalObjectSharedPtr
get_i(RpSlot* self)
{
    NOISY_MSG_("(%p)", self);
    return get_worker(RP_SLOT_IMPL(self)->m_index);
}

static void
run_on_all_threads_completed_i(RpSlot* self, RpUpdateCb cb, void (*complete_cb)(gpointer), gpointer arg)
{
    NOISY_MSG_("(%p, %p, %p, %p)", self, cb, complete_cb, arg);
    RpSlotImpl* me = RP_SLOT_IMPL(self);
    g_autoptr(RpThreadLocalInstanceImpl) parent = g_weak_ref_get(&me->m_parent_ref);
    RpDataCallbackCbCtx* ctx = rp_data_callback_cb_ctx_new(me->m_index, cb, arg);
    rp_thread_local_instance_impl_run_on_all_threads_completed(parent, data_callback_cb, ctx, complete_cb, arg, (GDestroyNotify)rp_data_callback_cb_ctx_free);
}

static void
run_on_all_threads_i(RpSlot* self, RpUpdateCb cb, gpointer arg)
{
    NOISY_MSG_("(%p, %p, %p)", self, cb, arg);
    RpSlotImpl* me = RP_SLOT_IMPL(self);
    g_autoptr(RpThreadLocalInstanceImpl) parent = g_weak_ref_get(&me->m_parent_ref);
    RpDataCallbackCbCtx* ctx = rp_data_callback_cb_ctx_new(me->m_index, cb, arg);
    rp_thread_local_instance_impl_run_on_all_threads(parent, data_callback_cb, ctx);
}

static void
set_i(RpSlot* self, RpInitializeCb cb, gpointer arg)
{
    NOISY_MSG_("(%p, %p, %p)", self, cb, arg);

    RpSlotImpl* me = RP_SLOT_IMPL(self);
    g_autoptr(RpThreadLocalInstanceImpl) parent = g_weak_ref_get(&me->m_parent_ref);
    guint index_ = me->m_index;
    GList* itr = rp_thread_local_instance_impl_registered_threads_(parent);
    while (itr)
    {
        RpDispatcher* dispatcher = itr->data;
        RpSetCbCtx* ctx = rp_set_cb_ctx_new(index_, dispatcher, cb, arg);
        rp_dispatcher_base_post(RP_DISPATCHER_BASE(dispatcher), set_cb, ctx);
        itr = itr->next;
    }

    // Handle main thread.
    set_thread_local(index_, cb(rp_thread_local_instance_impl_dispatcher_(parent), arg));
}

static bool
is_shutdown_i(RpSlot* self)
{
    NOISY_MSG_("(%p)", self);
    return is_shutdown_impl(RP_SLOT_IMPL(self));
}

static void
slot_iface_init(RpSlotInterface* iface)
{
    LOGD("(%p)", iface);
    iface->current_thread_registered = current_thread_registered_i;
    iface->get = get_i;
    iface->run_on_all_threads_completed = run_on_all_threads_completed_i;
    iface->run_on_all_threads = run_on_all_threads_i;
    iface->set = set_i;
    iface->is_shutdown = is_shutdown_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_slot_impl_parent_class)->dispose(obj);

    RpSlotImpl* self = RP_SLOT_IMPL(obj);
    g_autoptr(RpThreadLocalInstanceImpl) parent = g_weak_ref_get(&self->m_parent_ref);
    bool is_shutdown = is_shutdown_impl(self);

    g_weak_ref_clear(&self->m_parent_ref);

    if (is_shutdown)
    {
        NOISY_MSG_("shut down");
        return;
    }

    if (!parent)
    {
        NOISY_MSG_("parent is null");
        return;
    }

    RpDispatcher* main_thread_dispatcher = rp_thread_local_instance_impl_main_thread_dispatcher_(parent);
    if (!main_thread_dispatcher || rp_dispatcher_base_is_thread_safe(RP_DISPATCHER_BASE(main_thread_dispatcher)))
    {
        rp_thread_local_instance_impl_remove_slot(parent, self->m_index);
    }
    else
    {
        RpRemoveSlotCbCtx* ctx = rp_remove_slot_cb_ctx_new(self->m_index, parent);
        rp_dispatcher_base_post(RP_DISPATCHER_BASE(main_thread_dispatcher), remove_slot_cb, ctx);
    }
}

static void
rp_slot_impl_class_init(RpSlotImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_slot_impl_init(RpSlotImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpSlotPtr
rp_slot_impl_new(RpThreadLocalInstanceImpl* parent, guint32 index)
{
    LOGD("(%p, %u)", parent, index);

    g_return_val_if_fail(RP_IS_THREAD_LOCAL_INSTANCE_IMPL(parent), NULL);

    RpSlotImpl* self = g_object_new(RP_TYPE_SLOT_IMPL, NULL);
    g_weak_ref_init(&self->m_parent_ref, parent);
    self->m_index = index;
    return RP_SLOT(self);
}
