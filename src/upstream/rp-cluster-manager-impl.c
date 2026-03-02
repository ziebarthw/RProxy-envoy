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
#include "rp-host-map-snap.h"
#include "rp-host-set-ptr-vector.h"
#include "rp-load-balancer.h"
#include "rp-thread-local-cluster.h"
#include "upstream/rp-cluster-manager-impl.h"

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
    RpSlotPtr m_tls; /* borrowed */
    RpDispatcher* m_dispatcher;
    RpTimeSource* m_time_source;

    RpClusterMap m_warming_clusters;
    RpClusterMap m_active_clusters;
    RpClusterInitializationMap m_cluster_initialization_map;

    RpClusterManagerInitHelper* m_init_helper;

    RpInitializeCbCtx m_initialize_cb_ctx;

    GHashTable* m_all_clusters;

    gchar* m_local_cluster_name;

    bool m_initialized : 1;
    bool m_deferred_cluster_creation : 1;
    bool m_shutdown : 1;
};

static void cluster_manager_iface_init(RpClusterManagerInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpClusterManagerImpl, rp_cluster_manager_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_CLUSTER_MANAGER, cluster_manager_iface_init)
)

/* FNV-1a 64-bit - simple, fast, good distribution */
static inline guint64
fnv1a_64_update(guint64 hash, const void* data, gsize len)
{
    const guint8 *bytes = data;
    for (size_t i = 0; i < len; i++)
    {
        hash ^= bytes[i];
        hash *= 0x100000001b3ULL;
    }
    return hash;
}
#define FNV1A_64_INIT 0xcbf29ce484222325ULL
static inline guint64
config_hash(const RpClusterCfg* cluster)
{
    NOISY_MSG_("(%p)", cluster);
    return fnv1a_64_update(FNV1A_64_INIT, cluster, sizeof(*cluster));
}

static void
cluster_warming_to_active(RpClusterManagerImpl* self, const char* cluster_name)
{
    NOISY_MSG_("(%p, %p(%s))", self, cluster_name, cluster_name);
    RpClusterDataPtr warming = g_hash_table_lookup(self->m_warming_clusters, cluster_name);
    g_assert(warming);

    //TODO...updates_map_.erase(cluster_name);

    g_hash_table_replace(self->m_active_clusters, g_strdup(cluster_name), g_object_ref(warming));
    g_hash_table_remove(self->m_warming_clusters, cluster_name);
}

typedef struct _RpUpdateCbCtx RpUpdateCbCtx;
struct _RpUpdateCbCtx {
    RpClusterInfoSharedPtr info;
    RpThreadLocalClusterUpdateParams params;
    RpLoadBalancerFactorySharedPtr load_balancer_factory;
    RpHostMapSnap* map;
    RpClusterInitializationObjectSharedPtr cluster_initialization_object;
    bool add_or_update_cluster;
};
static inline RpUpdateCbCtx
rp_update_cb_ctx_ctor_take(RpClusterInfoConstSharedPtr info, RpThreadLocalClusterUpdateParams* params,
                            RpLoadBalancerFactorySharedPtr load_balancer_factory, RpHostMapSnap* map,
                            RpClusterInitializationObjectConstSharedPtr cluster_initialization_object, bool add_or_update_cluster)
{
    RpUpdateCbCtx self = {
        .params = {0},
        .info = NULL,
        .load_balancer_factory = NULL,
        .map = map, // STEAL ownership (no ref),
        .cluster_initialization_object = NULL,
        .add_or_update_cluster = add_or_update_cluster
    };
    RP_SHARE_OBJ(&self.info, info);
    RP_SHARE_OBJ(&self.load_balancer_factory, load_balancer_factory);
    RP_SHARE_OBJ(&self.cluster_initialization_object, cluster_initialization_object);
    self.params.m_per_priority_update_params = g_steal_pointer(&params->m_per_priority_update_params);
    return self;
}
static inline RpUpdateCbCtx*
rp_update_cb_ctx_new_take(RpClusterInfoConstSharedPtr info, RpThreadLocalClusterUpdateParams* params,
                            RpLoadBalancerFactorySharedPtr load_balancer_factory, RpHostMapSnap* map /* Transfer full. */,
                            RpClusterInitializationObjectConstSharedPtr cluster_initialization_object, bool add_or_update_cluster)
{
    RpUpdateCbCtx* self = g_new0(RpUpdateCbCtx, 1);
    *self = rp_update_cb_ctx_ctor_take(info, params, load_balancer_factory, map, cluster_initialization_object, add_or_update_cluster);
    return self;
}
static inline void
rp_update_cb_ctx_dtor(RpUpdateCbCtx* self)
{
    g_return_if_fail(self != NULL);
    g_clear_object(&self->info);
    g_clear_object(&self->load_balancer_factory);
    g_clear_pointer(&self->map, rp_host_map_snap_unref);
    g_clear_object(&self->cluster_initialization_object);
    rp_thread_local_cluster_update_params_dtor(&self->params);
}
static inline void
rp_update_cb_ctx_free(RpUpdateCbCtx* self)
{
    g_return_if_fail(self != NULL);
    rp_update_cb_ctx_dtor(self);
    g_free(self);
}

