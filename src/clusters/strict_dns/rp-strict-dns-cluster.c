/*
 * rp-strict-dns-cluster.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_strict_dns_cluster_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_strict_dns_cluster_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rule.h"
#include "rp-cluster-configuration.h"
#include "rp-host-set-ptr-vector.h"
#include "network/rp-address-impl.h"
#include "thread_local/rp-thread-local-impl.h"
#include "upstream/rp-delegate-load-balancer-factory.h"
#include "clusters/strict_dns/rp-strict-dns-cluster.h"

struct _RpStrictDnsClusterImpl {
    RpBaseDynamicClusterImpl parent_instance;

    RpClusterLoadAssignmentCfgPtr m_load_assignment;
    RpLocalInfo* m_local_info;
    RpNetworkDnsResolverSharedPtr m_dns_resolver;
    RpThreadAwareLoadBalancerPtr m_thread_aware_lb;
    rule_t* m_rule;
    GList*/* <RpResolveTargetPtr> */ m_resolve_targets;
    guint64 m_dns_refresh_rate_ms;
    guint64 m_dns_jitter_ms;
    //TODO...BackOffStrategyPtr failure_backoff_strategy_;
    RpDnsLookupFamily_e m_dns_lookup_family;
    guint32 m_overprovisioning_factor;
    guint32 m_last_used_element;
    bool m_respect_dns_ttl : 1;
    bool m_weighted_priority_health : 1;
};

static void update_all_hosts(RpStrictDnsClusterImpl* self, const RpHostVector* hosts_added, const RpHostVector* hosts_removed, guint32 priority);

typedef struct _RpResolveTarget RpResolveTarget;
struct _RpResolveTarget {
    RpStrictDnsClusterImpl* m_parent;
    RpNetworkActiveDnsQuery* m_active_query;
    RpDispatcher* m_dispatcher;
    RpLocalityLbEndpointsCfg m_locality_lb_endpoints;
    RpLbEndpointCfg m_lb_endpoint;
    const char* m_dns_address;
    const char* m_hostname;
    guint32 m_port;
    //TODO...const Event::TimePtr resolve_timer_;
    RpHostVector* m_hosts;
    RpHostMap* m_all_hosts;
};

static inline RpResolveTarget
rp_resolve_target_ctor(RpStrictDnsClusterImpl* parent, RpDispatcher* dispatcher G_GNUC_UNUSED, const char* dns_address, guint32 dns_port,
                        const RpLocalityLbEndpointsCfg* locality_lb_endpoint, const RpLbEndpointCfg* lb_endpoint)
{
    RpResolveTarget self = {
        .m_parent = parent,
        .m_active_query = NULL,
        .m_dispatcher = dispatcher,
        .m_locality_lb_endpoints = *locality_lb_endpoint,
        .m_lb_endpoint = *lb_endpoint,
        .m_dns_address = dns_address,
        .m_hostname = !rp_endpoint_cfg_hostname(
                        rp_lb_endpoint_cfg_endpoint(lb_endpoint))[0]/*empty()*/ ?
                            dns_address : rp_endpoint_cfg_hostname(
                                            rp_lb_endpoint_cfg_endpoint(lb_endpoint)),
        .m_port = dns_port,
        .m_hosts = NULL,
        .m_all_hosts = NULL
    };
    return self;
}
static inline void
rp_resolve_target_dtor(RpResolveTarget* self G_GNUC_UNUSED)
{
}
static inline RpResolveTarget*
rp_resolve_target_new(RpStrictDnsClusterImpl* parent, RpDispatcher* dispatcher, const char* dns_address, guint32 dns_port,
                        const RpLocalityLbEndpointsCfg* locality_lb_endpoint, const RpLbEndpointCfg* lb_endpoint)
{
    NOISY_MSG_("(%p, %p, %p(%s), %u, %p, %p)",
        parent, dispatcher, dns_address, dns_address, dns_port, locality_lb_endpoint, lb_endpoint);
    RpResolveTarget* self = g_new(RpResolveTarget, 1);
    *self = rp_resolve_target_ctor(parent, dispatcher, dns_address, dns_port, locality_lb_endpoint, lb_endpoint);
    self->m_hosts = rp_host_vector_new();
    self->m_all_hosts = rp_host_map_new();
    //TODO...startResolve() in a timer...
    return self;
}
static inline void
rp_resolve_target_free(RpResolveTarget* self)
{
    NOISY_MSG_("(%p)", self);
    rp_resolve_target_dtor(self);
    g_clear_pointer(&self->m_hosts, rp_host_vector_unref);
    g_clear_pointer(&self->m_all_hosts, rp_host_map_unref);
    g_free(g_steal_pointer(&self));
}

