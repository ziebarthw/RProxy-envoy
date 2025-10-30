/*
 * rp-dfp-cluster-impl.c
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_dfp_cluster_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_dfp_cluster_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <glib.h>

#include "event/rp-dispatcher-impl.h"
#include "rp-dfp-cluster-store.h"
#include "rp-header-utility.h"
#include "rp-http-utility.h"
#include "rp-state-filter.h"
#include "rp-dfp-cluster-impl.h"

#define PARENT_THREAD_LOCAL_CLUSTER_IFACE(s) \
    ((RpThreadLocalClusterInterface*)g_type_interface_peek_parent(RP_THREAD_LOCAL_CLUSTER_GET_IFACE(s)))

struct _RpDfpClusterImpl {
    RpClusterImplBase parent_instance;

    RpClusterCfg m_config;
    RpClusterManager* m_cm;

    bool m_disable_sub_cluster;
};

static void thread_local_cluster_iface_init(RpThreadLocalClusterInterface* iface);
static void dfp_cluster_iface_init(RpDfpClusterInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpDfpClusterImpl, rp_dfp_cluster_impl, RP_TYPE_STATIC_CLUSTER_IMPL,
    G_IMPLEMENT_INTERFACE(RP_TYPE_THREAD_LOCAL_CLUSTER, thread_local_cluster_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_DFP_CLUSTER, dfp_cluster_iface_init)
)

static inline RpHostSelectionResponse
null_host_selection_response(void)
{
    NOISY_MSG_("()");
    return rp_host_selection_response_ctor(NULL, NULL, NULL);
}

static server_cfg_t*
create_server(RpDfpCluster* self, const char* host_name, int port)
{
    static cfg_opt_t downstream_opts[] = {
        CFG_BOOL("enabled",           cfg_true,       CFGF_NONE),
        CFG_INT("port",               0,              CFGF_NODEFAULT),
        CFG_INT("connections",        1,              CFGF_NONE),
        CFG_INT("high-watermark",     0,              CFGF_NONE),
        CFG_INT_LIST("read-timeout",  "{ 0, 0 }",     CFGF_NONE),
        CFG_INT_LIST("write-timeout", "{ 0, 0 }",     CFGF_NONE),
        CFG_INT_LIST("retry",         "{ 0, 50000 }", CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t rule_opts[] = {
        CFG_STR("uri-match",            NULL,         CFGF_NODEFAULT),
        CFG_STR_LIST("downstreams",     NULL,         CFGF_NODEFAULT),
        CFG_STR("lb-method",            "rtt",        CFGF_NONE),
        CFG_STR("discovery-type",       "strict_dns", CFGF_NONE),
        CFG_BOOL("passthrough",         cfg_false,    CFGF_NONE),
        CFG_BOOL("allow-redirect",      cfg_false,    CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t server_opts[] = {
        CFG_SEC("downstream", downstream_opts, CFGF_MULTI|CFGF_TITLE|CFGF_NO_TITLE_DUPES),
        CFG_SEC("rule",       rule_opts,       CFGF_TITLE|CFGF_MULTI|CFGF_NO_TITLE_DUPES),
        CFG_END()
    };
    static cfg_opt_t rproxy_opts[] = {
        CFG_SEC("server", server_opts, CFGF_MULTI|CFGF_TITLE|CFGF_NO_TITLE_DUPES),
        CFG_END()
    };
    static char cfg_fmt[] = "server dynamic_server {\n"
                            "   downstream %s {\n"
                            "       port           = %d\n"
                            "       connections    = 1\n"
                            "       high-watermark = 0\n"
                            "       read-timeout   = { 0, 0 }\n"
                            "       write-timeout  = { 0, 0 }\n"
                            "       retry          = { 1, 50000 }\n"
                            "   }\n"
                            "   rule default {\n"
                            "       uri-gmatch = \"*\"\n"
                            "       passthrough = true\n"
                            "       discovery-type = strict_dns\n"
                            "       downstreams = { %s }\n"
                            "   }\n"
                            "}\n";
    NOISY_MSG_("(%p, %p(%s), %d)", self, host_name, host_name, port);

    RpDfpClusterImpl* me = RP_DFP_CLUSTER_IMPL(self);
    g_autofree gchar* buf = g_strdup_printf(cfg_fmt, host_name, port, host_name);
    NOISY_MSG_("buf\n\"%s\"", buf);

    cfg_t* cfg = cfg_init(rproxy_opts, CFGF_NOCASE|CFGF_IGNORE_UNKNOWN);
    cfg_parse_buf(cfg, buf);

    server_cfg_t* scfg = server_cfg_parse(cfg_getnsec(cfg, "server", 0));
    downstream_cfg_t* ds_cfg = lztq_elem_data(lztq_first(scfg->downstreams));
    rule_cfg_t* rule_cfg = scfg->default_rule_cfg;

    cfg_free(cfg);

    RpDispatcher* dispatcher = me->m_config.dispatcher;
    evthr_t* thr = rp_dispatcher_thr(RP_DISPATCHER_IMPL(dispatcher));
    rproxy_t* rproxy = evthr_get_aux(thr);
    downstream_t* downstream = downstream_new(rproxy, ds_cfg);

    lztq_append(rproxy->downstreams, downstream, sizeof(downstream), downstream_free);

    rule_t* rule = g_new0(rule_t, 1);
    rule->rproxy = rproxy;
    rule->config = rule_cfg;
    rule->downstreams = lztq_new();
    g_assert(rule->downstreams != NULL);

    lztq_append(rule->downstreams, downstream, sizeof(downstream), NULL);
    lztq_append(rproxy->rules, rule, sizeof(rule), NULL);

    scfg->default_rule = rule;

    return scfg;
}

char*
normalize_host_for_dfp(const char* host, guint16 default_port)
{
    NOISY_MSG_("(%p(%s), %u)", host, host, default_port);
    if (rp_header_utility_host_has_port(host))
    {
        NOISY_MSG_("has port");
        return g_strdup(host);
    }
    return g_strdup_printf("%s:%u", host, default_port);
}

static RpHostSelectionResponse
choose_host_i(RpThreadLocalCluster* self, RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %p)", self, context);

    if (!context)
    {
        NOISY_MSG_("context is null");
        return null_host_selection_response();
    }

    //TODO...const bool is_secure = ...
    guint32 default_port = /*TODO...is_secure ? ...*/80;

    RpStreamInfo* stream_info = rp_load_balancer_context_request_stream_info(context);
    RpFilterState* filter_state = stream_info ? rp_stream_info_filter_state(stream_info) : NULL;

    const char* raw_host = NULL;
    guint32 port = default_port;

    if (filter_state)
    {
        NOISY_MSG_("getting host from filter state");
        raw_host = rp_filter_state_get_data(filter_state, dynamic_host_key);

        gpointer dynamic_port_filter_state = rp_filter_state_get_data(filter_state, dynamic_port_key);
        if (dynamic_port_filter_state)
        {
            port = *((guint32*)dynamic_port_filter_state);
        }
    }
    else if (rp_load_balancer_context_downstream_headers(context))
    {
        NOISY_MSG_("getting host from downstream headers");
        raw_host = evhtp_header_find(rp_load_balancer_context_downstream_headers(context), RpHeaderValues.HostLegacy);
    }
    else if (rp_load_balancer_context_downstream_connection(context))
    {
        NOISY_MSG_("getting host from downstream connection");
        raw_host = rp_connection_socket_requested_server_name(RP_CONNECTION_SOCKET(rp_load_balancer_context_downstream_connection(context)));
    }

    //TODO...if (stream_info && !dynamic_host_filter_state)

    if (!raw_host || !raw_host[0])
    {
        LOGD("host empty");
        return null_host_selection_response();
    }

    g_autofree char* hostname = normalize_host_for_dfp(raw_host, port);

    if (rp_dfp_cluster_enable_sub_cluster(RP_DFP_CLUSTER(self)))
    {
        NOISY_MSG_("calling rp_dfp_cluster_impl_choose_host(%p, %p(%s), %p)", self, hostname, hostname, context);
        return rp_dfp_cluster_impl_choose_host(RP_DFP_CLUSTER_IMPL(self), hostname, context);
    }

