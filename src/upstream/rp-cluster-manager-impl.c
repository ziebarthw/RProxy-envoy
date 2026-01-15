/*
 * rp-cluster-manager-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_manager_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_manager_impl_NOISY)
#   define NOISY_MSG_ LOGD
#   define IF_NOISY_(x, ...) x##__VA_ARGS__
#else
#   define NOISY_MSG_(x, ...)
#   define IF_NOISY_(x, ...)
#endif

#include "http1/rp-http1-conn-pool.h"
#include "server/rp-factory-context-impl.h"
#include "rp-cluster-factory.h"
#include "rp-load-balancer.h"
#include "rp-thread-local-cluster.h"
#include "rp-cluster-manager-impl.h"

typedef UNIQUE_PTR(GHashTable) RpClusterMap;

typedef struct _RpInitializeCbCtx RpInitializeCbCtx;
struct _RpInitializeCbCtx {
    RpLocalClusterParams local_cluster_params;
    RpClusterManagerImpl* cluster_manager_impl;
};
static inline RpInitializeCbCtx
rp_initialize_cb_ctx_ctor(RpLocalClusterParams local_cluster_params, RpClusterManagerImpl* cluster_manager_impl)
{
    RpInitializeCbCtx self = {
        .local_cluster_params = local_cluster_params,
        .cluster_manager_impl = cluster_manager_impl
    };
    return self;
}

struct _RpClusterManagerImpl {
    GObject parent_instance;

    RpServerInstance* m_server;
    RpClusterManagerFactory* m_factory;
    //TODO...ThreadLocal::TypedSlot<ThreadLocalClusterManagerImpl> tls_;
    RpSlotPtr m_tls;
    RpDispatcher* m_dispatcher;
    RpTimeSource* m_time_source;

    RpClusterMap m_warming_clusters;
    RpClusterMap m_active_clusters;

    RpClusterManagerInitHelper* m_init_helper;

    RpInitializeCbCtx m_initialize_cb_ctx;

    gchar* m_local_cluster_name;

    bool m_initialized : 1;
    bool m_deferred_cluster_creation : 1;
};

static void cluster_manager_iface_init(RpClusterManagerInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpClusterManagerImpl, rp_cluster_manager_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_CLUSTER_MANAGER, cluster_manager_iface_init)
)

#if 0
static bool
deferral_is_supported_for_cluster(RpClusterManagerImpl* self, RpClusterInfoConstSharedPtr info)
{
    NOISY_MSG_("(%p, %p)", self, info);

    if (!self->m_deferred_cluster_creation)
    {
        NOISY_MSG_("nope");
        return false;
    }

    //TODO..."envoy.cluster.static"...
return false;
}
#endif//0

typedef struct _RpUpdateCbCtx RpUpdateCbCtx;
struct _RpUpdateCbCtx {
    RpClusterInfoConstSharedPtr info;
    RpThreadLocalClusterUpdateParams params;
    RpLoadBalancerFactorySharedPtr load_balancer_factory;
    bool add_or_update_cluster;
};
static inline RpUpdateCbCtx
rp_update_cb_ctx_ctor(RpClusterInfoConstSharedPtr info, RpThreadLocalClusterUpdateParams params,
                        RpLoadBalancerFactorySharedPtr load_balancer_factory, bool add_or_update_cluster)
{
    RpUpdateCbCtx self = {
        .info = info,
        .params = params,
        .load_balancer_factory = load_balancer_factory,
        .add_or_update_cluster = add_or_update_cluster
    };
    return self;
}
static inline RpUpdateCbCtx*
rp_update_cb_ctx_new(RpClusterInfoConstSharedPtr info, RpThreadLocalClusterUpdateParams params,
                        RpLoadBalancerFactorySharedPtr load_balancer_factory, bool add_or_update_cluster)
{
    RpUpdateCbCtx* self = g_new(RpUpdateCbCtx, 1);
    *self = rp_update_cb_ctx_ctor(info, params, load_balancer_factory, add_or_update_cluster);
    return self;
}

static void
update_cb(RpThreadLocalObjectSharedPtr obj, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", obj, arg);

    RpThreadLocalClusterManagerImpl* cluster_manager = RP_THREAD_LOCAL_CLUSTER_MANAGER_IMPL(obj);
    RpUpdateCbCtx* ctx = arg;
    RpClusterInfoConstSharedPtr info = ctx->info;
    RpLoadBalancerFactorySharedPtr load_balancer_factory = ctx->load_balancer_factory;
    bool add_or_update_cluster = ctx->add_or_update_cluster;
    RpClusterEntry* new_cluster = NULL;
    if (add_or_update_cluster)
    {
        GHashTable* thread_local_clusters_ = rp_thread_local_cluster_manager_impl_thread_local_clusters_(cluster_manager);
        const char* name = rp_cluster_info_name(info);
        if (g_hash_table_contains(thread_local_clusters_, name))
        {
            NOISY_MSG_("updating TLS cluster \"%s\"", name);
        }
        else
        {
            NOISY_MSG_("adding TLS cluster \"%s\"", name);
        }

        new_cluster = rp_cluster_entry_new(cluster_manager, info, load_balancer_factory);
        g_hash_table_replace(thread_local_clusters_, g_strdup(name), new_cluster);
        //TODO...
    }
    //TODO...

    if (new_cluster)
    {
        //TODO...
    }
}

static void
completed_cb(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    RpUpdateCbCtx* ctx = arg;
    RpThreadLocalClusterUpdateParams params = ctx->params;
    g_clear_pointer(&params.m_hosts_added, g_ptr_array_unref);
    g_clear_pointer(&params.m_hosts_removed, g_ptr_array_unref);
    g_free(ctx);
}

#if 0
static RpClusterInitializationObjectConstSharedPtr
add_or_update_cluster_initialization_object_if_supported(RpClusterManagerImpl* self, const RpThreadLocalClusterUpdateParams* params,
                                                            RpClusterInfoConstSharedPtr cluster_info, RpLoadBalancerFactorySharedPtr load_balancer_factory,
                                                            RpHostMapConstSharedPtr map)
{
    NOISY_MSG_("(%p, %p, %p, %p, %p)", self, params, cluster_info, load_balancer_factory, map);

    if (!deferral_is_supported_for_cluster(self, cluster_info))
    {
        NOISY_MSG_("nope");
        return NULL;
    }

//TODO...
return NULL;
}
#endif//0

static void
post_thread_local_cluster_update(RpClusterManagerImpl* self, RpClusterManagerCluster* cm_cluster, RpThreadLocalClusterUpdateParams params)
{
    NOISY_MSG_("(%p, %p, ...)", self, cm_cluster);

    bool add_or_update_cluster = false;
    if (!rp_cluster_manager_cluster_added_or_updated(cm_cluster))
    {
        add_or_update_cluster = true;
        rp_cluster_manager_cluster_set_added_or_updated(cm_cluster);
    }

    RpLoadBalancerFactorySharedPtr load_balancer_factory = NULL;
    if (add_or_update_cluster)
    {
        NOISY_MSG_("getting load_balancer_factory");
        load_balancer_factory = rp_cluster_manager_cluster_load_balancer_factory(cm_cluster);
NOISY_MSG_("load balancer factory %p", load_balancer_factory);
    }

    //TODO...

    IF_NOISY_(RpHostMapConstSharedPtr host_map = rp_priority_set_cross_priority_host_map(
                                            rp_cluster_priority_set(
                                                rp_cluster_manager_cluster_cluster(cm_cluster)));)
NOISY_MSG_("host map %p", host_map);

    //TODO...pending_cluster_creations_.erase(...)

    RpClusterInfoConstSharedPtr info = rp_cluster_info(
                                            rp_cluster_manager_cluster_cluster(cm_cluster));
    RpUpdateCbCtx* ctx = rp_update_cb_ctx_new(info, params, load_balancer_factory, add_or_update_cluster);
    rp_slot_run_on_all_threads_completed(self->m_tls, update_cb, completed_cb, ctx);
}

static inline RpLbPolicy_e
translate_lb_policy(rule_cfg_t* cfg)
{
    NOISY_MSG_("(%p)", cfg);
    switch (cfg->lb_method)
    {
        case lb_method_rr:
        case lb_method_none:
        default:
            NOISY_MSG_("round_robin");
            return RpLbPolicy_ROUND_ROBIN;
        case lb_method_most_idle:
            NOISY_MSG_("most_idle");
            return RpLbPolicy_LEAST_REQUEST;
        case lb_method_rand:
            NOISY_MSG_("random");
            return RpLbPolicy_RANDOM;
    }
}

static inline RpDiscoveryType_e
translate_discovery_type(rule_cfg_t* cfg)
{
    NOISY_MSG_("(%p)", cfg);
    switch (cfg->discovery_type)
    {
        default:
        case discovery_type_static:
            NOISY_MSG_("static");
            return RpDiscoveryType_STATIC;
        case discovery_type_strict_dns:
            NOISY_MSG_("strict_dns");
            return RpDiscoveryType_STRICT_DNS;
        case discovery_type_local_dns:
            NOISY_MSG_("local_dns");
            return RpDiscoveryType_LOCAL_DNS;
        case discovery_type_eds:
            NOISY_MSG_("eds");
            return RpDiscoveryType_EDS;
        case discovery_type_original_dst:
            NOISY_MSG_("original_dst");
            return RpDiscoveryType_ORIGINAL_DST;
    }
}

static void
cluster_warming_to_active(RpClusterManagerImpl* self, const char* cluster_name)
{
    NOISY_MSG_("(%p, %p(%s))", self, cluster_name, cluster_name);
    RpClusterDataPtr warming = g_hash_table_lookup(self->m_warming_clusters, cluster_name);
    g_assert(warming);

    //TODO...updates_map_.erase(cluster_name);

    g_hash_table_replace(self->m_active_clusters, g_strdup(cluster_name), warming);
    g_hash_table_remove(self->m_warming_clusters, cluster_name);
}

static inline RpLocalClusterParams*
get_local_cluster_params(RpLocalClusterParams* local_cluster_params)
{
    return local_cluster_params->m_info ? local_cluster_params : NULL;
}

static RpThreadLocalObjectSharedPtr
initialize_cb(RpDispatcher* dispatcher, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", dispatcher, arg);
    RpInitializeCbCtx* ctx = arg;
    RpLocalClusterParams* local_cluster_params = get_local_cluster_params(&ctx->local_cluster_params);
    return (RpThreadLocalObjectSharedPtr)rp_thread_local_cluster_manager_impl_new(ctx->cluster_manager_impl,
                                                                                    dispatcher,
                                                                                    local_cluster_params);
}

static RpClusterDataPtr
load_cluster(RpClusterManagerImpl* self, RpClusterCfg* cluster, bool added_via_api, RpClusterMap cluster_map, GError** err)
{
    NOISY_MSG_("(%p, %p, %u, %p, %p)", self, cluster, added_via_api, cluster_map, err);

    PairClusterSharedPtrThreadAwareLoadBalancerPtr
        new_cluster_pair = rp_cluster_manager_factory_cluster_from_proto(self->m_factory,
                                                                            cluster,
                                                                            RP_CLUSTER_MANAGER(self),
                                                                            added_via_api);

    if (!new_cluster_pair.first)
    {
        g_set_error(err, G_FILE_ERROR, G_FILE_ERROR_INVAL, "invalid argument");
        return NULL;
    }

    g_autoptr(RpCluster) new_cluster = new_cluster_pair.first;
    g_autoptr(RpThreadAwareLoadBalancer) lb = new_cluster_pair.second;
    RpClusterInfoConstSharedPtr cluster_info = rp_cluster_info(new_cluster);
    const char* name = rp_cluster_info_name(cluster_info);

    if (!added_via_api)
    {
        if (g_hash_table_lookup(cluster_map, name))
        {
            g_set_error(err, G_FILE_ERROR, G_FILE_ERROR_INVAL, "cluster manager: duplicate cluster \"%s\"", name);
            return NULL;
        }
    }

    RpClusterDataPtr new_cluster_data = rp_cluster_data_new(cluster, added_via_api, g_steal_pointer(&new_cluster), self->m_time_source);
    RpClusterDataPtr result = g_hash_table_lookup(cluster_map, name);
    if (g_hash_table_replace(cluster_map, g_strdup(name), new_cluster_data))
    {
        // Key did NOT already exist.
        result = NULL;
    }

    //TODO...more complete lb stuff...
    RpThreadAwareLoadBalancerPtr* thread_aware_lb_ = rp_cluster_data_thread_aware_lb_(new_cluster_data);
    *thread_aware_lb_ = g_steal_pointer(&lb);

    //TODO..updateClusterCounts();
    return result;
}

static RpClusterManagerFactory*
cluster_manager_factory_i(RpClusterManager* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_MANAGER_FACTORY(self);
}

static void
drain_connections_i(RpClusterManager* self, const char* cluster, RpDrainConnectionsHostPredicate predicate)
{
    NOISY_MSG_("(%p, %p(%s), %p)", self, cluster, cluster, predicate);
//TODO...
//cluster == NULL == "drain all clusters"
}

static RpThreadLocalCluster*
get_thread_local_cluster_i(RpClusterManager* self, const char* cluster)
{
    NOISY_MSG_("(%p, %p(%s))", self, cluster, cluster);

    // Get the thread local cluster manage from the thread local slot.
    RpThreadLocalClusterManagerImpl* cluster_manager = RP_THREAD_LOCAL_CLUSTER_MANAGER_IMPL(
                                                            rp_slot_get(RP_CLUSTER_MANAGER_IMPL(self)->m_tls));

    GHashTable* thread_local_clusters_ = rp_thread_local_cluster_manager_impl_thread_local_clusters_(cluster_manager);
    RpClusterEntry* entry = g_hash_table_lookup(thread_local_clusters_, cluster);
    if (entry)
    {
        NOISY_MSG_("found entry %p", entry);
        return RP_THREAD_LOCAL_CLUSTER(entry);
    }
    return RP_THREAD_LOCAL_CLUSTER(
        rp_thread_local_cluster_manager_impl_initialize_cluster_inline_if_exists(cluster_manager, cluster));
}

static void
gh_func(gpointer key G_GNUC_UNUSED, gpointer value, gpointer user_data)
{
    NOISY_MSG_("(%p, %p, %p)", key, value, user_data);

    RpClusterManagerImpl* self = user_data;
    rp_cluster_manager_init_helper_add_cluster(self->m_init_helper, RP_CLUSTER_MANAGER_CLUSTER(value));
}

typedef struct _RpProcessRuleCbCtx RpProcessRuleCbCtx;
struct _RpProcessRuleCbCtx {
    RpClusterManagerImpl* cluster_manager;
    GError** err;
};
static inline RpProcessRuleCbCtx
rp_process_rule_cb_ctx_ctor(RpClusterManagerImpl* cluster_manager, GError** err)
{
    RpProcessRuleCbCtx self = {
        .cluster_manager = cluster_manager,
        .err = err
    };
    return self;
}


#if 0
static RpLbEndpointCfg*
rp_lb_endpoint_cfg_new(RpEndpointCfg endpoint, guint32 load_balancing_weight)
{
    RpLbEndpointCfg* self = g_new(RpLbEndpointCfg, 1);
    self->host_identifier.endpoint = endpoint;
    self->load_balancing_weight = load_balancing_weight;
    return self;
}
static RpLocalityLbEndpointsCfg*
rp_locality_lb_endpoints_new(rule_t* rule, guint32 load_balancing_weight, guint32 priority)
{
    RpLocalityLbEndpointsCfg* self = g_new(RpLocalityLbEndpointsCfg, 1);
    self->lb_endpoints = g_ptr_array_new_full(lztq_size(rule->upstreams), g_free);
    return self;
}
static void
rp_locality_lb_endpoints_free(gpointer arg)
{
    RpLocalityLbEndpointsCfg* self = arg;
    g_ptr_array_free(self->lb_endpoints, true);
    g_free(self);
}
static GPtrArray*
create_cluster_load_assignment_endpoints(rule_t* rule)
{
    NOISY_MSG_("(%p)", rule);
    g_autoptr(GPtrArray) endpoints = g_ptr_array_new_full(lztq_size(rule->upstreams), rp_locality_lb_endpoints_free);
    for (lztq_elem* elem = lztq_first(rule->upstreams); elem; elem = lztq_next(elem))
    {
        upstream_t* upstream = lztq_elem_data(elem);
        g_ptr_array_add(endpoints, rp_locality_lb_endpoints_new(rule, 1, 0));
    }
    return g_steal_pointer(&endpoints);
}
#endif//0

static int
process_rule_cb(lztq_elem* elem, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", elem, arg);
    RpProcessRuleCbCtx* ctx = arg;
    RpClusterManagerImpl* me = ctx->cluster_manager;
    GError** err = ctx->err;
    rule_t* rule = lztq_elem_data(elem);
    rule_cfg_t* rule_cfg = rule->config;
    RpDiscoveryType_e cluster_discovery_type = translate_discovery_type(rule_cfg);
    gchar* name = g_strdup_printf("%p", rule_cfg);

    NOISY_MSG_("rule %p, rule->config %p, name %p(%s)", rule, rule_cfg, name, name);

    RpLocalityLbEndpointsCfg locality_lb_endpoints = {
        .lb_endpoints = rule->upstreams,
        .load_balancing_weight = 1,
        .priority = 0
    };
    RpClusterCfg cluster = {
        .preconnect_policy = {
            .per_upstream_preconnect_ratio = 1.0,
            .predictive_preconnect_ratio = 1.0
        },
        .name = name,
        .type = cluster_discovery_type,
        .connect_timeout_secs = 5,
        .per_connection_buffer_limit_bytes = 1024*1024,
        .lb_policy = translate_lb_policy(rule_cfg),
        .dns_lookup_family = RpDnsLookupFamily_AUTO,
        .connection_pool_per_downstream_connection = false,
        .endpoints = locality_lb_endpoints,
        .rule = rule
    };

    RpClusterDataPtr new_cluster = load_cluster(me, &cluster, /*added_via_api=*/false, me->m_active_clusters, err);
    if (err && *err)
    {
        LOGE("error %d(%s)", (*err)->code, (*err)->message);
        return -1;
    }
    if (new_cluster)
    {
        NOISY_MSG_("replaced cluster data %p", new_cluster);
        g_clear_object(&new_cluster);
    }
    return 0;
}

