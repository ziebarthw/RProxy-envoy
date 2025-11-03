/*
 * rp-cluster-impl-base.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_cluster_impl_base_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_impl_base_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-cluster-factory.h"
#include "upstream/rp-cluster-info-impl.h"
#include "upstream/rp-cluster-impl-base.h"

typedef struct _RpClusterImplBasePrivate RpClusterImplBasePrivate;
struct _RpClusterImplBasePrivate {

    RpClusterInfoImpl* m_info;

    RpClusterCfg m_config;
    RpClusterFactoryContext* m_cluster_context;

    RpStatusCode_e (*m_initialize_complete_callback)(RpCluster*);

    bool m_wait_for_warm_on_init : 1;
    bool m_initialization_started : 1;
};

enum
{
    PROP_0, // Reserved.
    PROP_CONFIG,
    PROP_CLUSTER_CONTEXT,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void cluster_iface_init(RpClusterInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpClusterImplBase, rp_cluster_impl_base, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpClusterImplBase)
    G_IMPLEMENT_INTERFACE(RP_TYPE_CLUSTER, cluster_iface_init)
)

#define PRIV(obj) \
    ((RpClusterImplBasePrivate*) rp_cluster_impl_base_get_instance_private(RP_CLUSTER_IMPL_BASE(obj)))

static RpClusterInfo*
info_i(RpCluster* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_INFO(PRIV(self)->m_info);
}

static void
initialize_i(RpCluster* self, RpStatusCode_e (*callback)(RpCluster*))
{
    NOISY_MSG_("(%p, %p)", self, callback);
    PRIV(self)->m_initialize_complete_callback = callback;
    RP_CLUSTER_IMPL_BASE_GET_CLASS(self)->start_pre_init(RP_CLUSTER_IMPL_BASE(self));
}

static void
cluster_iface_init(RpClusterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->info = info_i;
    iface->initialize = initialize_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_CLUSTER_CONTEXT:
            g_value_set_object(value, PRIV(obj)->m_cluster_context);
            break;
        case PROP_CONFIG:
            g_value_set_pointer(value, &PRIV(obj)->m_config);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_CLUSTER_CONTEXT:
            PRIV(obj)->m_cluster_context = g_value_get_object(value);
            break;
        case PROP_CONFIG:
            PRIV(obj)->m_config = *((RpClusterCfg*)g_value_get_pointer(value));
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

    G_OBJECT_CLASS(rp_cluster_impl_base_parent_class)->constructed(obj);

    RpClusterImplBasePrivate* me = PRIV(obj);
    RpServerFactoryContext* server_context = rp_cluster_factory_context_server_factory_context(me->m_cluster_context);
    me->m_info = rp_cluster_info_impl_new(server_context,
                                            &me->m_config,
                                            false/*TODO...*/);
}

OVERRIDE void
dispose(GObject* object)
{
    LOGD("(%p)", object);
    G_OBJECT_CLASS(rp_cluster_impl_base_parent_class)->dispose(object);
}

static void
rp_cluster_impl_base_class_init(RpClusterImplBaseClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    obj_properties[PROP_CLUSTER_CONTEXT] = g_param_spec_object("cluster-context",
                                                    "Cluster context",
                                                    "ClusterFactoryContext Instance",
                                                    RP_TYPE_CLUSTER_FACTORY_CONTEXT,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_CONFIG] = g_param_spec_pointer("config",
                                                    "Config",
                                                    "Config",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_cluster_impl_base_init(RpClusterImplBase* self G_GNUC_UNUSED)
{
    LOGD("(%p)", self);
}

void
rp_cluster_impl_base_on_pre_init_complete(RpClusterImplBase* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_CLUSTER_IMPL_BASE(self));
    RpClusterImplBasePrivate* me = PRIV(self);
    if (me->m_initialization_started)
    {
        return;
    }
    me->m_initialization_started = true;

//TODO...init_manager_.initialize(init_watcher_);
}

const RpClusterCfg*
rp_cluster_impl_base_config_(RpClusterImplBase* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CLUSTER_IMPL_BASE(self), NULL);
    return &PRIV(self)->m_config;
}