RpNetworkAddressInstanceConstSharedPtr
rp_network_utiliy_get_address_with_port(const RpNetworkAddressInstance* address, guint32 port)
{
    LOGD("(%p, %u)", address, port);
    RpNetworkAddressIp* ip = rp_network_address_instance_ip((RpNetworkAddressInstance*)address);
    switch (rp_network_address_ip_version(ip))
    {
        case RpIpVersion_v4:
            return RP_NETWORK_ADDRESS_INSTANCE(
                    rp_network_address_ipv4_instance_new_3(
                        rp_network_address_ip_address_as_string(ip), port, NULL));
        case RpIpVersion_v6:
            return RP_NETWORK_ADDRESS_INSTANCE(
                    rp_network_address_ipv6_instance_new_3(
                        rp_network_address_ip_address_as_string(ip), port, NULL, true));
        default:
            LOGE("unknown ip version %d", rp_network_address_ip_version(ip));
            return NULL;
    }
}

static inline void
add_upstream_internal(rule_t* rule, upstream_t* upstream)
{
    upstream_cfg_t* ucfg = upstream->config;
    rproxy_t* rproxy = rule->parent_vhost->rproxy;
    server_cfg_t* server_cfg = rule->parent_vhost->config->server_cfg;
    server_cfg->upstream_cfgs = g_slist_append(server_cfg->upstream_cfgs, ucfg);
    rproxy->upstreams = g_slist_append(rproxy->upstreams, upstream);
}

//#define WITH_MUTLITHREAD_DNS
#ifdef WITH_MUTLITHREAD_DNS
typedef struct _RpAddUpstreamCtx RpAddUpstreamCtx;
struct _RpAddUpstreamCtx {
    rule_t* rule;
    upstream_t* upstream;
};
static inline RpAddUpstreamCtx
rp_add_upstream_cb_ctx_ctor(rule_t* rule, upstream_t* upstream)
{
    RpAddUpstreamCtx self = {
        .rule = rule,
        .upstream = upstream
    };
    return self;
}
static inline RpAddUpstreamCtx*
rp_add_upstream_cb_ctx_new(rule_t* rule, upstream_t* upstream)
{
    RpAddUpstreamCtx* self = g_new(RpAddUpstreamCtx, 1);
    *self = rp_add_upstream_cb_ctx_ctor(rule, upstream);
    return self;
}
static inline RpAddUpstreamCtx
rp_add_upstream_cb_captures(RpAddUpstreamCtx* self)
{
    RpAddUpstreamCtx captured = rp_add_upstream_cb_ctx_ctor(self->rule, self->upstream);
    g_free(self);
    return captured;
}

static void
add_upstream_cb(gpointer arg)
{
    extern void upstream_cfg_free(gpointer);
    NOISY_MSG_("(%p)", arg);
    RpAddUpstreamCtx* ctx = arg;
    RpAddUpstreamCtx captures = rp_add_upstream_cb_captures(ctx);
    add_upstream_internal(captures.rule, captures.upstream);
}

static inline void
post_add_upstream_cb(rule_t* rule, upstream_t* upstream, RpDispatcher* dispatcher)
{
    NOISY_MSG_("(%p, %p, %p)", rule, upstream, dispatcher);
    RpAddUpstreamCtx* ctx = rp_add_upstream_cb_ctx_new(rule, upstream);
    rp_dispatcher_base_post(RP_DISPATCHER_BASE(dispatcher), add_upstream_cb, ctx);
}
#endif//WITH_MUTLITHREAD_DNS