static bool
initialize_i(RpClusterManager* self, rproxy_t* bootstrap, GError** err)
{
    NOISY_MSG_("(%p, %p, %p)", self, bootstrap, err);

    RpClusterManagerImpl* me = RP_CLUSTER_MANAGER_IMPL(self);
    g_assert(!me->m_initialized);
    me->m_initialized = true;

    //TODO...

    RpProcessRuleCbCtx ctx = rp_process_rule_cb_ctx_ctor(me, err);
    if (lztq_for_each(bootstrap->rules, process_rule_cb, &ctx) != 0)
    {
        LOGE("failed");
        return false;
    }

    RpLocalClusterParams local_cluster_params = {0};
    if (me->m_local_cluster_name && me->m_local_cluster_name[0])
    {
        RpClusterManagerCluster* local_cluster = g_hash_table_lookup(me->m_active_clusters, me->m_local_cluster_name);
        if (!local_cluster)
        {
            g_set_error(err, G_FILE_ERROR, G_FILE_ERROR_INVAL, "local cluster \"%s\" must be defined", me->m_local_cluster_name);
            return false;
        }
        local_cluster_params.m_info = rp_cluster_info(
                                        rp_cluster_manager_cluster_cluster(local_cluster));
        local_cluster_params.m_load_balancer_factory = rp_cluster_manager_cluster_load_balancer_factory(local_cluster);
        rp_cluster_manager_cluster_set_added_or_updated(local_cluster);
    }

    me->m_initialize_cb_ctx = rp_initialize_cb_ctx_ctor(local_cluster_params, me);
    rp_slot_set(me->m_tls, initialize_cb, &me->m_initialize_cb_ctx);

    //TODO...

    g_hash_table_foreach(me->m_active_clusters, gh_func, me);

    rp_cluster_manager_init_helper_on_static_load_complete(me->m_init_helper);

    return true;
}

