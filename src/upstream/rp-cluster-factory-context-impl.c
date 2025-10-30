/*
 * rp-cluster-factory-context-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
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

enum
{
    PROP_0, // Reserved.
    PROP_SERVER_CONTEXT,
    PROP_CM,
    PROP_ADDED_VIA_API,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

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

static void
cluster_factory_context_iface_init(RpClusterFactoryContextInterface* iface)
{
    LOGD("(%p)", iface);
    iface->cluster_manager = cluster_manager_i;
    iface->server_factory_context = server_factory_context_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    LOGD("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_SERVER_CONTEXT:
            g_value_set_object(value, RP_CLUSTER_FACTORY_CONTEXT_IMPL(obj)->m_server_context);
            break;
        case PROP_CM:
            g_value_set_object(value, RP_CLUSTER_FACTORY_CONTEXT_IMPL(obj)->m_cluster_manager);
            break;
        case PROP_ADDED_VIA_API:
            g_value_set_boolean(value, RP_CLUSTER_FACTORY_CONTEXT_IMPL(obj)->m_added_via_api);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    LOGD("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_SERVER_CONTEXT:
            RP_CLUSTER_FACTORY_CONTEXT_IMPL(obj)->m_server_context = g_value_get_object(value);
            break;
        case PROP_CM:
            RP_CLUSTER_FACTORY_CONTEXT_IMPL(obj)->m_cluster_manager = g_value_get_object(value);
            break;
        case PROP_ADDED_VIA_API:
            RP_CLUSTER_FACTORY_CONTEXT_IMPL(obj)->m_added_via_api = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
constructed(GObject* obj)
{
    LOGD("(%p)", obj);

    G_OBJECT_CLASS(rp_cluster_factory_context_impl_parent_class)->constructed(obj);

    RpClusterFactoryContextImpl* self = RP_CLUSTER_FACTORY_CONTEXT_IMPL(obj);
    g_object_ref(self->m_cluster_manager);
}

OVERRIDE void
dispose(GObject* obj)
{
    LOGD("(%p)", obj);

    RpClusterFactoryContextImpl* self = RP_CLUSTER_FACTORY_CONTEXT_IMPL(obj);
    g_clear_object(&self->m_cluster_manager);

    G_OBJECT_CLASS(rp_cluster_factory_context_impl_parent_class)->dispose(obj);
}

static void
rp_cluster_factory_context_impl_class_init(RpClusterFactoryContextImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    obj_properties[PROP_SERVER_CONTEXT] = g_param_spec_object("server-context",
                                                    "Server context",
                                                    "ServerFactoryContext Instance",
                                                    RP_TYPE_SERVER_FACTORY_CONTEXT,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_CM] = g_param_spec_object("cm",
                                                    "cm",
                                                    "ClusterManager Instance",
                                                    RP_TYPE_CLUSTER_MANAGER,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_ADDED_VIA_API] = g_param_spec_boolean("added-via-api",
                                                    "Added via api",
                                                    "Added Via API Flag",
                                                    false,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
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
    return g_object_new(RP_TYPE_CLUSTER_FACTORY_CONTEXT_IMPL,
                        "server-context", server_context,
                        "cm", cm,
                        "added-via-api", added_via_api,
                        NULL);
}
