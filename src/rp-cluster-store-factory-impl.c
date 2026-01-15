/*
 * rp-cluster-store-factory-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_store_factory_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_store_factory_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-cluster-store.h"

SINGLETON_MANAGER_REGISTRATION(dynamic_forward_proxy_cluster_store);

struct _RpDfpClusterStoreFactory {
    GObject parent_instance;

    RpSingletonManager* m_singleton_manager;
};

G_DEFINE_TYPE(RpDfpClusterStoreFactory, rp_dfp_cluster_store_factory, G_TYPE_OBJECT)

static RpSingletonInstanceSharedPtr
singleton_factory_cb(void)
{
    NOISY_MSG_("()");
    return RP_SINGLETON_INSTANCE(rp_dfp_cluster_store_new());
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_dfp_cluster_store_factory_parent_class)->dispose(obj);
}

static void
rp_dfp_cluster_store_factory_class_init(RpDfpClusterStoreFactoryClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_dfp_cluster_store_factory_init(RpDfpClusterStoreFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpDfpClusterStoreFactory*
rp_dfp_cluster_store_factory_new(RpSingletonManager* singleton_manager)
{
    LOGD("(%p)", singleton_manager);
    g_return_val_if_fail(RP_IS_SINGLETON_MANAGER(singleton_manager), NULL);
    RpDfpClusterStoreFactory* self = g_object_new(RP_TYPE_DFP_CLUSTER_STORE_FACTORY, NULL);
    self->m_singleton_manager = singleton_manager;
    return self;
}

RpDfpClusterStoreSharedPtr
rp_dfp_cluster_store_factory_get(RpDfpClusterStoreFactory* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_DFP_CLUSTER_STORE_FACTORY(self), NULL);
    return RP_DFP_CLUSTER_STORE(
        rp_singleton_manager_get(self->m_singleton_manager,
                                    SINGLETON_MANAGER_REGISTERED_NAME(dynamic_forward_proxy_cluster_store),
                                    singleton_factory_cb,
                                    false));
}
