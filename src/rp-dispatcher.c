/*
 * rp-dispatcher.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#define NO_rp_dispatcher_NOISY

#if (defined(rp_dispatcher_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_dispatcher_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "event2/watch.h"
#include "rp-dispatcher.h"

G_DEFINE_INTERFACE(RpDispatcherBase, rp_dispatcher_base, G_TYPE_OBJECT)
static void
rp_dispatcher_base_default_init(RpDispatcherBaseInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpDispatcher, rp_dispatcher, RP_TYPE_DISPATCHER_BASE)
static void
rp_dispatcher_default_init(RpDispatcherInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}


#if 0
static __thread struct evwatch* watcher_;
static __thread GSList* object_queue_;
static __thread GSList* pointer_queue_;

typedef struct _pointer_node pointer_node;
struct _pointer_node {
    gpointer mem;
    GDestroyNotify destroy_cb;
};

static void
destroy_cb(pointer_node* node)
{
    NOISY_MSG_("(%p)", node);
    node->destroy_cb(node->mem);
    g_free(node);
}

#ifdef NO_rp_dispatcher_NOISY
static void
object_unref(gpointer obj)
{
    LOGD("(%p)", obj);
    g_object_unref(obj);
}
#endif//NO_rp_dispatcher_NOISY

static void
delete_cb(struct evwatch* watcher, const struct evwatch_check_cb_info* info, void* arg)
{
    NOISY_MSG_("(%p, %p, %p)", watcher, info, arg);

    if (object_queue_)
    {
        NOISY_MSG_("object queue_ %p, %d nodes", object_queue_, g_slist_length(object_queue_));
        object_queue_ = g_slist_reverse(object_queue_);
#ifdef NO_rp_dispatcher_NOISY
        g_slist_free_full(g_steal_pointer(&object_queue_), object_unref);
#else
        g_slist_free_full(g_steal_pointer(&object_queue_), g_object_unref);
#endif//NO_rp_dispatcher_NOISY
    }

    if (pointer_queue_)
    {
        NOISY_MSG_("pointer queue_ %p, %d nodes", pointer_queue_, g_slist_length(pointer_queue_));
        pointer_queue_ = g_slist_reverse(pointer_queue_);
        g_slist_free_full(g_steal_pointer(&pointer_queue_), (GDestroyNotify)destroy_cb);
    }
}

void
rp_dispatcher_init(RpDispatcher* self)
{
    LOGD("(%p)", self);
    watcher_ = evwatch_check_new(evthr_get_base(self), delete_cb, self);
    g_assert(watcher_ != NULL);
    object_queue_ = NULL;
    pointer_queue_ = NULL;
}

void
rp_dispatcher_exit(RpDispatcher* self G_GNUC_UNUSED)
{
    LOGD("(%p)", self);
    g_clear_pointer(&watcher_, evwatch_free);
}

void
rp_dispatcher_deferred_delete(RpDispatcher* self G_GNUC_UNUSED, GObject* obj)
{
    LOGD("(%p, %p)", self, obj);
    if (obj) object_queue_ = g_slist_prepend(object_queue_, obj);
}

void
rp_dispatcher_deferred_destroy(RpDispatcher* self G_GNUC_UNUSED, gpointer mem, GDestroyNotify cb)
{
    LOGD("(%p, %p, %p)", self, mem, cb);
    if (mem)
    {
        pointer_node* node = g_new(struct _pointer_node, 1);
        node->mem = mem;
        node->destroy_cb = cb;
        pointer_queue_ = g_slist_prepend(pointer_queue_, node);
    }
}
#endif//0