static upstream_t*
add_upstream(upstream_cfg_t* cfg, guint count, RpNetworkAddressInstanceConstSharedPtr addr, rule_t* rule, RpDispatcher* dispatcher)
{
    extern upstream_cfg_t* upstream_cfg_new(void);
    NOISY_MSG_("(%p, %u, %p, %p, %p)", cfg, count, addr, rule, dispatcher);
    RpNetworkAddressIp* ip = rp_network_address_instance_ip(addr);
    upstream_cfg_t* ucfg = upstream_cfg_new();
    ucfg->enabled = true;
    ucfg->high_watermark = cfg->high_watermark;
    ucfg->n_connections = cfg->n_connections;
    ucfg->read_timeout = cfg->read_timeout;
    ucfg->write_timeout = cfg->write_timeout;
    ucfg->name = g_strdup_printf("%s-%d", cfg->name, count);
    ucfg->host = g_strdup(rp_network_address_ip_address_as_string(ip));
    ucfg->port = rp_network_address_ip_port(ip);
    upstream_t* self = upstream_new(ucfg);
#ifdef WITH_MUTLITHREAD_DNS
    post_add_upstream_cb(rule, self, dispatcher);
#else
    add_upstream_internal(rule, self);
#endif//WITH_MUTLITHREAD_DNS
    return self;
}

static void
network_dns_resolve_cb(RpDnsResolutionStatus_e status, char *details, GList* response, gpointer arg)
{
    NOISY_MSG_("(%d, %p(%s), %p, %p)", status, details, details, response, arg);

    RpResolveTarget* self = arg;
    RpStrictDnsClusterImpl* parent_ = self->m_parent;
    RpDispatcher* dispatcher = self->m_dispatcher;

    g_clear_object(&self->m_active_query);

    NOISY_MSG_("async DNS resolution complete for %p(%p(%s)), details \"%s\"",
        parent_, self->m_dns_address, self->m_dns_address, details);

    guint64 final_refresh_rate = parent_->m_dns_refresh_rate_ms;
    guint32 priority = rp_locality_lb_endpoints_cfg_priority(&self->m_locality_lb_endpoints);
    upstream_t* parent_upstream = rp_lb_endpoint_cfg_metadata(&self->m_lb_endpoint);
    upstream_cfg_t* parent_upstream_cfg = parent_upstream->config;
    rule_t* rule = parent_->m_rule;
    guint count = g_slist_length(rule->upstreams);

    if (status == RpDnsResolutionStatus_COMPLETED)
    {
        //TODO...parent_.info_->configUpdateStats().update_success_.inc();

        RpHostVector* new_hosts = rp_host_vector_new();
        guint64 ttl_refresh_rate = G_MAXUINT64;
        g_autoptr(GHashTable) all_new_hosts = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
        for (GList* itr = response; itr; itr = itr->next)
        {
            RpNetworkDnsResponse* resp = itr->data;
            const RpAddrInfoResponse* addrinfo = rp_network_dns_response_addr_info(resp);
            g_assert(addrinfo->m_address != NULL);
            RpNetworkAddressInstanceConstSharedPtr address = rp_network_utiliy_get_address_with_port(addrinfo->m_address, self->m_port);
            if (g_hash_table_contains(all_new_hosts, rp_network_address_instance_as_string(address)))
            {
                NOISY_MSG_("exists");
                continue;
            }
            upstream_t* upstream = add_upstream(parent_upstream_cfg, count++, address, rule, dispatcher);
            RpHostImpl* host = rp_host_impl_create(rp_cluster_info(RP_CLUSTER(parent_)),
                                                    self->m_dns_address,
                                                    address,
                                                    upstream,
                                                    rp_lb_endpoint_cfg_load_balancing_weight_value(&self->m_lb_endpoint),
                                                    rp_locality_lb_endpoints_cfg_priority(&self->m_locality_lb_endpoints),
                                                    rp_cluster_impl_base_time_source_(RP_CLUSTER_IMPL_BASE(parent_)));
            rp_host_vector_add_take(new_hosts, (RpHost*)g_steal_pointer(&host));
            g_hash_table_add(all_new_hosts,
                g_strdup(rp_network_address_instance_as_string(address)));
            ttl_refresh_rate = MIN(ttl_refresh_rate, addrinfo->m_ttl);
        }

        RpHostVector* hosts_added = NULL;
        RpHostVector* hosts_removed = NULL;
        if (rp_base_dynamic_cluster_impl_update_dynamic_host_list(RP_BASE_DYNAMIC_CLUSTER_IMPL(parent_),
                                                                    new_hosts,
                                                                    &self->m_hosts,
                                                                    &hosts_added,
                                                                    &hosts_removed,
                                                                    self->m_all_hosts,
                                                                    all_new_hosts))
        {
            NOISY_MSG_("DNS hosts have changed for \"%s\"", self->m_dns_address);
            //TODO...ASSERT(...);

            if (hosts_removed)
            {
                for (guint i = 0; i < rp_host_vector_len(hosts_removed); ++i)
                {
                    RpHost* host = rp_host_vector_get(hosts_removed, i);
                    rp_host_map_remove(self->m_all_hosts, host);
                }
                g_clear_pointer(&hosts_removed, rp_host_vector_unref);
            }
            if (hosts_added)
            {
                for (guint i = 0; hosts_added && i < rp_host_vector_len(hosts_added); ++i)
                {
                    RpHost* host = rp_host_vector_get(hosts_added, i);
                    rp_host_map_add(self->m_all_hosts, host);
                }
                g_clear_pointer(&hosts_added, rp_host_vector_unref);
            }

            //TODO...parent_.updateAllHosts(...)
            update_all_hosts(parent_, hosts_added, hosts_removed, priority);
        }
        else
        {
            //TODO...parent_.info_->configUpdateStats().update_no_rebuild_.inc();
        }

        //TODO...parent_.failure_backoff_strategy_->reset();

        if (response/*!response.empty()*/ &&
            parent_->m_respect_dns_ttl &&
            ttl_refresh_rate != 0)
        {
            final_refresh_rate = ttl_refresh_rate;
        }
        if (parent_->m_dns_jitter_ms > 0)
        {
            //TODO...final_refresh_rate += ...
        }
    }
    else
    {
        //TODO...parent_.info_->configUpdateStats().update_failure_.inc();

        //TODO...final_refresh_rate = ...
    }

    g_list_free_full(response, g_object_unref);
    g_clear_pointer(&details, g_free);

    rp_cluster_impl_base_on_pre_init_complete(RP_CLUSTER_IMPL_BASE(parent_));
    //TODO...resolve_timer_->enabaleTimer(final_refresh_rate);
}

