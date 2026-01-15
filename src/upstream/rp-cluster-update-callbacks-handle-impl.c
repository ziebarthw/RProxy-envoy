/*
 * rp-cluster-update-callbacks-handle-impl.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_update_callbacks_handle_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_update_callbacks_handle_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-cluster-manager-impl.h"

struct _RpClusterUpdateCallbacksHandlerImpl {
    GObject parent_instance;

};

G_DEFINE_TYPE_WITH_CODE(RpClusterUpdateCallbacksHandlerImpl, rp_cluster_update_callbacks_handler_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_CLUSTER_UPDATE_CALLBACKS_HANDLE, NULL)
)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_cluster_update_callbacks_handler_impl_parent_class)->dispose(obj);
}

static void
rp_cluster_update_callbacks_handler_impl_class_init(RpClusterUpdateCallbacksHandlerImplClass* klass)
{
    LOGD("(%p)", klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_cluster_update_callbacks_handler_impl_init(RpClusterUpdateCallbacksHandlerImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpClusterUpdateCallbacksHandlerImpl*
rp_cluster_update_callbacks_handler_impl_new(RpClusterUpdateCallbacks* cb, GList** parent)
{
    LOGD("(%p, %p)", cb, parent);
    return g_object_new(RP_TYPE_CLUSTER_UPDATE_CALLBACKS_HANDLER_IMPL, NULL);
}