static bool
initialized_i(RpClusterManager* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_MANAGER_IMPL(self)->m_initialized;
}

static bool
add_or_update_cluster_i(RpClusterManager* self, RpClusterCfg* cluster, const char* version_info)
{
    NOISY_MSG_("(%p, %p, %p(%s))", self, cluster, version_info, version_info);

    const char* name = cluster->name;
    NOISY_MSG_("name \"%s\"", name);
    RpClusterManagerImpl* me = RP_CLUSTER_MANAGER_IMPL(self);
    RpClusterDataPtr existing_active_cluster = g_hash_table_lookup(me->m_active_clusters, name);
    //TODO...warming_clusters
    if (existing_active_cluster)
    {
        //TODO...init_helper_.removeCluster(*existing_active_cluster->second);
        //TODO...cm_stats_.cluster_modified_.inc();
    }
    else
    {
        //TODO...cm_stats_.cluster_added_.inc();
    }
    RpClusterDataPtr previous_cluster = load_cluster(me, cluster, /*added_via_api*/true, me->m_warming_clusters, NULL);
    if (previous_cluster)
    {
        NOISY_MSG_("replaced cluster %p",
            rp_cluster_manager_cluster_cluster(RP_CLUSTER_MANAGER_CLUSTER(previous_cluster)));
        g_clear_object(&previous_cluster);
    }
    return true;
}

