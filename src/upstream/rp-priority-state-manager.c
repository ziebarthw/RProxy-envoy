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
    const RpLocalInfo* m_local_info;
    RpPrioritySetHostUpdateCb* m_update_cb;
    RpPriorityStatePtr m_priority_state;

    RpPrioritySetUpdateHostsParams m_params;

    RpHostVector* m_host_vector;
};

G_DEFINE_FINAL_TYPE(RpPriorityStateManager, rp_priority_state_manager, G_TYPE_OBJECT)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpPriorityStateManager* self = RP_PRIORITY_STATE_MANAGER(obj);
    g_clear_pointer(&self->m_priority_state, rp_priority_state_unref);
    g_clear_pointer(&self->m_host_vector, rp_host_vector_unref);
    rp_priority_set_update_hosts_params_clear(&self->m_params);

    G_OBJECT_CLASS(rp_priority_state_manager_parent_class)->dispose(obj);
}

static void
rp_priority_state_manager_class_init(RpPriorityStateManagerClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_priority_state_manager_init(RpPriorityStateManager* self)
{
    LOGD("(%p)", self);
    self->m_priority_state = rp_priority_state_new();
}

RpPriorityStateManager*
rp_priority_state_manager_new(RpClusterImplBase* cluster, const RpLocalInfo* local_info, RpPrioritySetHostUpdateCb* update_cb)
{
    LOGD("(%p(%s), %p, %p)", cluster, G_OBJECT_TYPE_NAME(cluster), local_info, update_cb);
    g_return_val_if_fail(RP_IS_CLUSTER_IMPL_BASE(cluster), NULL);
    RpPriorityStateManager* self = g_object_new(RP_TYPE_PRIORITY_STATE_MANAGER, NULL);
    self->m_parent = cluster;
    self->m_local_info = local_info;
    self->m_update_cb = update_cb;
    return self;
}

void
rp_priority_state_manager_register_host_for_priority_2(RpPriorityStateManager* self, RpHost* host,
                                                        const RpLocalityLbEndpointsCfg* locality_lb_endpoint)
{
    LOGD("(%p, %p, %p)", self, host, locality_lb_endpoint);

    g_return_if_fail(RP_IS_PRIORITY_STATE_MANAGER(self));
    g_return_if_fail(RP_IS_HOST(host));
    g_return_if_fail(locality_lb_endpoint != NULL);

    guint32 priority = rp_locality_lb_endpoints_cfg_priority(locality_lb_endpoint);
    g_assert(rp_priority_state_peek_host_list(self->m_priority_state, priority));
    RpHostVector* host_vector = rp_priority_state_peek_host_list(self->m_priority_state, priority);
    rp_host_vector_add(host_vector, host);
}

static inline void
register_host_for_priority_2_take(RpPriorityStateManager* self, RpHost* host /* transfer full */,
                                        const RpLocalityLbEndpointsCfg* locality_lb_endpoint)
{
    NOISY_MSG_("(%p, %p, %p)", self, host, locality_lb_endpoint);
    guint32 priority = rp_locality_lb_endpoints_cfg_priority(locality_lb_endpoint);
    RpHostVector* host_vector = rp_priority_state_peek_host_list(self->m_priority_state, priority);
    rp_host_vector_add_take(host_vector, g_steal_pointer(&host)); // transfer full
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
    register_host_for_priority_2_take(self, RP_HOST(host), locality_lb_endpoint);
}

void
rp_priority_state_manager_initialize_priority_for(RpPriorityStateManager* self, const RpLocalityLbEndpointsCfg* locality_lb_endpoints)
{
    LOGD("(%p, %p)", self, locality_lb_endpoints);
    g_return_if_fail(RP_IS_PRIORITY_STATE_MANAGER(self));
    g_return_if_fail(locality_lb_endpoints != NULL);
    guint32 priority = locality_lb_endpoints->priority;
    rp_priority_state_ensure(self->m_priority_state, priority);
    if (!rp_priority_state_peek_host_list(self->m_priority_state, priority))
    {
        g_assert(self->m_host_vector == NULL);
        RpHostVector* host_vector = rp_host_vector_new();
        NOISY_MSG_("%p, allocated host vector %p", self, host_vector);
        rp_priority_state_set_host_list(self->m_priority_state, priority, host_vector);
        self->m_host_vector = host_vector;
    }
}

RpPriorityStatePtr
rp_priority_state_manager_priority_state(RpPriorityStateManager* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_PRIORITY_STATE_MANAGER(self), NULL);
    return self->m_priority_state;
}

void
rp_priority_state_manager_update_cluster_priority_set(RpPriorityStateManager* self, guint32 priority,
                                                        RpHostVector** current_hosts, const RpHostVector* hosts_added, const RpHostVector* hosts_removed,
                                                        bool* weighted_priority_health, guint32* overprovisioning_factor)
{
    LOGD("(%p, %u, %p, %p(%u), %p, %p, %p)",
        self, priority, current_hosts, hosts_added, rp_host_vector_len(hosts_added), hosts_removed, weighted_priority_health, overprovisioning_factor);

    RpHostVector* hosts = g_steal_pointer(current_hosts);

    rp_priority_set_update_hosts_params_clear(&self->m_params);

    self->m_params = rp_host_set_impl_partition_hosts_take(&hosts);
    g_assert(hosts == NULL);

    if (self->m_update_cb)
    {
        self->m_update_cb->update_hosts(self->m_update_cb,
                                        priority,
                                        &self->m_params,
                                        hosts_added,
                                        hosts_removed,
                                        weighted_priority_health,
                                        overprovisioning_factor);
    }
    else
    {
        RpPrioritySet* priority_set = rp_cluster_priority_set(RP_CLUSTER(self->m_parent));
        rp_priority_set_update_hosts(priority_set,
                                        priority,
                                        &self->m_params,
                                        hosts_added,
                                        hosts_removed,
                                        weighted_priority_health,
                                        overprovisioning_factor,
                                        NULL);
    }
}