static void
update_cb(RpThreadLocalObjectSharedPtr obj, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", obj, arg);

    RpThreadLocalClusterManagerImpl* cluster_manager = RP_THREAD_LOCAL_CLUSTER_MANAGER_IMPL(obj);
    RpUpdateCbCtx* ctx = arg;
    RpClusterInfoConstSharedPtr info = ctx->info;
    RpLoadBalancerFactorySharedPtr load_balancer_factory = ctx->load_balancer_factory;
    RpClusterInitializationObjectConstSharedPtr cluster_initialization_object = ctx->cluster_initialization_object;
    RpHostMapSnap* map = ctx->map;
    const char* cluster_name = rp_cluster_info_name(info);
    bool add_or_update_cluster = ctx->add_or_update_cluster;
    bool defer_unused_clusters = cluster_initialization_object &&
                                !rp_thread_local_cluster_manager_impl_thread_local_clusters_contains(cluster_manager, cluster_name) /*&&
                                TODO...!Envoy::Thread::MainThread::isMainThread()*/;
    if (defer_unused_clusters)
    {
NOISY_MSG_("TODO...defer unused clusters");
    }
    else
    {
        RpThreadLocalClusterUpdateParams* params = &ctx->params;
        RpClusterEntry* new_cluster = NULL;
        const char* name = rp_cluster_info_name(info);
        if (add_or_update_cluster)
        {
            GHashTable* thread_local_clusters_ = rp_thread_local_cluster_manager_impl_thread_local_clusters_(cluster_manager);
            if (g_hash_table_contains(thread_local_clusters_, name))
            {
                NOISY_MSG_("updating TLS cluster \"%s\"", name);
            }
            else
            {
                NOISY_MSG_("adding TLS cluster \"%s\"", name);
            }

            new_cluster = rp_cluster_entry_new(cluster_manager, info, load_balancer_factory);
            // thread_local_clusters_ owns new_cluster.
            g_hash_table_replace(thread_local_clusters_, g_strdup(name), new_cluster);
            //TODO...
        }
        //TODO...

        GArray* per_priority_update_params_ = params->m_per_priority_update_params;
        for (gint i = 0; i < per_priority_update_params_->len; ++i)
        {
            RpTlcPerPriority* per_priority = &g_array_index(per_priority_update_params_, RpTlcPerPriority, i);
            rp_thread_local_cluster_manager_impl_update_cluster_membership(cluster_manager,
                                                                            name,
                                                                            per_priority->m_priority,
                                                                            per_priority->m_update_hosts_params,
                                                                            per_priority->m_hosts_added,
                                                                            per_priority->m_hosts_removed,
                                                                            per_priority->m_weighted_priority_health,
                                                                            per_priority->m_overprovisioning_factor,
                                                                            map);
        }

        if (new_cluster)
        {
            //TODO...ThreadLocalClusterCommand command = [&new_cluster]() ->...
            const char* name = rp_cluster_info_name(info);
            GList* update_callbacks_ = rp_thread_local_cluster_manager_impl_update_callbacks_(cluster_manager);
            for (GList* cb_it = update_callbacks_; cb_it; )
            {
                GList* curr_cb_it = cb_it;
                cb_it = cb_it->next;
                rp_cluster_update_callbacks_on_cluster_add_or_update(RP_CLUSTER_UPDATE_CALLBACKS(curr_cb_it->data), name, NULL/*TODO...command*/);
            }
        }
    }
}