static RpClusterUpdateCallbacksHandlePtr
add_thread_local_cluster_update_callbacks_i(RpClusterManager* self, RpClusterUpdateCallbacks* cb)
{
    NOISY_MSG_("(%p, %p)", self, cb);
    RpThreadLocalClusterManagerImpl* cluster_manager = RP_THREAD_LOCAL_CLUSTER_MANAGER_IMPL(
                                                            rp_slot_get(RP_CLUSTER_MANAGER_IMPL(self)->m_tls));
    return rp_cluster_lifecycle_callback_handler_add_cluster_update_callbacks(
            RP_CLUSTER_LIFECYCLE_CALLBACK_HANDLER(cluster_manager), cb);
}

static void
cluster_manager_iface_init(RpClusterManagerInterface* iface)
{
    LOGD("(%p)", iface);
    iface->cluster_manager_factory = cluster_manager_factory_i;
    iface->drain_connections = drain_connections_i;
    iface->get_thread_local_cluster = get_thread_local_cluster_i;
    iface->initialize = initialize_i;
    iface->initialized = initialized_i;
    iface->add_or_update_cluster = add_or_update_cluster_i;
    iface->add_thread_local_cluster_update_callbacks = add_thread_local_cluster_update_callbacks_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpClusterManagerImpl* self = RP_CLUSTER_MANAGER_IMPL(obj);
    g_clear_pointer(&self->m_active_clusters, g_hash_table_unref);
    g_clear_pointer(&self->m_warming_clusters, g_hash_table_unref);
    g_clear_object(&self->m_tls);
    g_clear_object(&self->m_init_helper);

    G_OBJECT_CLASS(rp_cluster_manager_impl_parent_class)->dispose(obj);
}

