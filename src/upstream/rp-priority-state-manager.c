/*
 * rp-priority-state-manager.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_priority_state_manager_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_priority_state_manager_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-upstream-impl.h"

struct _RpPriorityStateManager {
    GObject parent_instance;

    RpClusterImplBase* m_parent;
    RpLocalInfo* m_local_info;
//    GPtrArray/*std::vector<std::pair<HostListPtr, LocalityWeightsMap>>*/* m_priority_state;
    RpPriorityState m_priority_state;
};

enum
{
    PROP_0, // Reserved.
    PROP_PARENT,
    PROP_LOCAL_INFO,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_FINAL_TYPE(RpPriorityStateManager, rp_priority_state_manager, G_TYPE_OBJECT)

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_PARENT:
            g_value_set_object(value, RP_PRIORITY_STATE_MANAGER(obj)->m_parent);
            break;
        case PROP_LOCAL_INFO:
            g_value_set_object(value, RP_PRIORITY_STATE_MANAGER(obj)->m_local_info);
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
        case PROP_PARENT:
            RP_PRIORITY_STATE_MANAGER(obj)->m_parent = g_value_get_object(value);
            break;
        case PROP_LOCAL_INFO:
            RP_PRIORITY_STATE_MANAGER(obj)->m_local_info = g_value_get_object(value);
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

    RpPriorityStateManager* self = RP_PRIORITY_STATE_MANAGER(obj);
    for (guint i=0; i < self->m_priority_state->len; ++i)
    {
        RpHostVector* hosts = g_ptr_array_index(self->m_priority_state, i);
        if (hosts) g_ptr_array_free(g_steal_pointer(&hosts), true);
    }
    g_ptr_array_free(g_steal_pointer(&self->m_priority_state), true);

    G_OBJECT_CLASS(rp_priority_state_manager_parent_class)->dispose(obj);
}

