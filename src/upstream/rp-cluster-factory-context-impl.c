/*
 * rp-cluster-factory-context-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_factory_context_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_factory_context_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-cluster-factory-context-impl.h"

struct _RpClusterFactoryContextImpl {
    GObject parent_instance;

    RpServerFactoryContext* m_server_context;
    RpClusterManager* m_cluster_manager;
    bool m_added_via_api;
};

static void cluster_factory_context_iface_init(RpClusterFactoryContextInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpClusterFactoryContextImpl, rp_cluster_factory_context_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_CLUSTER_FACTORY_CONTEXT, cluster_factory_context_iface_init)
)

static RpClusterManager*
cluster_manager_i(RpClusterFactoryContext* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_FACTORY_CONTEXT_IMPL(self)->m_cluster_manager;
}

static RpServerFactoryContext*
server_factory_context_i(RpClusterFactoryContext* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_FACTORY_CONTEXT_IMPL(self)->m_server_context;
}

static bool
added_via_api_i(RpClusterFactoryContext* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_FACTORY_CONTEXT_IMPL(self)->m_added_via_api;
}

static void
cluster_factory_context_iface_init(RpClusterFactoryContextInterface* iface)
{
    LOGD("(%p)", iface);
    iface->cluster_manager = cluster_manager_i;
    iface->server_factory_context = server_factory_context_i;
    iface->added_via_api = added_via_api_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    LOGD("(%p)", obj);

    RpClusterFactoryContextImpl* self = RP_CLUSTER_FACTORY_CONTEXT_IMPL(obj);
    g_object_unref(self->m_server_context);
    g_object_unref(self->m_cluster_manager);

    G_OBJECT_CLASS(rp_cluster_factory_context_impl_parent_class)->dispose(obj);
}

static void
rp_cluster_factory_context_impl_class_init(RpClusterFactoryContextImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_cluster_factory_context_impl_init(RpClusterFactoryContextImpl* self G_GNUC_UNUSED)
{
    LOGD("(%p)", self);
}

RpClusterFactoryContextImpl*
rp_cluster_factory_context_impl_new(RpServerFactoryContext* server_context, RpClusterManager* cm, bool added_via_api)
{
    LOGD("(%p, %p, %u)", server_context, cm, added_via_api);

    g_return_val_if_fail(RP_IS_SERVER_FACTORY_CONTEXT(server_context), NULL);
    g_return_val_if_fail(RP_IS_CLUSTER_MANAGER(cm), NULL);

    RpClusterFactoryContextImpl* self = g_object_new(RP_TYPE_CLUSTER_FACTORY_CONTEXT_IMPL, NULL);
    self->m_server_context = g_object_ref(server_context);
    self->m_cluster_manager = g_object_ref(cm);
    self->m_added_via_api = added_via_api;
    return self;
}
