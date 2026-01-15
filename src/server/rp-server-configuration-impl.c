/*
 * rp-server-configuration-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_server_configuration_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_server_configuration_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "server/rp-server-configuration-impl.h"

struct _RpServerConfigurationMainImpl {
    GObject parent_instance;

    UNIQUE_PTR(RpClusterManager) m_cluster_manager;
};

static void server_configuration_main_iface_init(RpServerConfigurationMainInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpServerConfigurationMainImpl, rp_server_configuration_main_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_SERVER_CONFIGURATION_MAIN, server_configuration_main_iface_init)
)

static RpClusterManager*
cluster_manager_i(RpServerConfigurationMain* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_SERVER_CONFIGURATION_MAIN_IMPL(self)->m_cluster_manager;
}

static void
server_configuration_main_iface_init(RpServerConfigurationMainInterface* iface)
{
    LOGD("(%p)", iface);
    iface->cluster_manager = cluster_manager_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_server_configuration_main_impl_parent_class)->dispose(obj);
}

static void
rp_server_configuration_main_impl_class_init(RpServerConfigurationMainImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_server_configuration_main_impl_init(RpServerConfigurationMainImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpServerConfigurationMainImpl*
rp_server_configuration_main_impl_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_SERVER_CONFIGURATION_MAIN_IMPL, NULL);
}

bool
rp_server_configuration_main_impl_initialize(RpServerConfigurationMainImpl* self, rproxy_t* bootstrap,
                                                RpServerInstance* server G_GNUC_UNUSED, RpClusterManagerFactory* cluster_manager_factory,
                                                GError** err)
{
    LOGD("(%p, %p, %p, %p, %p)", self, bootstrap, server, cluster_manager_factory, err);
    self->m_cluster_manager = rp_cluster_manager_factory_cluster_manager_from_proto(cluster_manager_factory,
                                                                                    bootstrap);

#if 0
    if (!rp_cluster_manager_initialize(self->m_cluster_manager, bootstrap, err))
    {
        GError* err_ = *err;
        g_error("error %d(%s)", err_->code, err_->message);
        return false;
    }
//TODO...
#endif//0
    return true;
}
