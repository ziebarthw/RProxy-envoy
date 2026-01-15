/*
 * rp-cluster-impl-base.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_impl_base_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_impl_base_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-cluster-factory.h"
#include "server/rp-transport-socket-config-impl.h"
#include "upstream/rp-upstream-impl.h"

typedef struct _RpClusterImplBasePrivate RpClusterImplBasePrivate;
struct _RpClusterImplBasePrivate {

    RpClusterInfoConstSharedPtr m_info;

    UNIQUE_PTR(RpMainPrioritySetImpl) m_priority_set;

    RpClusterCfg m_config;
    RpClusterFactoryContext* m_cluster_context;
    RpTransportSocketFactoryContextImplPtr m_transport_factory_context;

    RpClusterInitializeCb m_initialize_complete_callback;
    gpointer m_initialize_complete_callback_arg;

    guint64 m_pending_initialize_health_checks;

    RpStatusCode_e* m_creation_status;

    bool m_wait_for_warm_on_init : 1;
    bool m_initialization_started : 1;
};

enum
{
    PROP_0, // Reserved.
    PROP_CONFIG,
    PROP_CLUSTER_CONTEXT,
    PROP_CREATION_STATUS,
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

static void
finish_initialization(RpClusterImplBase* self)
{
    NOISY_MSG_("(%p)", self);
    RpClusterImplBasePrivate* me = PRIV(self);
    g_assert(me->m_initialize_complete_callback != NULL);
    g_assert(me->m_initialization_started);

    RpClusterInitializeCb snapped_callback = me->m_initialize_complete_callback;
    gpointer snapped_callback_arg = me->m_initialize_complete_callback_arg;
    me->m_initialize_complete_callback = NULL;
    me->m_initialize_complete_callback_arg = NULL;

    //TODO...if (health_checker_ != nullptr)...

    if (snapped_callback)
    {
        RpStatusCode_e status = snapped_callback(snapped_callback_arg);
        if (status != RpStatusCode_Ok)
        {
            LOGE("failed with status %d", status);
        }
    }
}

static void
on_init_done(RpClusterImplBase* self)
{
    NOISY_MSG_("(%p)", self);
    //TODO...init()->configUpdateStats().warming_state_.set(0);
    //TODO...if (health_checker_)...

    if (PRIV(self)->m_pending_initialize_health_checks == 0)
    {
        NOISY_MSG_("finished");
        finish_initialization(self);
    }
}

static RpClusterInfoConstSharedPtr
info_i(RpCluster* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_info;
}

static void
initialize_i(RpCluster* self, RpClusterInitializeCb cb, gpointer arg)
{
    NOISY_MSG_("(%p, %p, %p)", self, cb, arg);
    RpClusterImplBasePrivate* me = PRIV(self);
    me->m_initialize_complete_callback = cb;
    me->m_initialize_complete_callback_arg = arg;
    RP_CLUSTER_IMPL_BASE_GET_CLASS(self)->start_pre_init(RP_CLUSTER_IMPL_BASE(self));
}

static RpPrioritySet*
priority_set_i(RpCluster* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_PRIORITY_SET(PRIV(self)->m_priority_set);
}

static void
cluster_iface_init(RpClusterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->info = info_i;
    iface->initialize = initialize_i;
    iface->priority_set = priority_set_i;
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
        case PROP_CREATION_STATUS:
            g_value_set_pointer(value, &PRIV(obj)->m_creation_status);
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
        case PROP_CREATION_STATUS:
            PRIV(obj)->m_creation_status = g_value_get_pointer(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static RpClusterInfoImpl*
cluster_info_impl_create(RpClusterImplBasePrivate* me, RpServerFactoryContext* server_context)
{
    NOISY_MSG_("(%p, %p)", me, server_context);
    return rp_cluster_info_impl_new(server_context,
                                    &me->m_config,
                                    rp_cluster_factory_context_added_via_api(me->m_cluster_context));
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_cluster_impl_base_parent_class)->constructed(obj);

    RpClusterImplBasePrivate* me = PRIV(obj);
    RpServerFactoryContext* server_context = rp_cluster_factory_context_server_factory_context(me->m_cluster_context);
    me->m_transport_factory_context = rp_transport_socket_factory_context_impl_new(server_context,
                                                rp_cluster_factory_context_cluster_manager(me->m_cluster_context));
    me->m_info = RP_CLUSTER_INFO(cluster_info_impl_create(me, server_context));
    me->m_priority_set = rp_main_priority_set_impl_new();
    rp_priority_set_impl_get_or_create_host_set(RP_PRIORITY_SET_IMPL(me->m_priority_set), 0);
    *me->m_creation_status = RpStatusCode_Ok;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpClusterImplBasePrivate* me = PRIV(obj);
    g_clear_object(&me->m_priority_set);

    G_OBJECT_CLASS(rp_cluster_impl_base_parent_class)->dispose(obj);
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
    obj_properties[PROP_CREATION_STATUS] = g_param_spec_pointer("creation-status",
                                                                "Creation status",
                                                                "Creation status",
                                                                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_cluster_impl_base_init(RpClusterImplBase* self)
{
    LOGD("(%p)", self);
    PRIV(self)->m_pending_initialize_health_checks = 0;
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
on_init_done(self);
}

const RpClusterCfg*
rp_cluster_impl_base_config_(RpClusterImplBase* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CLUSTER_IMPL_BASE(self), NULL);
    return &PRIV(self)->m_config;
}