static inline void
rp_resolve_target_start_resolve(RpResolveTarget* self)
{
    NOISY_MSG_("(%p)", self);
    NOISY_MSG_("starting async DNS resolution for %p(%p(%s))", self->m_parent, self->m_dns_address, self->m_dns_address);
    //TODO...parent_.info->configUpdateStats().update_attempt_.inc();

    self->m_active_query = rp_dns_resolver_resolve(self->m_parent->m_dns_resolver,
                                                    rp_thread_local_instance_impl_get_dispatcher(),
                                                    self->m_dns_address,
                                                    self->m_parent->m_dns_lookup_family,
                                                    network_dns_resolve_cb,
                                                    self);

}

typedef UNIQUE_PTR(RpResolveTarget) RpResolveTargetPtr;

static void load_balancer_iface_init(RpLoadBalancerInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpStrictDnsClusterImpl, rp_strict_dns_cluster_impl, RP_TYPE_BASE_DYNAMIC_CLUSTER_IMPL,
    G_IMPLEMENT_INTERFACE(RP_TYPE_LOAD_BALANCER, load_balancer_iface_init)
)

static void
update_all_hosts(RpStrictDnsClusterImpl* self, const RpHostVector* hosts_added, const RpHostVector* hosts_removed, guint32 current_priority)
{
    NOISY_MSG_("(%p, %p, %p, %u)", self, hosts_added, hosts_removed, current_priority);
    g_autoptr(RpPriorityStateManager) priority_state_manager = rp_priority_state_manager_new(RP_CLUSTER_IMPL_BASE(self), self->m_local_info, NULL);
    for (GList* itr = self->m_resolve_targets; itr; itr = itr->next)
    {
        RpResolveTarget* target = itr->data;
        rp_priority_state_manager_initialize_priority_for(priority_state_manager, &target->m_locality_lb_endpoints);
        for (gint i = 0; i < rp_host_vector_len(target->m_hosts); ++i)
        {
            RpHost* host = rp_host_vector_get(target->m_hosts, i);
            if (rp_locality_lb_endpoints_cfg_priority(&target->m_locality_lb_endpoints) == current_priority)
            {
                rp_priority_state_manager_register_host_for_priority_2(priority_state_manager, host, &target->m_locality_lb_endpoints);
            }
        }
    }

    RpPriorityStatePtr priority_state = rp_priority_state_manager_priority_state(priority_state_manager);
    RpHostListPtr host_list = rp_priority_state_steal_host_list(priority_state, current_priority);
    bool weighted_priority_health = self->m_weighted_priority_health;
    rp_priority_state_manager_update_cluster_priority_set(priority_state_manager,
                                                            current_priority,
                                                            &host_list,
                                                            hosts_added,
                                                            hosts_removed,
                                                            &weighted_priority_health,
                                                            &self->m_overprovisioning_factor);
}

