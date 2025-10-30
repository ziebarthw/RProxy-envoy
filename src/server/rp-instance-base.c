/*
 * rp-instance-base.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_instance_base_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_instance_base_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "local_info/rp-local-info-impl.h"
#include "upstream/rp-cluster-manager-impl.h"
#include "server/rp-server-impl.h"
#include "server/rp-instance-base.h"

typedef struct _RpInstanceBasePrivate RpInstanceBasePrivate;
struct _RpInstanceBasePrivate {

    RpLocalInfo* m_local_info;
    RpClusterManagerFactory* m_cluster_manager_factory;
    RpServerFactoryContextImpl* m_server_contexts;

    const rproxy_hooks_t* m_hooks;

    bool m_terminated : 1;
    bool m_enable_reuse_port_default : 1;
};

enum
{
    PROP_0, // Reserved.
    PROP_HOOKS,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void instance_iface_init(RpInstanceInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpInstanceBase, rp_instance_base, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpInstanceBase)
    G_IMPLEMENT_INTERFACE(RP_TYPE_INSTANCE, instance_iface_init)
)

#define PRIV(obj) \
    ((RpInstanceBasePrivate*) rp_instance_base_get_instance_private(RP_INSTANCE_BASE(obj)))

static void
run_i(RpInstance* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static bool
enable_reuse_port_default_i(RpInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_enable_reuse_port_default;
}

static RpServerFactoryContext*
server_factory_context_i(RpInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_SERVER_FACTORY_CONTEXT(PRIV(self)->m_server_contexts);
}

static RpTransportSocketFactoryContext*
transport_socket_factory_context_i(RpInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_TRANSPORT_SOCKET_FACTORY_CONTEXT(PRIV(self)->m_server_contexts);
}

static RpClusterManager*
cluster_manager_i(RpInstance* self)
{
    NOISY_MSG_("(%p)", self);
//TODO...return *config_->clusterManager();
return NULL;
}

static RpLocalInfo*
local_info_i(RpInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_local_info;
}

static const rproxy_hooks_t*
hooks_i(RpInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_hooks;
}

static void
instance_iface_init(RpInstanceInterface* iface)
{
    LOGD("(%p)", iface);
    iface->run = run_i;
    iface->enable_reuse_port_default = enable_reuse_port_default_i;
    iface->server_factory_context = server_factory_context_i;
    iface->transport_socket_factory_context = transport_socket_factory_context_i;
    iface->cluster_manager = cluster_manager_i;
    iface->local_info = local_info_i;
    iface->hooks = hooks_i;
}

OVERRIDE void
set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_HOOKS:
            PRIV(obj)->m_hooks = g_value_get_pointer(value);
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

    G_OBJECT_CLASS(rp_instance_base_parent_class)->constructed(obj);

    RpInstanceBase* self = RP_INSTANCE_BASE(obj);
    RpInstanceBasePrivate* me = PRIV(self);
    // Implements both ServerFactoryContext and TransportSocketFactoryContext
    // interfaces.
    me->m_server_contexts = rp_server_factory_context_impl_new(RP_INSTANCE(obj));
}

OVERRIDE void
dispose(GObject* object)
{
    LOGD("(%p)", object);
    G_OBJECT_CLASS(rp_instance_base_parent_class)->dispose(object);
}

static void
rp_instance_base_class_init(RpInstanceBaseClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    obj_properties[PROP_HOOKS] = g_param_spec_pointer("hooks",
                                                    "Hooks",
                                                    "rproxy_hooks_t instance",
                                                    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_instance_base_init(RpInstanceBase* self)
{
    LOGD("(%p)", self);

    RpInstanceBasePrivate* me = PRIV(self);
    me->m_enable_reuse_port_default = true;
}

void
rp_instance_base_initialize(RpInstanceBase* self)
{
    LOGD("(%p)", self);
    RpInstanceBasePrivate* me = PRIV(self);
    me->m_local_info = RP_LOCAL_INFO(rp_local_info_impl_new());
#if 0
    me->m_cluster_manager_factory = RP_CLUSTER_MANAGER_FACTORY(
        rp_prod_cluster_manager_factory_new(
            rp_instance_server_factory_context(RP_INSTANCE(self)), RP_INSTANCE(self), NULL));
#endif
}
