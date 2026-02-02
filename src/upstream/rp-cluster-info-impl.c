/*
 * rp-cluster-info-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_info_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_info_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-resource-manager-impl.h"
#include "upstream/rp-upstream-impl.h"

struct _RpClusterInfoImpl {
    GObject parent_instance;

    RpResourceManagerImpl* m_resource_manager;
    RpServerFactoryContext* m_server_context;
    RpClusterCfgPtr m_config;
    const RpCustomClusterTypeCfg* m_cluster_type;

    RpLoadBalancerConfigPtr m_load_balancer_config;
    RpTypedLoadBalancerFactory* m_load_balancer_factory;

    guint64 m_max_requests_per_connection;

    bool m_added_via_api;
};

static void filter_chain_factory_iface_init(RpFilterChainFactoryInterface* iface);
static void cluster_info_iface_init(RpClusterInfoInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpClusterInfoImpl, rp_cluster_info_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_FILTER_CHAIN_FACTORY, filter_chain_factory_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_CLUSTER_INFO, cluster_info_iface_init)
)

static bool
create_filter_chain_i(RpFilterChainFactory* self, RpFilterChainManager* manager)
{
    NOISY_MSG_("(%p, %p)", self, manager);
    //TODO...if (http_filter_factories_.empty())...
    return false;
}

static void
filter_chain_factory_iface_init(RpFilterChainFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_filter_chain = create_filter_chain_i;
}

static bool
added_via_api_i(RpClusterInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_INFO_IMPL(self)->m_added_via_api;
}

static const char*
name_i(RpClusterInfo* self)
{
    NOISY_MSG_("(%p)", self);
    const RpClusterLoadAssignmentCfg* load_assignment = rp_cluster_cfg_load_assignment(RP_CLUSTER_INFO_IMPL(self)->m_config);
    return rp_cluster_load_assignment_cfg_cluster_name(load_assignment);
}

static RpResourceManager*
resource_manager_i(RpClusterInfo* self, RpResourcePriority_e priority)
{
    NOISY_MSG_("(%p, %d)", self, priority);
    return RP_RESOURCE_MANAGER(RP_CLUSTER_INFO_IMPL(self)->m_resource_manager);
}

static float
per_upstream_preconnect_ratio_i(RpClusterInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_INFO_IMPL(self)->m_config->preconnect_policy.per_upstream_preconnect_ratio;
}

static RpUpstreamLocalAddressSelector*
get_upstream_local_address_selector_i(RpClusterInfo* self)
{
    NOISY_MSG_("(%p)", self);
//TODO...
return NULL;
}

static guint64
max_requests_per_connection_i(RpClusterInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_INFO_IMPL(self)->m_max_requests_per_connection;
}

static RpDiscoveryType_e
type_i(RpClusterInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_cluster_cfg_type(RP_CLUSTER_INFO_IMPL(self)->m_config);
}

static RpTransportSocketFactoryContextPtr
transport_socket_context_i(RpClusterInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_server_factory_context_get_transport_socket_factory_context(
            RP_CLUSTER_INFO_IMPL(self)->m_server_context);
}

static evhtp_proto*
upstream_http_protocol_i(RpClusterInfo* self, evhtp_proto downstream_protocol)
{
    NOISY_MSG_("(%p, %d)", self, downstream_protocol);
    evhtp_proto* rval = g_malloc0(sizeof(*rval));
    //TODO...features_ & USE_DOWNSTREAM_PROTOCOL
    if (downstream_protocol != EVHTP_PROTO_INVALID)
    {
        if (downstream_protocol == EVHTP_PROTO_10)
        {
            *rval = EVHTP_PROTO_11;
            return rval;
        }
        *rval = downstream_protocol;
        return rval;
    }
    //TODO...HTTP2, HTTP3, etc.
    *rval = EVHTP_PROTO_11;
    return rval;
}

static const RpCustomClusterTypeCfg*
cluster_type_i(RpClusterInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_INFO_IMPL(self)->m_cluster_type;
}

static RpLoadBalancerConfig*
load_balancer_config_i(RpClusterInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_INFO_IMPL(self)->m_load_balancer_config;
}

static RpTypedLoadBalancerFactory*
load_balancer_factory_i(RpClusterInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_INFO_IMPL(self)->m_load_balancer_factory;
}

static void
cluster_info_iface_init(RpClusterInfoInterface* iface)
{
    LOGD("(%p)", iface);
    iface->added_via_api = added_via_api_i;
    iface->name = name_i;
    iface->resource_manager = resource_manager_i;
    iface->per_upstream_preconnect_ratio = per_upstream_preconnect_ratio_i;
    iface->get_upstream_local_address_selector = get_upstream_local_address_selector_i;
    iface->max_requests_per_connection = max_requests_per_connection_i;
    iface->type = type_i;
    iface->transport_socket_context = transport_socket_context_i;
    iface->upstream_http_protocol = upstream_http_protocol_i;
    iface->cluster_type = cluster_type_i;
    iface->load_balancer_config = load_balancer_config_i;
    iface->load_balancer_factory = load_balancer_factory_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpClusterInfoImpl* self = RP_CLUSTER_INFO_IMPL(obj);
    g_clear_pointer(&self->m_config, g_free);
    g_clear_object(&self->m_resource_manager);
    g_clear_object(&self->m_load_balancer_config);
    g_clear_object(&self->m_load_balancer_factory);

    G_OBJECT_CLASS(rp_cluster_info_impl_parent_class)->dispose(obj);
}

static void
rp_cluster_info_impl_class_init(RpClusterInfoImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_cluster_info_impl_init(RpClusterInfoImpl* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_load_balancer_factory = NULL;
}

static inline bool
configure_lb_policies(RpClusterInfoImpl* self, RpClusterCfgPtr config, RpServerFactoryContext* context)
{
    NOISY_MSG_("(%p, %p, %p)", self, config, context);
    LOGE("NOT IMPLEMENTED");
    //TODO...
    return false;
}

typedef struct _Result Result;
struct _Result {
    RpTypedLoadBalancerFactory* factory;
    RpLoadBalancerConfigPtr config;
};
static inline Result
result_ctor(RpTypedLoadBalancerFactory* factory, RpLoadBalancerConfigPtr config)
{
    Result self = {
        .factory = factory,
        .config = config
    };
    return self;
}

static inline Result
get_typed_lb_config_from_legacy_proto_without_subset(RpClusterInfoImpl*self, RpServerFactoryContext* context, const RpClusterCfg* cluster)
{
    extern RpTypedLoadBalancerFactory* default_cluster_provided_lb_factory;

    NOISY_MSG_("(%p, %p, %p)", self, context, cluster);
    RpTypedLoadBalancerFactory* lb_factory = NULL;

    switch (rp_cluster_cfg_lb_policy(cluster))
    {
        case RpLbPolicy_ROUND_ROBIN:
        case RpLbPolicy_LEAST_REQUEST:
        case RpLbPolicy_RANDOM:
        case RpLbPolicy_RING_HASH:
        case RpLbPolicy_MAGLEV:
            break;
        case RpLbPolicy_CLUSTER_PROVIDED:
            lb_factory = default_cluster_provided_lb_factory;
            break;
        case RpLbPolicy_LOAD_BALANCING_POLICY_CONFIG:
            break;
    }

    if (!lb_factory)
    {
        LOGE("No load balancer factory found LB type %d", rp_cluster_cfg_lb_policy(cluster));
        return result_ctor(NULL, NULL);
    }

    RpLoadBalancerConfigPtr lb_config = rp_typed_load_balancer_factory_load_legacy(lb_factory, context, cluster);
    return result_ctor(lb_factory, lb_config);
}

static inline Result
get_typed_lb_config_from_legacy_proto(RpClusterInfoImpl* self, RpServerFactoryContext* context, const RpClusterCfg* cluster)
{
    NOISY_MSG_("(%p, %p, %p)", self, context, cluster);
    if (!rp_cluster_cfg_has_lb_subset_config(cluster)
        /*TODO... || cluster.lb_subset_config().subset_selectors().empty()*/)
    {
        return get_typed_lb_config_from_legacy_proto_without_subset(self, context, cluster);
    }
    //TODO...
    return result_ctor(NULL, NULL);
}