static void
rp_cluster_manager_impl_class_init(RpClusterManagerImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static RpStatusCode_e
on_cluster_init(RpClusterManager* self, RpClusterManagerCluster* cm_cluster)
{
    NOISY_MSG_("(%p, %p)", self, cm_cluster);
    RpClusterManagerImpl* me = RP_CLUSTER_MANAGER_IMPL(self);
    RpCluster* cluster = rp_cluster_manager_cluster_cluster(cm_cluster);
    const char* name = rp_cluster_info_name(rp_cluster_info(cluster));
    RpClusterDataPtr cluster_data = g_hash_table_lookup(me->m_warming_clusters, name);
    if (cluster_data)
    {
        cluster_warming_to_active(me, name);
        //TODO...updateClusterCounts();
    }
    cluster_data = g_hash_table_lookup(me->m_active_clusters, name);

    if (rp_cluster_data_thread_aware_lb_(cluster_data))
    {
        //TODO...RETURN_IF_NOT_OK(cluster_data->second->thread_aware_lb_->initialize());
    }
    //TODO...

    RpThreadLocalClusterUpdateParams params = rp_thread_local_cluster_update_params_ctor(RpResourcePriority_Default, NULL, NULL);
    post_thread_local_cluster_update(me, cm_cluster, params);
    return RpStatusCode_Ok;
}

static void
rp_cluster_manager_impl_init(RpClusterManagerImpl* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_active_clusters = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    self->m_warming_clusters = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    self->m_init_helper = rp_cluster_manager_init_helper_new(RP_CLUSTER_MANAGER(self),
                                                                on_cluster_init);
    self->m_deferred_cluster_creation = false;//TODO...bootstrap.enable_deferred_cluster_creation();
}

RpClusterManagerImpl*
rp_cluster_manager_impl_new(rproxy_t* bootstrap G_GNUC_UNUSED, RpClusterManagerFactory* factory,
                            RpCommonFactoryContext* context G_GNUC_UNUSED, RpThreadLocalInstance* tls,
                            RpDispatcher* main_thread_dispatcher, RpServerInstance* server, RpStatusCode_e* creation_status)
{
    LOGD("(%p, %p, %p, %p, %p, %p, %p)",
        bootstrap, factory, context, tls, main_thread_dispatcher, server, creation_status);

    g_return_val_if_fail(bootstrap != NULL, NULL);
    g_return_val_if_fail(RP_IS_CLUSTER_MANAGER_FACTORY(factory), NULL);
    g_return_val_if_fail(RP_IS_COMMON_FACTORY_CONTEXT(context), NULL);
    g_return_val_if_fail(RP_IS_THREAD_LOCAL_INSTANCE(tls), NULL);
    g_return_val_if_fail(RP_IS_DISPATCHER(main_thread_dispatcher), NULL);
    g_return_val_if_fail(RP_IS_SERVER_INSTANCE(server), NULL);

    RpClusterManagerImpl* self = g_object_new(RP_TYPE_CLUSTER_MANAGER_IMPL, NULL);
    self->m_factory = factory;
    self->m_tls = rp_slot_allocator_allocate_slot(RP_SLOT_ALLOCATOR(tls));
    self->m_server = server;
    self->m_dispatcher = main_thread_dispatcher;
    self->m_time_source = rp_dispatcher_time_source(main_thread_dispatcher);

    if (creation_status) *creation_status = RpStatusCode_Ok;

    return self;
}

RpClusterManagerFactory*
rp_cluster_manager_impl_factory_(RpClusterManagerImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CLUSTER_MANAGER_IMPL(self), NULL);
    return self->m_factory;
}