static void
rp_priority_state_manager_class_init(RpPriorityStateManagerClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_PARENT] = g_param_spec_object("cluster",
                                                    "Cluster",
                                                    "Parent ClusterImplBase Instance",
                                                    RP_TYPE_CLUSTER_IMPL_BASE,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_LOCAL_INFO] = g_param_spec_object("local-info",
                                                    "Local info",
                                                    "Local Info Instance",
                                                    RP_TYPE_LOCAL_INFO,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_priority_state_manager_init(RpPriorityStateManager* self)
{
    LOGD("(%p)", self);
    self->m_priority_state = g_ptr_array_new_full(128, NULL); //REVISIT - max priority value.
}

RpPriorityStateManager*
rp_priority_state_manager_new(RpClusterImplBase* cluster, const RpLocalInfo* local_info/*, RpHostUpdateCb*/)
{
    LOGD("(%p, %p)", cluster, local_info);
    g_return_val_if_fail(RP_IS_CLUSTER_IMPL_BASE(cluster), NULL);
    return g_object_new(RP_TYPE_PRIORITY_STATE_MANAGER,
                        "cluster", cluster,
                        "local-info", local_info,
                        NULL);
}

#if 0
void
rp_priority_state_manager_register_host_for_priority(RpPriorityStateManager* self,
                                                        RpUpstreamTransportSocketFactory* socket_factory,
                                                        const char* hostname,
                                                        struct sockaddr* address,
                                                        guint32 initial_weight,
                                                        bool disable_health_check,
                                                        guint32 priority,
                                                        GSList* address_list)
{
    LOGD("(%p, %p, %p(%s), %p, %u, %u, %u, %p)",
        self, socket_factory, hostname, hostname, address, initial_weight, disable_health_check, priority, address_list);
    g_return_if_fail(RP_IS_PRIORITY_STATE_MANAGER(self));
    g_return_if_fail(address != NULL);
    RpClusterImplBase* parent = self->m_parent;
    RpClusterInfoConstSharedPtr info = rp_cluster_info(RP_CLUSTER(parent));
    RpHostImpl* host = rp_host_impl_new(info, socket_factory, hostname, address, initial_weight, disable_health_check, priority, address_list);
    GArray* priority_state = ensure_priority_state(self);
    GSList* hosts = g_array_index(priority_state, GSList*, priority);

    hosts = g_slist_append(hosts, host);
    self->m_priority_state = g_array_insert_val(priority_state, priority, hosts);
}
#endif//0
static void
internal_register_host_for_priority(RpPriorityStateManager* self, RpHostSharedPtr host, const RpLocalityLbEndpointsCfg* locality_lb_endpoint)
{
    NOISY_MSG_("(%p, %p, %p)", self, host, locality_lb_endpoint);
    guint32 priority = locality_lb_endpoint->priority;
    g_assert(self->m_priority_state->pdata[priority]);
    RpHostVector* host_vector = g_ptr_array_index(self->m_priority_state, priority);
    g_ptr_array_add(host_vector, host);
}
#if 0
const auto host = std::shared_ptr<HostImpl>((
      HostImpl::create(parent_.info(), hostname, address, endpoint_metadata, locality_metadata,
                       lb_endpoint.load_balancing_weight().value(), locality_lb_endpoint.locality(),
                       lb_endpoint.endpoint().health_check_config(),
                       locality_lb_endpoint.priority(), lb_endpoint.health_status(),
                       time_source,
                       address_list),
      std::unique_ptr<HostImpl>));
#endif//0
void
rp_priority_state_manager_register_host_for_priority(RpPriorityStateManager* self, const char* hostname,
                                                        RpNetworkAddressInstanceConstSharedPtr address/*,std::vector<Network::Address::InstanceConstSharedPtr> address_list*/,
                                                        const RpLocalityLbEndpointsCfg* locality_lb_endpoint, const RpLbEndpointCfg* lb_endpoint,
                                                        RpTimeSource* time_source)
{
    LOGD("(%p, %p(%s), %p, %p, %p, %p)",
        self, hostname, hostname, address, locality_lb_endpoint, lb_endpoint, time_source);

    g_return_if_fail(RP_IS_PRIORITY_STATE_MANAGER(self));
    g_return_if_fail(hostname != NULL);
    g_return_if_fail(hostname[0]);
    g_return_if_fail(locality_lb_endpoint != NULL);
    g_return_if_fail(lb_endpoint != NULL);
    g_return_if_fail(RP_IS_TIME_SOURCE(time_source));

    RpHostImpl* host = rp_host_impl_create(rp_cluster_info(RP_CLUSTER(self->m_parent)),
                                            hostname,
                                            address,
                                            lb_endpoint->metadata,
                                            lb_endpoint->load_balancing_weight,
                                            locality_lb_endpoint->priority,
                                            time_source);
    internal_register_host_for_priority(self, RP_HOST(host), locality_lb_endpoint);
}

static RpHostVector*
rp_host_vector_new(void)
{
    return g_ptr_array_new();
}

void
rp_priority_state_manager_initialize_priority_for(RpPriorityStateManager* self, const RpLocalityLbEndpointsCfg* locality_lb_endpoints)
{
    LOGD("(%p, %p)", self, locality_lb_endpoints);
    g_return_if_fail(RP_IS_PRIORITY_STATE_MANAGER(self));
    g_return_if_fail(locality_lb_endpoints != NULL);
    guint32 priority = locality_lb_endpoints->priority;
    if (self->m_priority_state->len <= priority)
    {
        NOISY_MSG_("expanding priority state array to %u", priority + 1);
        g_ptr_array_set_size(self->m_priority_state, priority + 1);
    }
    if (!self->m_priority_state->pdata[priority])
    {
        NOISY_MSG_("allocating host vector");
        self->m_priority_state->pdata[priority] = rp_host_vector_new();
    }
}

RpPriorityState
rp_priority_state_manager_priority_state(RpPriorityStateManager* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_PRIORITY_STATE_MANAGER(self), NULL);
    return self->m_priority_state;
}