//TODO...

//    return null_host_selection_response();
return PARENT_THREAD_LOCAL_CLUSTER_IFACE(self)->choose_host(self, context);
}

static RpHttpPoolData*
http_conn_pool_i(RpThreadLocalCluster* self, RpHost* host, RpResourcePriority_e priority, evhtp_proto protocol, RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %p, %d, %d, %p)", self, host, priority, protocol, context);
    return PARENT_THREAD_LOCAL_CLUSTER_IFACE(self)->http_conn_pool(self, host, priority, protocol, context);
}

static RpClusterInfo*
info_i(RpThreadLocalCluster* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_THREAD_LOCAL_CLUSTER_IFACE(self)->info(self);
}

static RpTcpPoolData*
tcp_conn_pool_i(RpThreadLocalCluster* self, RpHost* host, RpResourcePriority_e priority, RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %p, %d, %p)", self, host, priority, context);
    return PARENT_THREAD_LOCAL_CLUSTER_IFACE(self)->tcp_conn_pool(self, host, priority, context);
}

static void
thread_local_cluster_iface_init(RpThreadLocalClusterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->choose_host = choose_host_i;
    iface->http_conn_pool = http_conn_pool_i;
    iface->info = info_i;
    iface->tcp_conn_pool = tcp_conn_pool_i;
}