static inline RpClusterInfoImpl*
constructed(RpClusterInfoImpl* self)
{
    NOISY_MSG_("(%p)", self);

    RpClusterCfgPtr config = self->m_config;
    const RpClusterLoadAssignmentCfg* load_assignment = rp_cluster_cfg_load_assignment(config);
    const char* cluster_name = rp_cluster_load_assignment_cfg_cluster_name(load_assignment);
    self->m_resource_manager = rp_resource_manager_impl_new(cluster_name, 1024, 1024, 1024, 3, G_MAXUINT64, G_MAXUINT64);
    self->m_cluster_type = rp_cluster_cfg_has_cluster_type(config) ?
                            rp_cluster_cfg_cluster_type(config) : NULL;
self->m_max_requests_per_connection = 1024;//TODO...config...
    if (rp_cluster_cfg_has_load_balancing_policy(config) ||
        rp_cluster_cfg_lb_policy(config) == RpLbPolicy_LOAD_BALANCING_POLICY_CONFIG)
    {
        if (!configure_lb_policies(self, config, self->m_server_context))
        {
            LOGE("configure lb policies failed");
            g_clear_object(&self);
        }
    }
    else
    {
        Result lb_pair = get_typed_lb_config_from_legacy_proto(self, self->m_server_context, config);
        self->m_load_balancer_factory = lb_pair.factory ? g_object_ref(lb_pair.factory) : NULL;
        self->m_load_balancer_config = lb_pair.config;
        if (!self->m_load_balancer_factory || !self->m_load_balancer_config)
        {
            LOGE("failed");
            g_clear_object(&self);
        }
    }

    return self;
}

RpClusterInfoImpl*
rp_cluster_info_impl_new(RpServerFactoryContext* server_context, const RpClusterCfg* config, bool added_via_api)
{
    LOGD("(%p, %p, %u)", server_context, config, added_via_api);

    g_return_val_if_fail(RP_IS_SERVER_FACTORY_CONTEXT(server_context), NULL);
    g_return_val_if_fail(config != NULL, NULL);

    RpClusterInfoImpl* self = g_object_new(RP_TYPE_CLUSTER_INFO_IMPL, NULL);
    self->m_server_context = server_context;
    self->m_config = rp_cluster_cfg_new(config);
    self->m_added_via_api = added_via_api;
    return constructed(self);
}
