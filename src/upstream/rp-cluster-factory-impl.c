/*
 * rp-cluster-factory-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_cluster_factory_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_factory_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-cluster-factory-impl.h"

typedef struct _RpClusterFactoryImplBasePrivate RpClusterFactoryImplBasePrivate;
struct _RpClusterFactoryImplBasePrivate {

    char* m_name;

};

enum
{
    PROP_0, // Reserved.
    PROP_NAME,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void cluster_factory_iface_init(RpClusterFactoryInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpClusterFactoryImplBase, rp_cluster_factory_impl_base, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpClusterFactoryImplBase)
    G_IMPLEMENT_INTERFACE(RP_TYPE_CLUSTER_FACTORY, cluster_factory_iface_init)
)

#define PRIV(obj) \
    ((RpClusterFactoryImplBasePrivate*) rp_cluster_factory_impl_base_get_instance_private(RP_CLUSTER_FACTORY_IMPL_BASE(obj)))

static RpCluster*
create_i(RpClusterFactory* self, RpClusterCfg* cluster, RpClusterFactoryContext* context)
{
    NOISY_MSG_("(%p, %p, %p)", self, cluster, context);

NOISY_MSG_("name \"%s\"", cluster->name);

    RpClusterFactoryImplBase* me = RP_CLUSTER_FACTORY_IMPL_BASE(self);
    RpClusterImplBase* cluster_ = RP_CLUSTER_FACTORY_IMPL_BASE_GET_CLASS(me)->create_cluster_impl(me, cluster, context);
    return RP_CLUSTER(cluster_);
}

static void
cluster_factory_iface_init(RpClusterFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create = create_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    LOGD("(%p, %u, %p, %p)", obj, prop_id, value, pspec);
    switch (prop_id)
    {
        case PROP_NAME:
            g_value_set_string(value, g_strdup(PRIV(obj)->m_name));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    LOGD("(%p, %u, %p, %p)", obj, prop_id, value, pspec);
    switch (prop_id)
    {
        case PROP_NAME:
            g_clear_pointer(&PRIV(obj)->m_name, g_free);
            PRIV(obj)->m_name = g_value_dup_string(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
dispose(GObject* object)
{
    LOGD("(%p)", object);
    G_OBJECT_CLASS(rp_cluster_factory_impl_base_parent_class)->dispose(object);
}

static void
rp_cluster_factory_impl_base_class_init(RpClusterFactoryImplBaseClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_NAME] = g_param_spec_string("name",
                                                    "Name",
                                                    "Cluster Name",
                                                    NULL,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_cluster_factory_impl_base_init(RpClusterFactoryImplBase* self G_GNUC_UNUSED)
{
    LOGD("(%p)", self);
}