static RpClusterCfg
create_sub_cluster_config_i(RpDfpCluster* self, const char* cluster_name, const char* host_name, int port)
{
    LOGD("(%p, %p(%s), %p(%s), %d)",
        self, cluster_name, cluster_name, host_name, host_name, port);

    RpDfpClusterImpl* me = RP_DFP_CLUSTER_IMPL(self);
    RpClusterCfg config = me->m_config;

    rp_cluster_cfg_set_name(&config, cluster_name);
    rp_cluster_cfg_set_type(&config, RpDiscoveryType_STRICT_DNS);

    config.lb_endpoints = lztq_new();
    server_cfg_t* scfg = create_server(self, host_name, port);
    downstream_t* downstream = lztq_elem_data(lztq_first(scfg->default_rule->downstreams));
    lztq_append(config.lb_endpoints, downstream, sizeof(downstream), NULL);
    config.rule = scfg->default_rule;

    return config;
}

static bool
enable_sub_cluster_i(RpDfpCluster* self)
{
    NOISY_MSG_("(%p)", self);
    return !RP_DFP_CLUSTER_IMPL(self)->m_disable_sub_cluster;
}

static bool
touch_i(RpDfpCluster* self G_GNUC_UNUSED, const char* cluster_name G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p(%s))", self, cluster_name, cluster_name);
    return true;
}

static void
dfp_cluster_iface_init(RpDfpClusterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_sub_cluster_config = create_sub_cluster_config_i;
    iface->enable_sub_cluster = enable_sub_cluster_i;
    iface->touch = touch_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_dfp_cluster_impl_parent_class)->dispose(obj);
}

static void
rp_dfp_cluster_impl_class_init(RpDfpClusterImplClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_dfp_cluster_impl_init(RpDfpClusterImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpDfpClusterImpl*
rp_dfp_cluster_impl_new(RpClusterCfg* config, RpClusterFactoryContext* context, RpStatusCode_e* creation_status)
{
    LOGD("(%p, %p, %p)", config, context, creation_status);
    g_return_val_if_fail(config != NULL, NULL);
    g_return_val_if_fail(RP_IS_CLUSTER_FACTORY_CONTEXT(context), NULL);
    g_return_val_if_fail(creation_status != NULL, NULL);
    RpDfpClusterImpl* self = g_object_new(RP_TYPE_DFP_CLUSTER_IMPL,
                                            "config", config,
                                            "context", context,
                                            "cluster-context", context,
                                            NULL);
    self->m_config = *config;
    self->m_cm = rp_cluster_factory_context_cluster_manager(context);
    *creation_status = RpStatusCode_Ok;
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
//return PARENT_THREAD_LOCAL_CLUSTER_IFACE(self)->choose_host(cluster, context);
}

RpClusterImplBase*
rp_dfp_cluster_impl_create_cluster_with_config(RpClusterCfg* cluster, RpClusterFactoryContext* context)
{
    LOGD("(%p, %p)", cluster, context);
    //TODO...

    //TODO...

    //TODO...

    RpStatusCode_e creation_status = RpStatusCode_Ok;
    RpDfpClusterImpl* new_cluster = rp_dfp_cluster_impl_new(cluster, context, &creation_status);

    RpClusterManager* cluster_manager = rp_cluster_factory_context_cluster_manager(context);
    RpDfpClusterStore* cluster_store = rp_dfp_cluster_store_get_instance(cluster_manager);
    const char* cluster_name = rp_cluster_info_name(rp_cluster_info(RP_CLUSTER(new_cluster)));

    rp_dfp_cluster_store_save(cluster_store, cluster_name, RP_DFP_CLUSTER(new_cluster));

    //TODO...

    return RP_CLUSTER_IMPL_BASE(new_cluster);
}

void
rp_dfp_cluster_impl_set_disable_sub_cluster(RpDfpClusterImpl* self, bool disable)
{
    LOGD("(%p, %u)", self, disable);
    g_return_if_fail(RP_IS_DFP_CLUSTER_IMPL(self));
    self->m_disable_sub_cluster = disable;
}
