/*
 * rp-dfp-cluster-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_dfp_cluster_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_dfp_cluster_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-headers.h"
#include "rp-header-utility.h"
#include "rp-http-utility.h"
#include "rp-state-filter.h"
#include "dynamic_forward_proxy/rp-cluster.h"

#define PARENT_THREAD_LOCAL_CLUSTER_IFACE(s) \
    ((RpThreadLocalClusterInterface*)g_type_interface_peek_parent(RP_THREAD_LOCAL_CLUSTER_GET_IFACE(s)))

typedef GHashTable* HostInfoMap;
typedef GHashTable* ClusterInfoMap;
typedef struct _ClusterInfo ClusterInfo;
struct _ClusterInfo {
    const char* m_cluster_name;
    RpDfpCluster* m_parent;
    //TODO...std::atomic<std::chrono::steady_clock::duration> last_used_time_;
};

static inline ClusterInfo
cluster_info_ctor(const char* cluster_name, SHARED_PTR(RpDfpCluster) parent)
{
    ClusterInfo self = {
        .m_cluster_name = cluster_name,
        .m_parent = g_object_ref(parent)
    };
    return self;
}

static inline ClusterInfo*
cluster_info_new(const char* cluster_name, SHARED_PTR(RpDfpCluster) parent)
{
    NOISY_MSG_("(%p(%s), %p)", cluster_name, cluster_name, parent);
    ClusterInfo* self = g_new(ClusterInfo, 1);
    *self = cluster_info_ctor(cluster_name, parent);
    return self;
}

static inline void
cluster_info_free(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    ClusterInfo* self = arg;
    g_clear_object(&self->m_parent);
    g_free(self);
}

static inline void
cluster_info_touch(ClusterInfo* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    //TODO...
}

struct _RpDfpClusterImpl {
    RpClusterImplBase parent_instance;

    RpClusterManager* m_cm;

    GRWLock m_host_map_lock;
    HostInfoMap m_host_map;

    GRWLock m_cluster_map_lock;
    ClusterInfoMap m_cluster_map;

    RpClusterCfg m_orig_cluster_config;

    bool m_enable_sub_cluster;
};

static void cluster_iface_init(RpClusterInterface* iface);
static void dfp_cluster_iface_init(RpDfpClusterInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpDfpClusterImpl, rp_dfp_cluster_impl, RP_TYPE_CLUSTER_IMPL_BASE,
    G_IMPLEMENT_INTERFACE(RP_TYPE_DFP_CLUSTER, dfp_cluster_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_CLUSTER, cluster_iface_init)
)

static inline RpHostSelectionResponse
null_host_selection_response(void)
{
    NOISY_MSG_("()");
    return rp_host_selection_response_ctor(NULL, NULL, NULL);
}

static bool
touch_i(RpDfpCluster* self, const char* cluster_name)
{
    NOISY_MSG_("(%p, %p(%s))", self, cluster_name, cluster_name);
    RpDfpClusterImpl* me = RP_DFP_CLUSTER_IMPL(self);
    G_RW_LOCK_READER_AUTO_LOCK(&me->m_cluster_map_lock, locker);
    SHARED_PTR(ClusterInfo) cluster_info = g_hash_table_lookup(me->m_cluster_map, cluster_name);
    if (cluster_info)
    {
        cluster_info_touch(cluster_info);
        return true;
    }
    LOGD("cluster \"%s\" has been removed while touching", cluster_name);
    return false;
}

static RpDfpCreateSubClusterConfigRval
create_sub_cluster_config_i(RpDfpCluster* self, const char* cluster_name, const char* host, int port)
{
    NOISY_MSG_("(%p, %p(%s), %p(%s), %d)",
        self, cluster_name, cluster_name, host, host, port);
    RpDfpClusterImpl* me = RP_DFP_CLUSTER_IMPL(self);
    {
        G_RW_LOCK_WRITER_AUTO_LOCK(&me->m_cluster_map_lock, locker);
        ClusterInfo* cluster_info = g_hash_table_lookup(me->m_cluster_map, cluster_name);
        if (cluster_info)
        {
            cluster_info_touch(cluster_info);
            return rp_dfp_create_sub_cluster_config_rval_ctor(NULL, true);
        }
        //TODO...
        g_hash_table_insert(me->m_cluster_map, g_strdup(cluster_name), cluster_info_new(cluster_name, self));
    }

    RpClusterCfg config = me->m_orig_cluster_config;
    rp_cluster_cfg_set_name(&config, cluster_name);
    rp_cluster_cfg_set_type(&config, RpDiscoveryType_STRICT_DNS);

    //TODO...Set endpoint.

    return rp_dfp_create_sub_cluster_config_rval_ctor(&config, true);
}

static bool
enable_sub_cluster_i(RpDfpCluster* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_DFP_CLUSTER_IMPL(self)->m_enable_sub_cluster;
}

static void
dfp_cluster_iface_init(RpDfpClusterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->touch = touch_i;
    iface->create_sub_cluster_config = create_sub_cluster_config_i;
    iface->enable_sub_cluster = enable_sub_cluster_i;
}

static RpInitializePhase_e
initialize_phase_i(RpCluster* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return RpInitializePhase_Primary;
}

#define PARENT_CLUSTER_IFACE(s) \
    ((RpClusterInterface*)g_type_interface_peek_parent(RP_CLUSTER_GET_IFACE(s)))

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

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpDfpClusterImpl* self = RP_DFP_CLUSTER_IMPL(obj);
    if (rp_cluster_manager_is_shutdown(self->m_cm))
    {
        NOISY_MSG_("cluster manager is shut down");
        g_clear_pointer(&self->m_cluster_map, g_hash_table_destroy);
        return;
    }

    G_RW_LOCK_WRITER_AUTO_LOCK(&self->m_cluster_map_lock, locker);
    GHashTableIter itr;
    gpointer key;
    gpointer value;
    g_hash_table_iter_init(&itr, self->m_cluster_map);
    while (g_hash_table_iter_next(&itr, &key, &value))
    {
        NOISY_MSG_("removing %p", value);
        g_hash_table_iter_remove(&itr);
        rp_cluster_manager_remove_cluster(self->m_cm, (char*)key);
    }

    G_OBJECT_CLASS(rp_dfp_cluster_impl_parent_class)->dispose(obj);
}

OVERRIDE void
start_pre_init(RpClusterImplBase* self)
{
    NOISY_MSG_("(%p)", self);
    //TODO...
    rp_cluster_impl_base_on_pre_init_complete(self);
}

static void
cluster_impl_base_class_init(RpClusterImplBaseClass* klass)
{
    LOGD("(%p)", klass);
    klass->start_pre_init = start_pre_init;
}

static void
rp_dfp_cluster_impl_class_init(RpDfpClusterImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    cluster_impl_base_class_init(RP_CLUSTER_IMPL_BASE_CLASS(klass));
}

static void
rp_dfp_cluster_impl_init(RpDfpClusterImpl* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_enable_sub_cluster = true; //TODO...needs to come from proto config.
    self->m_cluster_map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, cluster_info_free);
    self->m_host_map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    g_rw_lock_init(&self->m_cluster_map_lock);
    g_rw_lock_init(&self->m_host_map_lock);
}

RpDfpClusterImpl*
rp_dfp_cluster_impl_new(const RpClusterCfg* config, RpClusterFactoryContext* context, RpStatusCode_e* creation_status)
{
    LOGD("(%p, %p, %p)", config, context, creation_status);
    g_return_val_if_fail(config != NULL, NULL);
    g_return_val_if_fail(RP_IS_CLUSTER_FACTORY_CONTEXT(context), NULL);
    RpDfpClusterImpl* self = g_object_new(RP_TYPE_DFP_CLUSTER_IMPL,
                                        "config", config,
                                        "cluster-context", context,
                                        "creation-status", creation_status,
                                        NULL);
    self->m_orig_cluster_config = *config;
    self->m_cm = rp_cluster_factory_context_cluster_manager(context);
    return self;
}

RpHostSelectionResponse
rp_dfp_cluster_impl_choose_host(RpDfpClusterImpl* self, const char* host, RpLoadBalancerContext* context)
{
    LOGD("(%p, %p(%s), %p)", self, host, host, context);

    g_return_val_if_fail(RP_IS_DFP_CLUSTER_IMPL(self), null_host_selection_response());
    g_return_val_if_fail(host != NULL, null_host_selection_response());
    g_return_val_if_fail(host[0], null_host_selection_response());
    g_return_val_if_fail(RP_IS_LOAD_BALANCER_CONTEXT(context), null_host_selection_response());

    guint16 default_port = 80;
    //TODO...if (info_->transportSocketMatcher()....)

    RpAuthorityAttributes host_attributes = http_utility_parse_authority(host);
    string_view dynamic_host = host_attributes.m_host;
    guint16 port = host_attributes.m_port ? host_attributes.m_port : default_port;

    g_autoptr(GString) cluster_name = g_string_new(NULL);
    g_string_printf(cluster_name, "DFPCluster:%.*s:%d", (int)dynamic_host.m_length, dynamic_host.m_data, port);

    RpThreadLocalCluster* cluster = rp_cluster_manager_get_thread_local_cluster(self->m_cm, cluster_name->str);
    if (!cluster)
    {
        LOGD("get thread local failed, too short ttl?");
        return null_host_selection_response();
    }

    return rp_thread_local_cluster_choose_host(cluster, context);
}

RpHostConstSharedPtr
rp_dfp_cluster_impl_find_host_by_name(RpDfpClusterImpl* self, const char* host)
{
    LOGD("(%p, %p(%s))", self, host, host);

    g_return_val_if_fail(RP_IS_DFP_CLUSTER_IMPL(self), NULL);
    g_return_val_if_fail(host != NULL, NULL);
    g_return_val_if_fail(host[0], NULL);
    {
        G_RW_LOCK_READER_AUTO_LOCK(&self->m_host_map_lock, locker);
        return g_hash_table_lookup(self->m_host_map, host);
    }
}
