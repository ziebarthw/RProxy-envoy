/*
 * rp-cluster-factory-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

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

static PairClusterSharedPtrThreadAwareLoadBalancerPtr
create_i(RpClusterFactory* self, const RpClusterCfg* cluster, RpClusterFactoryContext* context)
{
    NOISY_MSG_("(%p, %p, %p)", self, cluster, context);

    RpClusterFactoryImplBase* me = RP_CLUSTER_FACTORY_IMPL_BASE(self);
    return RP_CLUSTER_FACTORY_IMPL_BASE_GET_CLASS(me)->create_cluster_impl(me, cluster, context);
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
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
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
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
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
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_cluster_factory_impl_base_parent_class)->dispose(obj);
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
    NOISY_MSG_("(%p)", self);
}

static inline PairClusterSharedPtrThreadAwareLoadBalancerPtr
null_pair(void)
{
    return PairClusterSharedPtrThreadAwareLoadBalancerPtr_make(NULL, NULL);
}

PairClusterSharedPtrThreadAwareLoadBalancerPtr
rp_cluster_factory_impl_base_create(const RpClusterCfg* cluster, RpServerFactoryContext* server_context, RpClusterManager* cm,
                                    RpLazyCreateDnsResolver dns_resolver_fn, gpointer dns_resolver_arg, bool added_via_api)
{
    extern RpClusterFactory* default_static_cluster_factory;
    extern RpClusterFactory* default_strict_dns_cluster_factory;
    extern RpClusterFactory* default_dfp_cluster_factory;

    LOGD("(%p, %p, %p, %u)", cluster, server_context, cm, added_via_api);

    g_return_val_if_fail(cluster != NULL, null_pair());
    g_return_val_if_fail(RP_IS_SERVER_FACTORY_CONTEXT(server_context), null_pair());
    g_return_val_if_fail(RP_IS_CLUSTER_MANAGER(cm), null_pair());

    //TODO...FactoryRegistry...

    const char* cluster_name = NULL;

    RpClusterFactory* factory = NULL;
    if (rp_cluster_cfg_has_cluster_type(cluster))
    {
        NOISY_MSG_("cluster type");
        cluster_name = rp_cluster_type_cfg_name(rp_cluster_cfg_cluster_type(cluster));
NOISY_MSG_("cluster name \"%s\"", cluster_name);
        if (g_ascii_strcasecmp(cluster_name, "rproxy.clusters.dynamic_forward_proxy") == 0)
        {
            factory = default_dfp_cluster_factory;
        }
    }
    else
    {
NOISY_MSG_("no cluster type");
        switch (rp_cluster_cfg_type(cluster))
        {
            case RpDiscoveryType_STATIC:
//                cluster_name = "rproxy.cluster.static";
                factory = default_static_cluster_factory;
                break;
            case RpDiscoveryType_STRICT_DNS:
//                cluster_name = "rproxy.cluster.strict_dns";
                factory = default_strict_dns_cluster_factory;
                break;
            case RpDiscoveryType_EDS:
                LOGE("RpDiscoveryType_EDS not yet supported");
                break;
            case RpDiscoveryType_LOCAL_DNS:
                LOGE("RpDiscoveryType_LOCAL_DNS not yet supported");
                break;
            case RpDiscoveryType_ORIGINAL_DST:
                LOGE("RpDiscoveryType_ORIGINAL_DST not yet supported");
                break;
        }
    }

    if (!factory)
    {
        LOGE("no factory");
        return PairClusterSharedPtrThreadAwareLoadBalancerPtr_make(NULL, NULL);
    }


#if 0
    RpClusterFactory* factory;
    if (cluster->type == RpDiscoveryType_STRICT_DNS)
    {
        factory = default_dfp_factory;
    }
    else
    {
        factory = default_cluster_factory;
    }
#endif//0

    g_autoptr(RpClusterFactoryContext) context = RP_CLUSTER_FACTORY_CONTEXT(
        rp_cluster_factory_context_impl_new(server_context, cm, dns_resolver_fn, dns_resolver_arg, added_via_api));
    return rp_cluster_factory_create(factory, cluster, context);
}

RpNetworkDnsResolverSharedPtr
rp_cluster_factory_impl_base_select_dns_resolver(RpClusterFactoryImplBase* self, const RpClusterCfg* cluster,
                                                    RpClusterFactoryContext* context)
{
    LOGD("(%p, %p, %p)", self, cluster, context);

    g_return_val_if_fail(RP_CLUSTER_FACTORY_IMPL_BASE(self), NULL);
    g_return_val_if_fail(cluster != NULL, NULL);
    g_return_val_if_fail(RP_IS_CLUSTER_FACTORY_CONTEXT(context), NULL);

    //TODO...if (cluster.has_typed_dns_resolver_config())...
    //...return dns_resolver_factory.createDnsResolver(...)...

    return rp_cluster_factory_context_dns_resolver(context);
}