static void
completed_cb(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    RpUpdateCbCtx* ctx = arg;
    rp_update_cb_ctx_free(ctx);
}

static bool
deferral_is_supported_for_cluster(RpClusterManagerImpl* self, RpClusterInfoConstSharedPtr info)
{
    NOISY_MSG_("(%p, %p)", self, info);

    if (!self->m_deferred_cluster_creation)
    {
        NOISY_MSG_("nope");
        return false;
    }

    const RpCustomClusterTypeCfg* custom_cluster_type = rp_cluster_info_cluster_type(info);
    if (custom_cluster_type)
    {
        static const char* supported_well_known_types[] = {
            "rproxy.clusters.static",
            "rproxy.clusters.aggregate",
            "rproxy.clusters.eds",
            "rproxy.clusters.redis"
        };
        static const gsize num_elems = sizeof(supported_well_known_types)/sizeof(supported_well_known_types[0]);
        for (gsize i = 0; i < num_elems; ++i)
        {
NOISY_MSG_("custom cluster type \"%s\"", custom_cluster_type->name);
            if (g_ascii_strcasecmp(supported_well_known_types[i], custom_cluster_type->name) == 0)
            {
                NOISY_MSG_("yep");
                return true;
            }
        }
    }
    else
    {
        static RpDiscoveryType_e supported_cluster_types[] = {
            RpDiscoveryType_EDS,
            RpDiscoveryType_STATIC
        };
        static const gsize num_elems = sizeof(supported_cluster_types)/sizeof(supported_cluster_types[0]);
        for (gsize i = 0; i < num_elems; ++i)
        {
            if (rp_cluster_info_type(info) == supported_cluster_types[i])
            {
                NOISY_MSG_("yep");
                return true;
            }
        }
    }
    NOISY_MSG_("nope");
    return false;
}

static RpClusterInitializationObjectConstSharedPtr
add_or_update_cluster_initialization_object_if_supported(RpClusterManagerImpl* self, const RpThreadLocalClusterUpdateParams* params,
                                                            RpClusterInfoConstSharedPtr cluster_info, RpLoadBalancerFactorySharedPtr load_balancer_factory,
                                                            RpHostMapSnap* map)
{
    NOISY_MSG_("(%p, %p, %p, %p, %p)", self, params, cluster_info, load_balancer_factory, map);

    if (!deferral_is_supported_for_cluster(self, cluster_info))
    {
        NOISY_MSG_("nope");
        return NULL;
    }

    const char* cluster_name = rp_cluster_info_name(cluster_info);
    RpClusterInitializationObjectConstSharedPtr entry = g_hash_table_lookup(self->m_cluster_initialization_map, cluster_name);
    bool should_merge_with_prior_cluster = entry != NULL &&
        rp_cluster_initialization_object_cluster_info_(entry) == cluster_info;

    RpClusterInitializationObject* new_initialization_object;
    if (should_merge_with_prior_cluster)
    {
        RpLoadBalancerFactorySharedPtr lb_factory = load_balancer_factory ?
            load_balancer_factory : rp_cluster_initialization_object_cluster_load_balancer_factory_(entry);
        new_initialization_object = rp_cluster_initialization_object_new(params, cluster_info, lb_factory, map);
    }
    else
    {
        new_initialization_object = rp_cluster_initialization_object_new(params, cluster_info, load_balancer_factory, map);
    }
    // cluster initialization container owns new initialization object.
    g_hash_table_replace(self->m_cluster_initialization_map, g_strdup(cluster_name), new_initialization_object);
    return new_initialization_object;
}

