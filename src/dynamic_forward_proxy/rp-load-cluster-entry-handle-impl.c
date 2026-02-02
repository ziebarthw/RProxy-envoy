/*
 * rp-load-cluster-entry-handle-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_load_cluster_entry_handle_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_load_cluster_entry_handle_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-dynamic-forward-proxy.h"

struct _RpLoadClusterEntryHandleImpl {
    GObject parent_instance;

    RpLoadClusterEntryCallbacks* m_callbacks;
    GHashTable* m_map;
    const char* m_key;
    bool m_cancelled;
};

G_DEFINE_TYPE_WITH_CODE(RpLoadClusterEntryHandleImpl, rp_load_cluster_entry_handle_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_LOAD_CLUSTER_ENTRY_HANDLE, NULL)
)

static inline void
erase(RpLoadClusterEntryHandleImpl* self)
{
    NOISY_MSG_("(%p)", self);
    g_assert(!self->m_cancelled);
    GList* callbacks = g_hash_table_lookup(self->m_map, self->m_key);
    callbacks = g_list_remove(callbacks, self);
    if (!callbacks)
    {
        g_hash_table_remove(self->m_map, self->m_key);
    }
    self->m_cancelled = true;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpLoadClusterEntryHandleImpl* self = RP_LOAD_CLUSTER_ENTRY_HANDLE_IMPL(obj);
    if (!self->m_cancelled)
    {
        erase(self);
    }

    G_OBJECT_CLASS(rp_load_cluster_entry_handle_impl_parent_class)->dispose(obj);
}

static void
rp_load_cluster_entry_handle_impl_class_init(RpLoadClusterEntryHandleImplClass* klass)
{
    LOGD("(%p)", klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_load_cluster_entry_handle_impl_init(RpLoadClusterEntryHandleImpl* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_cancelled = false;
}

static inline RpLoadClusterEntryHandleImpl*
constructed(RpLoadClusterEntryHandleImpl* self)
{
    NOISY_MSG_("(%p)", self);

    GList* clusters = g_hash_table_lookup(self->m_map, self->m_key);
    clusters = g_list_prepend(clusters, self);
    g_hash_table_replace(self->m_map, g_strdup(self->m_key), clusters);
    return self;
}

RpLoadClusterEntryHandleImpl*
rp_load_cluster_entry_handle_impl_new(GHashTable* parent, const char* host, RpLoadClusterEntryCallbacks* callbacks)
{
    LOGD("(%p, %p(%s), %p)", parent, host, host, callbacks);

    g_return_val_if_fail(parent != NULL, NULL);
    g_return_val_if_fail(host != NULL, NULL);
    g_return_val_if_fail(host[0], NULL);
    g_return_val_if_fail(RP_LOAD_CLUSTER_ENTRY_CALLBACKS(callbacks), NULL);

    RpLoadClusterEntryHandleImpl* self = g_object_new(RP_TYPE_LOAD_CLUSTER_ENTRY_HANDLE_IMPL, NULL);
    self->m_map = parent;
    self->m_key = host;
    self->m_callbacks = callbacks;
    return constructed(self);
}

void
rp_load_cluster_entry_handle_impl_cancel(RpLoadClusterEntryHandleImpl* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_LOAD_CLUSTER_ENTRY_HANDLE_IMPL(self));
    self->m_cancelled = true;
}

RpLoadClusterEntryCallbacks*
rp_load_cluster_entry_handle_impl_callbacks_(RpLoadClusterEntryHandleImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_LOAD_CLUSTER_ENTRY_HANDLE_IMPL(self), NULL);
    return self->m_callbacks;
}
