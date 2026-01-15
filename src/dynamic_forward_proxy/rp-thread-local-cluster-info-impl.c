/*
 * rp-thread-local-cluster-info-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_thread_local_cluster_info_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_thread_local_cluster_info_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-dynamic-forward-proxy.h"

struct _RpThreadLocalClusterInfo {
    GObject parent_instance;

    UNIQUE_PTR(GHashTable) m_pending_clusters;
    RpClusterUpdateCallbacksHandlePtr m_handle;
};

static void cluster_update_callbacks_iface_init(RpClusterUpdateCallbacksInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpThreadLocalClusterInfo, rp_thread_local_cluster_info, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_CLUSTER_UPDATE_CALLBACKS, cluster_update_callbacks_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_THREAD_LOCAL_OBJECT, NULL)
)

static void
on_cluster_add_or_update_i(RpClusterUpdateCallbacks* self, const char* cluster_name, RpThreadLocalClusterCommand command G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p(%s), %p)", self, cluster_name, cluster_name, command);

    RpThreadLocalClusterInfo* me = RP_THREAD_LOCAL_CLUSTER_INFO(self);
    GList* clusters = g_hash_table_lookup(me->m_pending_clusters, cluster_name);
    if (clusters)
    {
        for (GList* cluster = clusters; cluster; cluster = cluster->next)
        {
            RpLoadClusterEntryHandleImpl* handle = cluster->data;
            RpLoadClusterEntryCallbacks* callbacks = rp_load_cluster_entry_handle_impl_callbacks_(handle);
            //TODO...?handle->cancel();
            rp_load_cluster_entry_callbacks_on_load_cluster_complete(callbacks);
        }
        g_list_free_full(clusters, g_object_unref);
        g_hash_table_remove(me->m_pending_clusters, cluster_name);
    }
    else
    {
        LOGD("no pending request waiting on \"%s\"", cluster_name);
    }
}

static void
on_cluster_removal_i(RpClusterUpdateCallbacks* self G_GNUC_UNUSED, const char* name G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p(%s))", self, name, name);
}

static void
cluster_update_callbacks_iface_init(RpClusterUpdateCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_cluster_add_or_update = on_cluster_add_or_update_i;
    iface->on_cluster_removal = on_cluster_removal_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpThreadLocalClusterInfo* self = RP_THREAD_LOCAL_CLUSTER_INFO(obj);
    g_clear_pointer(&self->m_pending_clusters, g_hash_table_unref);

    G_OBJECT_CLASS(rp_thread_local_cluster_info_parent_class)->dispose(obj);
}

static void
rp_thread_local_cluster_info_class_init(RpThreadLocalClusterInfoClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_thread_local_cluster_info_init(RpThreadLocalClusterInfo* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_pending_clusters = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
}

RpThreadLocalClusterInfo*
rp_thread_local_cluster_info_new(RpProxyFilterConfig* parent)
{
    LOGD("(%p)", parent);
    RpThreadLocalClusterInfo* self = g_object_new(RP_TYPE_THREAD_LOCAL_CLUSTER_INFO, NULL);
    // Run in each worker thread.
    self->m_handle = rp_cluster_manager_add_thread_local_cluster_update_callbacks(
                        rp_proxy_filter_config_cluster_manager(parent), RP_CLUSTER_UPDATE_CALLBACKS(self));
NOISY_MSG_("self %p", self);
    return self;
}

GHashTable*
rp_thread_local_cluster_info_pending_clusters_(RpThreadLocalClusterInfo* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_THREAD_LOCAL_CLUSTER_INFO(self), NULL);
    return self->m_pending_clusters;
}