static void
post_thread_local_cluster_update(RpClusterManagerImpl* self, RpClusterManagerCluster* cm_cluster, RpThreadLocalClusterUpdateParams* params)
{
    NOISY_MSG_("(%p, %p(%p), ...)", self, cm_cluster, rp_cluster_manager_cluster_cluster(cm_cluster));

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
    }

    RpCluster* cluster = rp_cluster_manager_cluster_cluster(cm_cluster);
    for (guint i = 0; i < params->m_per_priority_update_params->len; ++i)
    {
        RpTlcPerPriority* per_priority = &g_array_index(params->m_per_priority_update_params, RpTlcPerPriority, i);
        RpPrioritySet* priority_set = rp_cluster_priority_set(cluster);
        const RpHostSetPtrVector* host_sets = rp_priority_set_host_sets_per_priority(priority_set);
        RpHostSetPtr host_set = rp_host_set_ptr_vector_get(host_sets, per_priority->m_priority);
        per_priority->m_update_hosts_params = rp_host_set_impl_update_hosts_params_2(RP_HOST_SET_IMPL(host_set));
        per_priority->m_weighted_priority_health = rp_host_set_get_weighted_priority_health(host_set);
        per_priority->m_overprovisioning_factor = rp_host_set_get_overprovisioning_factor(host_set);
    }

    //TODO...

    g_autoptr(RpHostMapSnap) host_map = rp_priority_set_cross_priority_host_map(
                                            rp_cluster_priority_set(
                                                rp_cluster_manager_cluster_cluster(cm_cluster)));
    NOISY_MSG_("host map %p(%p/%u)", host_map, rp_host_map_snap_host_map(host_map), rp_host_map_size(rp_host_map_snap_host_map(host_map)));

    //TODO...pending_cluster_creations_.erase(...)

    RpClusterInfoConstSharedPtr info = rp_cluster_info(
                                        rp_cluster_manager_cluster_cluster(cm_cluster));
    RpClusterInitializationObjectConstSharedPtr cluster_initialization_object =
        add_or_update_cluster_initialization_object_if_supported(self, params, info, load_balancer_factory, host_map);

    RpUpdateCbCtx* ctx =
        rp_update_cb_ctx_new_take(info, params, load_balancer_factory, g_steal_pointer(&host_map), cluster_initialization_object, add_or_update_cluster);

g_assert(params->m_per_priority_update_params == NULL);

    /* params->m_per_priority_update_params is now NULL */
    rp_slot_run_on_all_threads_completed(self->m_tls, update_cb, completed_cb, ctx);
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

    if (rp_cluster_data_thread_aware_lb(cluster_data))
    {
        RpStatusCode_e status = rp_thread_aware_load_balancer_initialize(rp_cluster_data_thread_aware_lb(cluster_data));
        if (status != RpStatusCode_Ok)
        {
            LOGE("failed");
            return status;
        }
    }
    //TODO...

    RpThreadLocalClusterUpdateParams params = rp_thread_local_cluster_update_params_ctor();
    RpPrioritySet* priority_set = rp_cluster_priority_set(cluster);
    const RpHostSetPtrVector* host_sets = rp_priority_set_host_sets_per_priority(priority_set);
    for (guint i = 0; i < rp_host_set_ptr_vector_size(host_sets); ++i)
    {
        RpHostSetPtr host_set = rp_host_set_ptr_vector_get(host_sets, i);
        const RpHostVector* hosts = rp_host_set_get_hosts(host_set);
        if (rp_host_vector_is_empty(hosts))
        {
            NOISY_MSG_("empty");
            continue;
        }
        RpTlcPerPriority per_priority = {
            .m_hosts_added = hosts,  // borrowed
            .m_hosts_removed = NULL,
            .m_priority = rp_host_set_get_priority(host_set)
        };
        g_array_append_val(params.m_per_priority_update_params, per_priority);
    }

    post_thread_local_cluster_update(me, cm_cluster, &params);
//    rp_thread_local_cluster_update_params_dtor(&params);
    return RpStatusCode_Ok;
}