RpHost*
get_round_robin(RpLoadBalancer* self)
{
    LOGD("(%p)", self);

    RpStrictDnsClusterImpl* me = RP_STRICT_DNS_CLUSTER_IMPL(self);

    RpPrioritySet* priority_set = rp_cluster_priority_set(RP_CLUSTER(self));
    // REVISIT to use cross priority host map.
    const RpHostSetPtrVector* host_set = rp_priority_set_host_sets_per_priority(priority_set);
    const RpHostVector* hosts = rp_host_set_get_healthy_hosts(rp_host_set_ptr_vector_get(host_set, 0));

    NOISY_MSG_("choosing from %u hosts, last used element %u", rp_host_vector_len(hosts), me->m_last_used_element);

    if (rp_host_vector_len(hosts) < 2)
    {
        NOISY_MSG_("only 1 host");
        return rp_host_vector_get(hosts, 0);
    }

    guint32 last_used_element = me->m_last_used_element;

    RpHost* host;
    if (last_used_element == G_MAXUINT32)
    {
        host = rp_host_vector_get(hosts, 0);
        last_used_element = 0;
    }
    else
    {
        ++last_used_element;
        if (last_used_element >= rp_host_vector_len(hosts))
        {
            NOISY_MSG_("wrapping");
            last_used_element = 0;
        }
        host = rp_host_vector_get(hosts, last_used_element);
    }

    NOISY_MSG_("chose host %p at index %u", host, last_used_element);

    me->m_last_used_element = last_used_element;
    return host;
}

static RpHostSelectionResponse
choose_host_i(RpLoadBalancer* self, RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %p)", self, context);
    return rp_host_selection_response_ctor(get_round_robin(self), NULL, NULL);
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

    RpStrictDnsClusterImpl* self = RP_STRICT_DNS_CLUSTER_IMPL(obj);
    g_clear_pointer(&self->m_load_assignment, g_free);
    g_clear_object(&self->m_dns_resolver);
    g_list_free_full(g_steal_pointer(&self->m_resolve_targets),
        (GDestroyNotify)rp_resolve_target_free);

    G_OBJECT_CLASS(rp_strict_dns_cluster_impl_parent_class)->dispose(obj);
}

OVERRIDE void
start_pre_init(RpClusterImplBase* self)
{
    NOISY_MSG_("(%p)", self);

    RpStrictDnsClusterImpl* me = RP_STRICT_DNS_CLUSTER_IMPL(self);
    for (GList* itr = me->m_resolve_targets; itr; itr = itr->next)
    {
        RpResolveTarget* target = itr->data;
        rp_resolve_target_start_resolve(target);
    }
    if (!me->m_resolve_targets/*empty()*/ || !rp_cluster_impl_base_wait_for_warm_on_init_(self))
    {
        NOISY_MSG_("calling rp_cluster_impl_base_on_pre_init_complete(%p)", self);
        rp_cluster_impl_base_on_pre_init_complete(self);
    }
}

