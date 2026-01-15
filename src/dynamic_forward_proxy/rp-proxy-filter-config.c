/*
 * rp-proxy-filter-config.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_proxy_filter_config_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_proxy_filter_config_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-dynamic-forward-proxy.h"

struct _RpProxyFilterConfig {
    GObject parent_instance;

    RpDfpClusterStoreSharedPtr m_cluster_store;
    RpClusterManager* m_cluster_manager;
    RpDispatcher* m_main_thread_dispatcher;
    WEAK_PTR(RpSlot) m_tls_slot;
    //TODO...const std::chrono::milliseconds cluster_init_timeout_;
    bool m_save_upstream_address;
};

G_DEFINE_TYPE(RpProxyFilterConfig, rp_proxy_filter_config, G_TYPE_OBJECT)

static inline gchar*
create_cluster_name(const char* host, int port)
{
    NOISY_MSG_("(%p(%s), %d)", host, host, port);
    return g_strdup_printf("DFPCluster:%s:%u", host, port);
}

static inline RpClusterManager*
cluster_manager_from_factory_context(RpFactoryContext* context)
{
    return rp_common_factory_context_cluster_manager(RP_COMMON_FACTORY_CONTEXT(
        rp_generic_factory_context_server_factory_context(RP_GENERIC_FACTORY_CONTEXT(context))));
}

static inline RpDispatcher*
main_thread_dispatcher_from_factory_context(RpFactoryContext* context)
{
    return rp_common_factory_context_main_thread_dispatcher(RP_COMMON_FACTORY_CONTEXT(
        rp_generic_factory_context_server_factory_context(RP_GENERIC_FACTORY_CONTEXT(context))));
}

static inline RpSlotAllocator*
slot_allocator_from_factory_context(RpFactoryContext* context)
{
    return rp_common_factory_context_thread_local(RP_COMMON_FACTORY_CONTEXT(
        rp_generic_factory_context_server_factory_context(RP_GENERIC_FACTORY_CONTEXT(context))));
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpProxyFilterConfig* self = RP_PROXY_FILTER_CONFIG(obj);
    g_clear_object(&self->m_cluster_store);
    g_clear_object(&self->m_cluster_manager);
    g_clear_object(&self->m_main_thread_dispatcher);

    G_OBJECT_CLASS(rp_proxy_filter_config_parent_class)->dispose(obj);
}

static void
rp_proxy_filter_config_class_init(RpProxyFilterConfigClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static RpThreadLocalObjectSharedPtr
slot_initialize_cb(RpDispatcher* dispatcher G_GNUC_UNUSED, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", dispatcher, arg);
    return RP_THREAD_LOCAL_OBJECT(
            rp_thread_local_cluster_info_new(RP_PROXY_FILTER_CONFIG(arg)));
}

static void
rp_proxy_filter_config_init(RpProxyFilterConfig* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpProxyFilterConfigSharedPtr
rp_proxy_filter_config_new(RpDfpClusterStoreFactory* cluster_store_factory, RpFactoryContext* context)
{
    LOGD("(%p, %p)", cluster_store_factory, context);

    g_return_val_if_fail(RP_IS_DFP_CLUSTER_STORE_FACTORY(cluster_store_factory), NULL);
    g_return_val_if_fail(RP_IS_FACTORY_CONTEXT(context), NULL);

    RpProxyFilterConfig* self = g_object_new(RP_TYPE_PROXY_FILTER_CONFIG, NULL);
    self->m_cluster_store = g_object_ref(rp_dfp_cluster_store_factory_get(cluster_store_factory));
    self->m_cluster_manager = g_object_ref(cluster_manager_from_factory_context(context));
    self->m_main_thread_dispatcher = g_object_ref(main_thread_dispatcher_from_factory_context(context));
    self->m_tls_slot = rp_slot_allocator_allocate_slot(slot_allocator_from_factory_context(context));
    rp_slot_set(self->m_tls_slot, slot_initialize_cb, self);
    //TODO...save_upstream_address_(proto_config.save_upstream_address())
    return self;
}

RpDfpClusterStoreSharedPtr
rp_proxy_filter_config_cluster_store(RpProxyFilterConfig* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_PROXY_FILTER_CONFIG(self), NULL);
    return self->m_cluster_store;
}

RpClusterManager*
rp_proxy_filter_config_cluster_manager(RpProxyFilterConfig* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_PROXY_FILTER_CONFIG(self), NULL);
    return self->m_cluster_manager;
}

bool
rp_proxy_filter_config_save_upstream_address(RpProxyFilterConfig* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_PROXY_FILTER_CONFIG(self), false);
    return self->m_save_upstream_address;
}

typedef struct _RpAddOrUpdateClusterCbCtx RpAddOrUpdateClusterCbCtx;
struct _RpAddOrUpdateClusterCbCtx {
    RpProxyFilterConfig* filter_config;
    RpClusterCfg config;
    const char* version_info;
};
static inline RpAddOrUpdateClusterCbCtx
rp_add_or_update_cluster_cb_ctx_ctor(RpProxyFilterConfig* filter_config, const RpClusterCfg* config, const char* version_info)
{
    RpAddOrUpdateClusterCbCtx self = {
        .filter_config = g_object_ref(filter_config),
        .config = *config,
        .version_info = version_info
    };
    return self;
}
static inline void
rp_add_or_update_cluster_cb_ctx_dtor(RpAddOrUpdateClusterCbCtx* self)
{
    NOISY_MSG_("(%p)", self);
    g_clear_object(&self->filter_config);
    rp_cluster_cfg_dtor(&self->config);
}
static inline RpAddOrUpdateClusterCbCtx*
rp_add_or_update_cluster_cb_ctx_new(RpProxyFilterConfig* filter_config, const RpClusterCfg* config, const char* version_info)
{
    NOISY_MSG_("(%p, %p, %p(%s))", filter_config, config, version_info, version_info);
    RpAddOrUpdateClusterCbCtx* self = g_new(RpAddOrUpdateClusterCbCtx, 1);
    *self = rp_add_or_update_cluster_cb_ctx_ctor(filter_config, config, version_info);
    return self;
}
static inline void
rp_add_or_update_cluster_cb_ctx_free(RpAddOrUpdateClusterCbCtx* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_if_fail(self != NULL);
    rp_add_or_update_cluster_cb_ctx_dtor(self);
    g_free(self);
}

static void
add_or_update_cluster_cb(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    RpAddOrUpdateClusterCbCtx* ctx = arg;
    RpProxyFilterConfig* self = ctx->filter_config;
    RpClusterCfg config = ctx->config;
    const char* version_info = ctx->version_info;
    if (!rp_cluster_manager_add_or_update_cluster(self->m_cluster_manager, &config, version_info))
    {
        LOGE("failed");
    }
#if 0
    {
        upstream_t* upstream = lztq_elem_data(lztq_first(config->lb_endpoints));
        upstream_cfg_t* dscfg = upstream->config;
        g_autofree gchar* cluster_name = create_cluster_name(dscfg->name, dscfg->port);
        RpDfpClusterSharedPtr new_cluster = rp_dfp_cluster_store_load(self->m_cluster_store, cluster_name);
        return true;
    }
#endif//0
    rp_add_or_update_cluster_cb_ctx_free(ctx);
}

static inline bool
valid_add_dynamic_cluster_params(RpProxyFilterConfig* self, RpDfpCluster* cluster,
                                            const char* cluster_name, const char* host, int port,
                                            RpLoadClusterEntryCallbacks* callbacks)
{
    g_return_val_if_fail(RP_IS_PROXY_FILTER_CONFIG(self), false);
    g_return_val_if_fail(RP_IS_DFP_CLUSTER(cluster), false);
    g_return_val_if_fail(cluster_name != NULL, false);
    g_return_val_if_fail(cluster_name[0], false);
    g_return_val_if_fail(host != NULL, false);
    g_return_val_if_fail(host[0], false);
    g_return_val_if_fail(port >= 0, false);
    g_return_val_if_fail(RP_IS_LOAD_CLUSTER_ENTRY_CALLBACKS(callbacks), false);
    return true;
}

RpLoadClusterEntryHandlePtr
rp_proxy_filter_config_add_dynamic_cluster(RpProxyFilterConfig* self, RpDfpCluster* cluster,
                                            const char* cluster_name, const char* host, int port,
                                            RpLoadClusterEntryCallbacks* callbacks)
{
    LOGD("(%p, %p, %p(%s), %p(%s), %d, %p)",
        self, cluster, cluster_name, cluster_name, host, host, port, callbacks);

    g_return_val_if_fail(valid_add_dynamic_cluster_params(self, cluster, cluster_name, host, port, callbacks), NULL);

    RpDfpCreateSubClusterConfigRval sub_cluster_pair = rp_dfp_cluster_create_sub_cluster_config(cluster, cluster_name, host, port);

    if (!sub_cluster_pair.success)
    {
        LOGD("cluster \"%s\" create failed due to max sub cluster limitation", cluster_name);
        return false;
    }

//TODO...this is more complicated than necessary

    if (sub_cluster_pair.config)
    {
        RpClusterCfg* cluster_ = &sub_cluster_pair.config_;
        const char* version_info = "";
        LOGD("deliver dynamic cluster \"%s\" creation to main thread", cluster_name);
        rp_dispatcher_base_post(RP_DISPATCHER_BASE(self->m_main_thread_dispatcher),
                                add_or_update_cluster_cb,
                                rp_add_or_update_cluster_cb_ctx_new(self, cluster_, version_info));
    }

    LOGD("adding pending cluster for \"%s\"", cluster_name);
    RpThreadLocalClusterInfo* tls_cluster_info = RP_THREAD_LOCAL_CLUSTER_INFO(rp_slot_get(self->m_tls_slot));
    GHashTable* parent = rp_thread_local_cluster_info_pending_clusters_(tls_cluster_info);
    return RP_LOAD_CLUSTER_ENTRY_HANDLE(
            rp_load_cluster_entry_handle_impl_new(parent, cluster_name, callbacks));
}