#if 0
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
#endif

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
load_cluster(RpClusterManagerImpl* self, const RpClusterCfg* cluster, guint64 cluster_hash,
                bool added_via_api, RpClusterMap cluster_map, GError** err)
{
    NOISY_MSG_("(%p, %p, %" G_GUINT64_FORMAT ", %u, %p, %p)",
        self, cluster, cluster_hash, added_via_api, cluster_map, err);

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

    RpTypedLoadBalancerFactory* typed_lb_factory = rp_cluster_info_load_balancer_factory(cluster_info);
    bool cluster_provided_lb = g_ascii_strcasecmp(
                                rp_typed_load_balancer_factory_name(typed_lb_factory), "rproxy.load_balancing_policies.cluster_provided") == 0;

    if (cluster_provided_lb && lb == NULL)
    {
        g_set_error(err, G_FILE_ERROR, G_FILE_ERROR_INVAL,
            "cluster manager: cluster provided LB specified but cluster \"%s\" did not provided one. Check cluster documentation.",
                rp_cluster_info_name(cluster_info));
        return NULL;
    }
    if (!cluster_provided_lb && lb != NULL)
    {
        g_set_error(err, G_FILE_ERROR, G_FILE_ERROR_INVAL,
            "cluster manager: cluster provided LB not specified but cluster \"%s\" provided one. Check cluster documentation.",
                rp_cluster_info_name(cluster_info));
        return NULL;
    }

    RpClusterDataPtr new_cluster_data = rp_cluster_data_new(cluster, cluster_hash, added_via_api, new_cluster, self->m_time_source);
    RpClusterDataPtr result = g_hash_table_lookup(cluster_map, name);
    // cluster_map owns new_cluster_data.
    if (g_hash_table_replace(cluster_map, g_strdup(name), new_cluster_data))
    {
        // Key did NOT already exist.
        result = NULL;
    }

    if (cluster_provided_lb)
    {
        rp_cluster_data_thread_aware_lb_take(new_cluster_data, &lb);
        g_assert(lb == NULL);
    }
    else
    {
        lb = rp_typed_load_balancer_factory_create(typed_lb_factory,
                                                    rp_cluster_info_load_balancer_config(cluster_info),
                                                    cluster_info,
                                                    rp_cluster_priority_set(new_cluster),
                                                    self->m_time_source);
        rp_cluster_data_thread_aware_lb_take(new_cluster_data, &lb);
        g_assert(lb == NULL);
    }

    // The all_clusters container owns new_cluster.
    g_hash_table_add(self->m_all_clusters, g_steal_pointer(&new_cluster));
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

static inline void
init_socket_address_cfg(RpSocketAddressCfg* self, upstream_t* upstream)
{
    NOISY_MSG_("(%p, %p)", self, upstream);
    // upstream->config->host might be null in the case of strict dns and
    // dynamic forward proxy clusters.
    const char* src = upstream->config->host ?
                        upstream->config->host : "0.0.0.0";
    rp_socket_address_cfg_set_address(self, src);
    rp_socket_address_cfg_set_port_value(self, upstream->config->port);
    self->ipv4_compat = true;
    self->network_namespace_filepath[0] = NUL;
    self->protocol = RpProtocol_TCP;
    self->resolver_name[0] = NUL;
}

static inline void
init_address_cfg(RpAddressCfg* self, upstream_t* upstream)
{
    NOISY_MSG_("(%p, %p)", self, upstream);
    init_socket_address_cfg(&self->address.socket_address, upstream);
}

static inline void
init_endpoint_cfg(RpEndpointCfg* self, upstream_t* upstream)
{
    NOISY_MSG_("(%p, %p)", self, upstream);
    init_address_cfg(&self->address, upstream);
    rp_endpoint_cfg_clear_additional_addresses(self);
    g_strlcpy(self->hostname, upstream->config->name, sizeof(self->hostname));
}

static inline void
init_lb_endpoint_cfg(RpLbEndpointCfg* self, upstream_t* upstream)
{
    NOISY_MSG_("(%p, %p)", self, upstream);
    init_endpoint_cfg(rp_lb_endpoint_cfg_mutable_endpoint(self), upstream);
    self->load_balancing_weight = 1;
    self->metadata = upstream;
}

static inline void
init_locality_lb_endpoints_cfg(RpLocalityLbEndpointsCfg* self, rule_t* rule)
{
    NOISY_MSG_("(%p, %p) %u upstreams", self, rule, g_slist_length(rule->upstreams));
    for (GSList* itr = rule->upstreams; itr; itr = itr->next)
    {
        upstream_t* upstream = itr->data;
        RpLbEndpointCfg* lb_endpoint = rp_locality_lb_endpoints_cfg_add_lb_endpoints(self);
        if (!lb_endpoint)
        {
            LOGE("too many upstreams!");
            break;
        }
        init_lb_endpoint_cfg(lb_endpoint, upstream);
    }
    self->load_balancing_weight = 1;
    self->priority = 0;
}

static inline void
init_preconnect_policy_cfg(RpPreconnectPolicyCfg* self)
{
    NOISY_MSG_("(%p)", self);
    self->per_upstream_preconnect_ratio = 1.0;
    self->predictive_preconnect_ratio = 1.0;
}

static inline void
init_lb_policy_cfg(RpLbPolicyCfg* self)
{
    NOISY_MSG_("(%p)", self);
    self->overprovisioning_factor = 1;
    self->endpoint_stale_after = 60/*REVISIT*/;
    self->weighted_priority_health = 1;
}

static inline void
init_cluster_load_assignment_cfg(RpClusterLoadAssignmentCfg* self, rule_t* rule)
{
    NOISY_MSG_("(%p, %p)", self, rule);
    init_lb_policy_cfg(&self->policy);
    rp_cluster_load_assignment_cfg_clear_endpoints(self);
    RpLocalityLbEndpointsCfg* lb_endpoint = rp_cluster_load_assignment_cfg_add_endpoints(self);
    init_locality_lb_endpoints_cfg(lb_endpoint, rule);
    g_snprintf(self->cluster_name, sizeof(self->cluster_name), "%p", rule->config);
}

static inline void
init_dfp_sub_clusters_config(RpDfpSubClustersCfg* self, dfp_sub_clusters_cfg_t* cfg)
{
    NOISY_MSG_("(%p, %p)", self, cfg);
//    self->lb_policy = cfg->lb_policy;
self->lb_policy = RpLbPolicy_CLUSTER_PROVIDED;//REVISIT to be config-driven.
    self->max_sub_clusters = cfg->max_sub_clusters;
    self->sub_cluster_ttl = cfg->sub_cluster_ttl;
}

static inline void
init_dfp_dns_cache_config(RpDfpDnsCacheCfg* self, dfp_dns_cache_cfg_t* cfg)
{
    NOISY_MSG_("(%p, %p)", self, cfg);
    g_strlcpy(self->name, cfg->name, sizeof(self->name));
    self->dns_lookup_family = cfg->dns_lookup_family;
    self->host_ttl = cfg->host_ttl;
    self->max_hosts = cfg->max_hosts;
}

static inline void
init_dfp_cluster_config(RpDfpClusterCfg* self, dfp_cluster_cfg_t* cfg)
{
    NOISY_MSG_("(%p, %p)", self, cfg);
    if (cfg->sub_clusters_cfg)
    {
        init_dfp_sub_clusters_config(&self->implementation_specifier.sub_clusters_config, cfg->sub_clusters_cfg);
        self->implementation_specifier_type = RpDfpImplementationSpecifierType_SUB_CLUSTER_TYPE;
    }
    else if (cfg->dns_cache_cfg)
    {
        init_dfp_dns_cache_config(&self->implementation_specifier.dns_cache_config, cfg->dns_cache_cfg);
        self->implementation_specifier_type = RpDfpImplementationSpecifierType_DNS_CACHE_CONFIG_TYPE;
    }
}

static inline void
init_cluster_type_cfg(RpCustomClusterTypeCfg* self, cluster_type_cfg_t* cfg)
{
    NOISY_MSG_("(%p, %p)", self, cfg);
    g_strlcpy(self->name, cfg->name, sizeof(self->name));
    if (cfg->dfp_cluster_config)
    {
        init_dfp_cluster_config(&self->typed_config.dfp_cluster_config, cfg->dfp_cluster_config);
        self->typed_config_type = RpTypedConfigType_DFP_CLUSTER_CONFIG;
    }
    //TODO...else if (...) {...}
}

static inline void
init_cluster_cfg(RpClusterCfg* self, rule_t* rule)
{
    NOISY_MSG_("(%p, %p)", self, rule);
    rule_cfg_t* rule_cfg = rule->config;
    init_preconnect_policy_cfg(&self->preconnect_policy);
    init_cluster_load_assignment_cfg(&self->load_assignment, rule);
    if (rule_cfg->cluster_type)
    {
        init_cluster_type_cfg(&self->cluster_discovery_type.cluster_type, rule_cfg->cluster_type);
        self->cluster_discovery_type_type = RpClusterDiscoveryTypeType_CLUSTER_TYPE;
    }
    else
    {
        self->cluster_discovery_type.type = translate_discovery_type(rule_cfg);
        self->cluster_discovery_type_type = RpClusterDiscoveryTypeType_TYPE;
    }
    self->connect_timeout_secs = 5;
    self->per_connection_buffer_limit_bytes = 1024*1024;
//    self->lb_policy = translate_lb_policy(rule_cfg);
rp_cluster_cfg_set_lb_policy(self, RpLbPolicy_CLUSTER_PROVIDED);
    self->dns_lookup_family = RpDnsLookupFamily_AUTO;
    self->connection_pool_per_downstream_connection = false;
    self->rule = rule;
}

static void
process_rule_cb(gpointer data, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", data, arg);
    RpProcessRuleCbCtx* ctx = arg;
    RpClusterManagerImpl* me = ctx->cluster_manager;
    GError** err = ctx->err;
    rule_t* rule = data;

    RpClusterCfgPtr cluster = rp_cluster_cfg_new();
    init_cluster_cfg(cluster, rule);

    RpClusterDataPtr new_cluster = load_cluster(me,
                                                cluster,
                                                config_hash(cluster),
                                                /*added_via_api=*/false,
                                                me->m_active_clusters,
                                                err);
    if (err && *err)
    {
        LOGE("error %d(%s)", (*err)->code, (*err)->message);
    }
    else if (new_cluster)
    {
        NOISY_MSG_("replaced cluster data %p", new_cluster);
        g_clear_object(&new_cluster);
    }

    rp_cluster_cfg_free(cluster);
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
    g_slist_foreach(bootstrap->rules, process_rule_cb, &ctx);

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

typedef struct _RpClusterInitializeCbCtx RpClusterInitializeCbCtx;
struct _RpClusterInitializeCbCtx {
    RpClusterManager* this_;
    char* cluster_name;
};
static inline RpClusterInitializeCbCtx
rp_cluster_initialize_cb_ctx_ctor(RpClusterManager* this_, char* cluster_name)
{
    RpClusterInitializeCbCtx self = {
        .this_ = this_,
        .cluster_name = cluster_name
    };
    return self;
}
static inline RpClusterInitializeCbCtx*
rp_cluster_initialize_cb_ctx_new(RpClusterManager* this_, const char* cluster_name)
{
    RpClusterInitializeCbCtx* self = g_new(RpClusterInitializeCbCtx, 1);
    *self = rp_cluster_initialize_cb_ctx_ctor(this_, g_strdup(cluster_name));
    return self;
}
static inline RpClusterInitializeCbCtx
rp_cluster_initialize_cb_captured(RpClusterInitializeCbCtx* self)
{
    RpClusterInitializeCbCtx captured = rp_cluster_initialize_cb_ctx_ctor(self->this_, self->cluster_name);
    g_free(self);
    return captured;
}
static RpStatusCode_e
rp_cluster_initialize_cb(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    RpClusterInitializeCbCtx* ctx = arg;
    RpClusterInitializeCbCtx captured = rp_cluster_initialize_cb_captured(ctx);
    RpClusterManager* self = captured.this_;
    RpClusterManagerImpl* me = RP_CLUSTER_MANAGER_IMPL(self);
    char* cluster_name = captured.cluster_name;
    RpClusterDataPtr state_changed_cluster_entry = g_hash_table_lookup(me->m_warming_clusters, cluster_name);
    NOISY_MSG_("warming state changed cluster %p(%s) complete", state_changed_cluster_entry, cluster_name);
    g_free(cluster_name);
    //TODO...
    return on_cluster_init(self, RP_CLUSTER_MANAGER_CLUSTER(state_changed_cluster_entry));
}

static bool
add_or_update_cluster_i(RpClusterManager* self, const RpClusterCfg* cluster, const char* version_info)
{
    NOISY_MSG_("(%p, %p, %p(%s))", self, cluster, version_info, version_info);

    const char* cluster_name = rp_cluster_cfg_name(cluster);
    NOISY_MSG_("cluster name \"%s\"", cluster_name);
    RpClusterManagerImpl* me = RP_CLUSTER_MANAGER_IMPL(self);
    RpClusterDataPtr existing_active_cluster = g_hash_table_lookup(me->m_active_clusters, cluster_name);
    RpClusterDataPtr existing_warming_cluster = g_hash_table_lookup(me->m_warming_clusters, cluster_name);
    guint64 new_hash = config_hash(cluster);
    if (existing_warming_cluster)
    {
        NOISY_MSG_("warming cluster %p(%s)", existing_warming_cluster, cluster_name);
        if (rp_cluster_data_block_update(existing_warming_cluster, new_hash))
        {
            NOISY_MSG_("return false");
            return false;
        }
    }
    else if (existing_active_cluster && rp_cluster_data_block_update(existing_active_cluster, new_hash))
    {
        NOISY_MSG_("active cluster %p(%s), returning false", existing_active_cluster, cluster_name);
        return false;
    }

    if (existing_active_cluster || existing_warming_cluster)
    {
        if (existing_active_cluster)
        {
            NOISY_MSG_("active cluster %p(%s)", existing_active_cluster, cluster_name);
            rp_cluster_manager_init_helper_remove_cluster(me->m_init_helper, RP_CLUSTER_MANAGER_CLUSTER(existing_active_cluster));
        }
        //TODO...cm_stats_.cluster_modified_.inc();
    }
    else
    {
        //TODO...cm_stats_.cluster_added_.inc();
    }

    bool all_clusters_initialized = rp_cluster_manager_init_helper_all_clusters_initialized(me->m_init_helper);
    RpClusterDataPtr previous_cluster = load_cluster(me, cluster, config_hash(cluster), /*added_via_api*/true, me->m_warming_clusters, NULL);
    RpClusterDataPtr cluster_entry = g_hash_table_lookup(me->m_warming_clusters, cluster_name);
    if (!all_clusters_initialized)
    {
        NOISY_MSG_("add/update cluster %p(%s) during init", cluster_entry, cluster_name);
        rp_cluster_manager_init_helper_add_cluster(me->m_init_helper, RP_CLUSTER_MANAGER_CLUSTER(cluster_entry));
    }
    else
    {
        NOISY_MSG_("add/update cluster %p(%s) starting warming", cluster_entry, cluster_name);
        RpClusterInitializeCbCtx* ctx = rp_cluster_initialize_cb_ctx_new(self, cluster_name);
        rp_cluster_initialize(
            rp_cluster_manager_cluster_cluster(RP_CLUSTER_MANAGER_CLUSTER(cluster_entry)), rp_cluster_initialize_cb, ctx);
    }
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

static bool
is_shutdown_i(RpClusterManager* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_MANAGER_IMPL(self)->m_shutdown;
}

static void
shutdown_i(RpClusterManager* self)
{
    NOISY_MSG_("(%p)", self);
    RpClusterManagerImpl* me = RP_CLUSTER_MANAGER_IMPL(self);
    me->m_shutdown = true;
    g_clear_pointer(&me->m_active_clusters, g_hash_table_unref);
    g_clear_pointer(&me->m_warming_clusters, g_hash_table_unref);
    g_clear_pointer(&me->m_all_clusters, g_hash_table_unref);
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
    iface->is_shutdown = is_shutdown_i;
    iface->shutdown = shutdown_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpClusterManagerImpl* self = RP_CLUSTER_MANAGER_IMPL(obj);
    g_clear_pointer(&self->m_active_clusters, g_hash_table_unref);
    g_clear_pointer(&self->m_warming_clusters, g_hash_table_unref);
    g_clear_pointer(&self->m_cluster_initialization_map, g_hash_table_unref);
    g_clear_pointer(&self->m_all_clusters, g_hash_table_unref);
    g_clear_object(&self->m_init_helper);
    self->m_tls = NULL;

    G_OBJECT_CLASS(rp_cluster_manager_impl_parent_class)->dispose(obj);
}

static void
rp_cluster_manager_impl_class_init(RpClusterManagerImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_cluster_manager_impl_init(RpClusterManagerImpl* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_active_clusters = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
    self->m_warming_clusters = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
    self->m_cluster_initialization_map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
    self->m_init_helper = rp_cluster_manager_init_helper_new(RP_CLUSTER_MANAGER(self),
                                                                on_cluster_init);
    self->m_all_clusters = g_hash_table_new_full(g_direct_hash, g_direct_equal, g_object_unref, NULL);
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