static void
cluster_impl_base_class_init(RpClusterImplBaseClass* klass)
{
    LOGD("(%p)", klass);
    klass->start_pre_init = start_pre_init;
}

static void
rp_strict_dns_cluster_impl_class_init(RpStrictDnsClusterImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    cluster_impl_base_class_init(RP_CLUSTER_IMPL_BASE_CLASS(klass));
}

static RpLoadBalancerPtr
lb_create_fn(RpClusterSharedPtr self, RpLoadBalancerParams* params G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p(%s), %p)", self, G_OBJECT_TYPE_NAME(self), params);
    return g_object_ref(RP_LOAD_BALANCER(self));
}

static void
rp_strict_dns_cluster_impl_init(RpStrictDnsClusterImpl* self)
{
    NOISY_MSG_("(%p(%s))", self, G_OBJECT_TYPE_NAME(self));
    self->m_last_used_element = G_MAXUINT32;
    self->m_thread_aware_lb = rp_simple_thread_aware_load_balancer_new(
                                rp_delegate_load_balancer_factory_new(RP_CLUSTER(self), lb_create_fn, false/*TODO...true*/));
}

static inline RpStrictDnsClusterImpl*
constructed(RpStrictDnsClusterImpl* self, RpClusterFactoryContext* context)
{
    NOISY_MSG_("(%p(%s), %p(%s))", self, G_OBJECT_TYPE_NAME(self), context, G_OBJECT_TYPE_NAME(context));

    GList* resolve_targets = NULL;
    RpDispatcher* main_thread_dispatcher = rp_common_factory_context_main_thread_dispatcher(RP_COMMON_FACTORY_CONTEXT(
                                            rp_cluster_factory_context_server_factory_context(context)));
    const RpLocalityLbEndpointsCfg* locality_lb_endpoints = rp_cluster_load_assignment_cfg_endpoints(self->m_load_assignment);
    gint n_locality_lb_endpoints = rp_cluster_load_assignment_cfg_endpoints_len(self->m_load_assignment);
    for (gint i = 0; i < n_locality_lb_endpoints; ++i)
    {
        RpLocalityLbEndpointsCfg locality_lb_endpoint = locality_lb_endpoints[i];
        const RpLbEndpointCfg* lb_endpoints = rp_locality_lb_endpoints_cfg_lb_endpoints(&locality_lb_endpoint);
        gint n_lb_endpoints = rp_locality_lb_endpoints_cfg_lb_endpoints_len(&locality_lb_endpoint);
        for (gint e = 0; e < n_lb_endpoints; ++e)
        {
            RpLbEndpointCfg lb_endpoint = lb_endpoints[e];
            const RpSocketAddressCfg* socket_address = rp_address_cfg_socket_address(
                                                        rp_endpoint_cfg_address(
                                                            rp_lb_endpoint_cfg_endpoint(&lb_endpoint)));
            if (rp_socket_address_cfg_resolver_name(socket_address)[0]/*!empty()*/)
            {
                LOGE("STRICT_DNS clusters must NOT have a custom resolver name set");
                //TODO...supposed to be an exception.
            }

            NOISY_MSG_("socket address \"%s:%u\"",
                rp_socket_address_cfg_address(socket_address), rp_socket_address_cfg_port_value(socket_address));

            resolve_targets = g_list_append(resolve_targets,
                                            rp_resolve_target_new(self,
                                                main_thread_dispatcher,
                                                rp_socket_address_cfg_address(socket_address),
                                                rp_socket_address_cfg_port_value(socket_address),
                                                &locality_lb_endpoint,
                                                &lb_endpoint));
        }
    }
    self->m_resolve_targets = g_steal_pointer(&resolve_targets);

    // Add the rule to the bootstrap rproxy rules list.
    rproxy_t* rproxy = self->m_rule->parent_vhost->rproxy;
    rproxy->rules = g_slist_append(rproxy->rules, self->m_rule);

    const RpLbPolicyCfg* policy = rp_cluster_load_assignment_cfg_policy(self->m_load_assignment);
    self->m_overprovisioning_factor = policy->overprovisioning_factor ?
        policy->overprovisioning_factor : kDefaultOverProvisioningFactor;
    self->m_weighted_priority_health = policy->weighted_priority_health;
    return self;
}

