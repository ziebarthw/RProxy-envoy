/*
 * rp-static-cluster.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_static_cluster_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_static_cluster_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "network/rp-address-impl.h"
#include "clusters/static/rp-static-cluster.h"

#define PARENT_CLUSTER_IFACE(s) \
    ((RpClusterInterface*)g_type_interface_peek_parent(RP_CLUSTER_GET_IFACE(s)))

struct _RpStaticClusterImpl {
    RpClusterImplBase parent_instance;

    UNIQUE_PTR(GHashTable) m_host_map;

    RpPriorityStateManagerPtr m_priority_state_manager;
    guint32 m_overprovisioning_factor;
    bool m_weighted_priority_health;
};

static void cluster_iface_init(RpClusterInterface* iface);
static void load_balancer_iface_init(RpLoadBalancerInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpStaticClusterImpl, rp_static_cluster_impl, RP_TYPE_CLUSTER_IMPL_BASE,
    G_IMPLEMENT_INTERFACE(RP_TYPE_CLUSTER, cluster_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_LOAD_BALANCER, load_balancer_iface_init)
)

static RpInitializePhase_e
initialize_phase_i(RpCluster* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return RpInitializePhase_Primary;
}

static RpClusterInfoConstSharedPtr
info_i(RpCluster* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_CLUSTER_IFACE(self)->info(self);
}

static void
initialize_i(RpCluster* self, RpClusterInitializeCb cb, gpointer arg)
{
    NOISY_MSG_("(%p, %p, %p)", self, cb, arg);
    PARENT_CLUSTER_IFACE(self)->initialize(self, cb, arg);
}

static RpPrioritySet*
priority_set_i(RpCluster* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_CLUSTER_IFACE(self)->priority_set(self);
}

static void
cluster_iface_init(RpClusterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->initialize_phase = initialize_phase_i;
    iface->info = info_i;
    iface->initialize = initialize_i;
    iface->priority_set = priority_set_i;
}

static RpHostSelectionResponse
choose_host_i(RpLoadBalancer* self, RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %p)", self, context);

    RpStaticClusterImpl* me = RP_STATIC_CLUSTER_IMPL(self);
    const RpClusterCfg* config = rp_cluster_impl_base_config_(RP_CLUSTER_IMPL_BASE(self));
    upstream_t* upstream = upstream_get(config->rule);
    RpHostConstSharedPtr host = g_hash_table_lookup(me->m_host_map, upstream);
    return rp_host_selection_response_ctor(host, NULL, NULL);
}

static void
load_balancer_iface_init(RpLoadBalancerInterface* iface)
{
    LOGD("(%p)", iface);
    iface->choose_host = choose_host_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpStaticClusterImpl* self = RP_STATIC_CLUSTER_IMPL(obj);
    g_clear_pointer(&self->m_host_map, g_hash_table_unref);
    g_clear_object(&self->m_priority_state_manager);

    G_OBJECT_CLASS(rp_static_cluster_impl_parent_class)->dispose(obj);
}

OVERRIDE void
start_pre_init(RpClusterImplBase* self)
{
    NOISY_MSG_("(%p)", self);
    rp_cluster_impl_base_on_pre_init_complete(self);
}

static void
cluster_impl_base_class_init(RpClusterImplBaseClass* klass)
{
    LOGD("(%p)", klass);
    klass->start_pre_init = start_pre_init;
}

static void
rp_static_cluster_impl_class_init(RpStaticClusterImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    cluster_impl_base_class_init(RP_CLUSTER_IMPL_BASE_CLASS(klass));
}

static void
rp_static_cluster_impl_init(RpStaticClusterImpl* self)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpLocalInfo*
local_info_from_cluster_factory_context(RpClusterFactoryContext* context)
{
    NOISY_MSG_("(%p)", context);
    return rp_common_factory_context_local_info(RP_COMMON_FACTORY_CONTEXT(
            rp_cluster_factory_context_server_factory_context(context)));
}

static inline RpDispatcher*
main_thread_dispatcher_from_cluster_factory_context(RpClusterFactoryContext* context)
{
    NOISY_MSG_("(%p)", context);
    return rp_common_factory_context_main_thread_dispatcher(RP_COMMON_FACTORY_CONTEXT(
            rp_cluster_factory_context_server_factory_context(context)));
}

static RpNetworkAddressInstanceConstSharedPtr
parse_internet_address(const char* ip_address, guint16 port, bool v6only)
{
    NOISY_MSG_("(%p(%s), %u, %u)", ip_address, ip_address, port, v6only);

    struct sockaddr_storage out;
    int outlen = sizeof(out);
    if (evutil_parse_sockaddr_port(ip_address, (struct sockaddr*)&out, &outlen) == -1)
    {
        LOGE("invalid address \"%s\"", ip_address);
        return NULL;
    }
    switch (out.ss_family)
    {
        case AF_INET:
        {
            struct sockaddr_in* in4 = (struct sockaddr_in*)&out;
            in4->sin_port = htons(port);
            return rp_network_address_instance_factory_create_instance_ptr(in4);
        }
        case AF_INET6:
        {
            struct sockaddr_in6* in6 = (struct sockaddr_in6*)&out;
            in6->sin6_port = htons(port);
            return rp_network_address_instance_factory_create_instance_ptr_2(in6, v6only);
        }
        default:
            break;
    }
    LOGE("shouldn't be here...");
    return NULL;
}

static inline RpStaticClusterImpl*
constructed(RpStaticClusterImpl* self, const RpClusterCfg* cluster, RpClusterFactoryContext* context)
{
    NOISY_MSG_("(%p, %p, %p)", self, cluster, context);

    RpLocalInfo* local_info = local_info_from_cluster_factory_context(context);
    self->m_priority_state_manager = rp_priority_state_manager_new(RP_CLUSTER_IMPL_BASE(self), local_info);
    self->m_overprovisioning_factor = kDefaultOverProvisioningFactor;
    self->m_weighted_priority_health = false; //kDefaultWeightedPriorityHealth;

    RpDispatcher* dispatcher = main_thread_dispatcher_from_cluster_factory_context(context);

    RpLocalityLbEndpointsCfg endpoints = cluster->endpoints;
    for (lztq_elem* elem = lztq_first(endpoints.lb_endpoints); elem; elem = lztq_next(elem))
    {
        upstream_t* upstream = lztq_elem_data(elem);
        upstream_cfg_t* cfg = upstream->config;

        RpLocalityLbEndpointsCfg locality_lb_endpoint = {
            .load_balancing_weight = 1,
            .priority = 0
        };
        RpNetworkAddressInstanceConstSharedPtr address = parse_internet_address(cfg->host, cfg->port, true);
        RpEndpointCfg endpoint = {
            .address = address,
            .hostname = cfg->name
        };
        RpLbEndpointCfg lb_endpoint = {
            .host_identifier.endpoint = endpoint,
            .load_balancing_weight = 1,
            .metadata = upstream
        };

        rp_priority_state_manager_initialize_priority_for(self->m_priority_state_manager,
                                                            &locality_lb_endpoint);
        //TODO...addition addresses...
        rp_priority_state_manager_register_host_for_priority(self->m_priority_state_manager,
                                                                lb_endpoint.host_identifier.endpoint.hostname,
                                                                address/*TODO...resolveProtoAddress(lb_endpoint.endpoint().address())*/,
                                                                &locality_lb_endpoint,
                                                                &lb_endpoint,
                                                                rp_dispatcher_time_source(dispatcher));

        // Associate |upstream| with the newly-created host.
        RpPriorityState priority_state = rp_priority_state_manager_priority_state(self->m_priority_state_manager);
        RpHostListPtr host_list = g_ptr_array_index(priority_state, 0);
        RpHostConstSharedPtr host = g_ptr_array_index(host_list, host_list->len - 1);
        g_hash_table_insert(self->m_host_map, upstream, g_object_ref(host));

        g_clear_object(&address);
    }

    return self;
}

RpStaticClusterImpl*
rp_static_cluster_impl_new(const RpClusterCfg* cluster, RpClusterFactoryContext* context, RpStatusCode_e* creation_status)
{
    LOGD("(%p, %p, %p)", cluster, context, creation_status);

    g_return_val_if_fail(cluster != NULL, NULL);
    g_return_val_if_fail(RP_IS_CLUSTER_FACTORY_CONTEXT(context), NULL);
    g_return_val_if_fail(creation_status != NULL, NULL);

    g_autoptr(RpStaticClusterImpl) self = g_object_new(RP_TYPE_STATIC_CLUSTER_IMPL,
                                                        "config", cluster,
                                                        "cluster-context", context,
                                                        "creation-status", creation_status,
                                                        NULL);
    if (*creation_status != RpStatusCode_Ok)
    {
        LOGE("failed");
        return NULL;
    }
    self->m_host_map = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_object_unref);
    return constructed(g_steal_pointer(&self), cluster, context);
}