RpStrictDnsClusterImpl*
rp_strict_dns_cluster_impl_new(const RpClusterCfg* cluster, const RpDnsClusterCfg* dns_cluster,
                                RpClusterFactoryContext* context, RpNetworkDnsResolverSharedPtr dns_resolver, RpStatusCode_e* creation_status)
{
    LOGD("(%p, %p, %p(%s), %p(%s), %p)",
        cluster, dns_cluster, context, G_OBJECT_TYPE_NAME(context), dns_resolver, G_OBJECT_TYPE_NAME(dns_resolver), creation_status);

    g_return_val_if_fail(cluster != NULL, NULL);
    g_return_val_if_fail(dns_cluster != NULL, NULL);
    g_return_val_if_fail(RP_IS_CLUSTER_FACTORY_CONTEXT(context), NULL);
    g_return_val_if_fail(RP_IS_NETWORK_DNS_RESOLVER(dns_resolver), NULL);
    g_return_val_if_fail(creation_status != NULL, NULL);

    RpStrictDnsClusterImpl* self = g_object_new(RP_TYPE_STRICT_DNS_CLUSTER_IMPL,
                                                "cluster-context", context,
                                                "config", cluster,
                                                "creation-status", creation_status,
                                                NULL);
    self->m_load_assignment = rp_cluster_load_assignment_cfg_new(
                                rp_cluster_cfg_load_assignment(cluster));
    self->m_local_info = rp_common_factory_context_local_info(RP_COMMON_FACTORY_CONTEXT(
                            rp_cluster_factory_context_server_factory_context(context)));
    self->m_dns_resolver = g_object_ref(dns_resolver);
    self->m_dns_refresh_rate_ms = rp_dns_cluster_cfg_dns_refresh_rate(dns_cluster);
    if (!self->m_dns_refresh_rate_ms) self->m_dns_refresh_rate_ms = 5000;
    self->m_respect_dns_ttl = rp_dns_cluster_cfg_respect_dns_ttl(dns_cluster);
    self->m_dns_lookup_family = rp_dns_cluster_cfg_dns_lookup_family(dns_cluster);
    // Create a new rule - based on the parent rule (the dfp cluster), primarily
    // to keep the upstream(s) associated with this sub-cluster contained
    // within this sub-cluster.
    rule_t* rule = rp_cluster_cfg_rule(cluster);
    self->m_rule = rule_new(rule->config, rule->parent_vhost);
    return constructed(self, context);
}

static inline PairClusterSharedPtrThreadAwareLoadBalancerPtr
null_pair(void)
{
    return PairClusterSharedPtrThreadAwareLoadBalancerPtr_make(NULL, NULL);
}

PairClusterSharedPtrThreadAwareLoadBalancerPtr
rp_strict_dns_cluster_impl_create(const RpClusterCfg* cluster, const RpDnsClusterCfg* dns_cluster,
                                    RpClusterFactoryContext* context, RpNetworkDnsResolverSharedPtr dns_resolver)
{
    LOGD("(%p, %p, %p, %p)", cluster, dns_cluster, context, dns_resolver);

    g_return_val_if_fail(cluster != NULL, null_pair());
    g_return_val_if_fail(dns_cluster != NULL, null_pair());
    g_return_val_if_fail(RP_IS_CLUSTER_FACTORY_CONTEXT(context), null_pair());
    g_return_val_if_fail(RP_IS_NETWORK_DNS_RESOLVER(dns_resolver), null_pair());

    RpStatusCode_e creation_status = RpStatusCode_Ok;
    g_autoptr(RpStrictDnsClusterImpl) new_cluster = rp_strict_dns_cluster_impl_new(cluster, dns_cluster, context, dns_resolver, &creation_status);
    if (creation_status != RpStatusCode_Ok)
    {
        LOGE("failed");
        return null_pair();
    }
    RpThreadAwareLoadBalancerPtr lb = new_cluster->m_thread_aware_lb;
    return PairClusterSharedPtrThreadAwareLoadBalancerPtr_make(
            RP_CLUSTER(g_steal_pointer(&new_cluster)), lb);

}
