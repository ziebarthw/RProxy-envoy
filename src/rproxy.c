/* Copyright [2012] [Mandiant, inc]
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#ifdef USE_MALLOPT
#include <malloc.h>
#endif

#ifndef NO_RLIMIT
#include <sys/resource.h>
#endif

#include <glib.h>

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#include "rproxy.h"

#include "event/rp-dispatcher-impl.h"
#include "event/rp-real-time-system.h"
#include "network/rp-accepted-socket-impl.h"
#include "network/rp-default-client-conn-factory.h"
#include "network/rp-transport-socket-factory-impl.h"
#include "router/rp-router-filter.h"
#include "server/rp-instance-impl.h"
#include "server/rp-factory-context-impl.h"
#include "stream_info/rp-stream-info-impl.h"
#include "upstream/rp-cluster-factory-impl.h"
#include "upstream/rp-cluster-manager-impl.h"
#include "upstream/rp-host-impl.h"
#include "rp-active-tcp-conn.h"
#include "rp-cluster-configuration.h"
#include "rp-codec-client-prod.h"
#include "rp-dfp-cluster-factory.h"
#include "rp-headers.h"
#include "rp-http-conn-manager-config.h"
#include "rp-http-conn-manager-impl.h"
#include "rp-net-server-conn-impl.h"
#include "rp-per-host-upstream.h"
#include "rp-static-cluster-factory.h"

#if (defined(rproxy_NOISY) || defined(ALL_NOISY)) && !defined(NO_rproxy_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

static RpInstanceImpl* server = NULL;
static RpFactoryContextImpl* factory_context = NULL; // It *appears* this is the listener_manger (or listener?) in envoy.???
static RpTransportSocketFactoryImpl* transport_socket_factory = NULL;

RpPerHostGenericConnPoolFactory* default_conn_pool_factory = NULL;
RpDefaultClientConnectionFactory* default_client_connection_factory = NULL;

/**
 * @brief allocates a new upstream_t, and appends it to the
 *        rproxy->upstreams list. This is callback for the
 *        lztq_for_each function from rproxy_thread_init().
 *
 * @param elem
 * @param arg
 *
 * @return
 */
static int
add_upstream(lztq_elem* elem, void* arg)
{
    LOGD("(%p, %p)", elem, arg);

    rproxy_t* rproxy = arg;
    g_assert(rproxy != NULL);

    upstream_cfg_t* ds_cfg = lztq_elem_data(elem);
    g_assert(ds_cfg != NULL);

    upstream_t* upstream = upstream_new(rproxy, ds_cfg);
    g_assert(upstream != NULL);

    lztq_elem* nelem = lztq_append(rproxy->upstreams, upstream,
                                    sizeof(upstream), upstream_free);
    g_assert(nelem != NULL);

    return 0;
}

static int
map_rule_to_upstreams(rule_t* rule, rule_cfg_t* rule_cfg, rproxy_t* rproxy)
{
    LOGD("(%p, %p, %p)", rule, rule_cfg, rproxy);

    LOGD("rule_cfg %p(%s)", rule_cfg, rule_cfg->name);

    rule->rproxy       = rproxy;
    rule->config       = rule_cfg;

    rule->upstreams = lztq_new();
    g_assert(rule->upstreams != NULL);

    /* for each string in the rule_cfg's upstreams section, find the matching
     * upstream_t and append it.
     */
    for (lztq_elem* elem = lztq_first(rule_cfg->upstreams); elem; elem = lztq_next(elem))
    {
        const char* ds_name = lztq_elem_data(elem);
        g_assert(ds_name != NULL);

        upstream_t* ds;
        if (!(ds = upstream_find_by_name(rproxy->upstreams, ds_name)))
        {
            /* could not find a upstream_t which has this name! */
            g_error("Could not find upstream named '%s!", ds_name);
            exit(EXIT_FAILURE);
        }

        lztq_elem* nelem = lztq_append(rule->upstreams, ds, sizeof(ds), NULL);
        g_assert(nelem != NULL);
    }

    /* upstream_request_start_hook is passed only the rule_cfg as an argument.
     * This function will call file_rule_from_cfg to get the actual rule from
     * the global rproxy->rules list. Since we compare pointers, it is safe to
     * keep this as one single list.
     */
    lztq_append(rproxy->rules, rule, sizeof(rule), NULL);

    return 0;
} /* map_rule_to_upstreams */

/**
 * @brief match up names in the list of upstream_cfg_t's in rule_cfg->upstreams
 *        to the upstream_t's in the rproxy->upstreams list. If found,
 *        create a rule_t and appends it to the rproxy->rules list.
 *
 * @param elem a lztq elem with the type of vhost_cfg_t *
 * @param arg  the vhost_t *
 *
 * @return
 */
static int
map_vhost_rules_to_upstreams(lztq_elem* elem, void* arg)
{
    LOGD("(%p, %p)", elem, arg);

    vhost_t* vhost = arg;
    g_assert(vhost != NULL);

    LOGD("vhost %p(%s)", vhost, vhost->config->server_name);

    rproxy_t* rproxy = vhost->rproxy;
    g_assert(rproxy != NULL);

    rule_cfg_t* rule_cfg = lztq_elem_data(elem);
    g_assert(rule_cfg != NULL);

    LOGD("rule_cfg %p(%s)", rule_cfg, rule_cfg->name);

    rule_t* rule = g_new0(rule_t, 1);
    g_assert(rule != NULL);

    map_rule_to_upstreams(rule, rule_cfg, rproxy);
    rule->parent_vhost = vhost;

    /*
     * if a rule specific logging is found then all is good to go. otherwise
     * if a vhost specific logging is found then set it to rule. otherwise
     * if a server specific logging is found, set both vhost and rule to this.
     */
// REVISIT: not sure how this would EVER be set in its current from.(?)
#ifdef WITH_LOGGER
    if (rule_cfg->req_log)
    {
        LOGD("rule_cfg->req_log");
        rule->req_log = logger_init(rule_cfg->req_log, 0);
    }
    else if (vhost->req_log)
    {
        LOGD("vhost->req_log");
        rule->req_log = vhost->req_log;
    }
    else
    {
        LOGD("rproxy->req_log");
        rule->req_log  = rproxy->req_log;
        vhost->req_log = rproxy->req_log;
    }

    /*
     * the same logic applies as above for error logging.
     */
    if (rule_cfg->err_log)
    {
        LOGD("rule_cfg->err_log");
        rule->err_log = logger_init(rule_cfg->err_log, 0);
    }
    else if (vhost->err_log)
    {
        LOGD("vhost->err_log");
        rule->err_log = vhost->err_log;
    }
    else
    {
        LOGD("rproxy->err_log");
        rule->err_log  = rproxy->err_log;
        vhost->err_log = rproxy->err_log;
    }
#endif//WITH_LOGGER

    return 0;
} /* map_vhost_rules_to_upstreams */

static int
map_default_rule_to_upstreams(rule_t* rule, rule_cfg_t* rule_cfg, rproxy_t* rproxy)
{
    LOGD("(%p, %p, %p)", rule, rule_cfg, rproxy);

    LOGD("rule_cfg %p(%s)", rule_cfg, rule_cfg->name);

    map_rule_to_upstreams(rule, rule_cfg, rproxy);

    /*
     * if a rule specific logging is found then all is good to go. otherwise
     * if a vhost specific logging is found then set it to rule. otherwise
     * if a server specific logging is found, set both vhost and rule to this.
     */
// REVISIT: not sure how this would EVER be set in its current from.(?)
#ifdef WITH_LOGGER
    if (rule_cfg->req_log)
    {
        LOGD("rule_cfg->req_log");
        rule->req_log = logger_init(rule_cfg->req_log, 0);
    }
    else
    {
        LOGD("rproxy->req_log");
        rule->req_log  = rproxy->req_log;
    }

    /*
     * the same logic applies as above for error logging.
     */
    if (rule_cfg->err_log)
    {
        LOGD("rule_cfg->err_log");
        rule->err_log = logger_init(rule_cfg->err_log, 0);
    }
    else
    {
        LOGD("rproxy->err_log");
        rule->err_log  = rproxy->err_log;
    }
#endif//WITH_LOGGER

    return 0;
} /* map_default_rule_to_upstreams */

static int
add_vhost(lztq_elem* elem, void* arg)
{
    LOGD("(%p, %p)", elem, arg);

    rproxy_t* rproxy = arg;
    vhost_cfg_t* vhost_cfg = lztq_elem_data(elem);
    LOGD("vhost_cfg %p(%s)", vhost_cfg, vhost_cfg->server_name);

    vhost_t* vhost = g_new0(vhost_t, 1);
    g_assert(vhost != NULL);

    /* initialize the vhost specific logging, these are used if a rule
        * does not have its own logging configuration. This allows for rule
        * specific logs, and falling back to a global one.
        */
#ifdef WITH_LOGGER
    vhost->req_log = logger_init(vhost_cfg->req_log, 0);
    vhost->err_log = logger_init(vhost_cfg->err_log, 0);
#endif//WITH_LOGGER
    vhost->config  = vhost_cfg;
    vhost->rproxy  = rproxy;

    int res = lztq_for_each(vhost_cfg->rule_cfgs, map_vhost_rules_to_upstreams, vhost);
    g_assert(res == 0);

    return 0;
}

/**
 * @brief Before accepting an downstream connection, evhtp will call this function
 *        which will check whether we have hit our max-pending limits, and if so,
 *        inform evhtp to not accept().
 *
 * @param up_conn
 * @param arg
 *
 * @return
 */
evhtp_res
downstream_pre_accept(evhtp_connection_t* up_conn, void* arg)
{
    LOGD("(%p, %p)", up_conn, arg);

    rproxy_t* rproxy = arg;
    NOISY_MSG_("rproxy %p, server_cfg %p(%s)", rproxy, rproxy->server_cfg, rproxy->server_cfg->name);

    if (rproxy->server_cfg->max_pending <= 0)
    {
        /* configured with unlimited pending */
        LOGD("unlimited pending");
        return EVHTP_RES_OK;
    }

    /* check to see if we have too many pending requests, and if so, drop this
     * connection.
     */
    if ((g_atomic_int_get(&rproxy->m_thread_ctx->n_processing) + 1) > rproxy->server_cfg->max_pending)
    {
        g_error("Dropped connection, too many pending requests");
        return EVHTP_RES_ERROR;
    }

    g_atomic_int_inc(&rproxy->m_thread_ctx->n_processing);

    if (rproxy->server_cfg->disable_client_nagle)
    {
        /* config has requested that client sockets have the nagle algorithm
         * turned off.
         */

        NOISY_MSG_("disabling nagle");
        setsockopt(up_conn->sock, IPPROTO_TCP, TCP_NODELAY, (int[]) { 1 }, sizeof(int));
    }

    NOISY_MSG_("returning EVHTP_RES_OK");
    return EVHTP_RES_OK;
}

static inline RpNetworkTransportSocket*
create_transport_socket(evhtp_connection_t* conn)
{
    NOISY_MSG_("(%p)", conn);
    RpDownstreamTransportSocketFactory* factory = RP_DOWNSTREAM_TRANSPORT_SOCKET_FACTORY(transport_socket_factory);
    return rp_downstream_transport_socket_factory_create_downstream_transport_socket(factory, conn);
}

static evhtp_res
downstream_post_accept(evhtp_connection_t* up_conn, void* arg)
{
    LOGD("(%p, %p)", up_conn, arg);

    rproxy_t* rproxy = arg;
    NOISY_MSG_("rproxy %p, fd %d", rproxy, bufferevent_getfd(evhtp_connection_get_bev(up_conn)));

    RpConnectionManagerConfig* config = RP_CONNECTION_MANAGER_CONFIG(rproxy->m_filter_config);
    RpCommonFactoryContext* context = RP_COMMON_FACTORY_CONTEXT(rproxy->m_context);
    RpNetworkTransportSocket* transport_socket = create_transport_socket(up_conn);
    RpIoHandle* io_handle = rp_network_transport_socket_create_io_handle(transport_socket);
    g_autoptr(RpAcceptedSocketImpl) socket = rp_accepted_socket_impl_new(io_handle, up_conn);
    RpConnectionInfoSetter* info = rp_socket_connection_info_provider(RP_SOCKET(socket));
    g_autoptr(RpStreamInfoImpl) stream_info =
        rp_stream_info_impl_new(EVHTP_PROTO_INVALID,
                                RP_CONNECTION_INFO_PROVIDER(info),
                                RpFilterStateLifeSpan_Connection,
                                NULL);
    g_autoptr(RpNetworkServerConnectionImpl) connection =
        rp_network_server_connection_impl_new(rproxy->m_dispatcher,
                                                RP_CONNECTION_SOCKET(g_steal_pointer(&socket)),
                                                transport_socket,
                                                RP_STREAM_INFO(stream_info));
    g_autoptr(RpHttpConnectionManagerImpl) hcm =
        rp_http_connection_manager_impl_new(config,
                                            rp_common_factory_context_local_info(context),
                                            rproxy->m_cluster_manager);
    rp_network_filter_manager_add_read_filter(RP_NETWORK_FILTER_MANAGER(connection),
                                                RP_NETWORK_READ_FILTER(g_steal_pointer(&hcm)));
    g_autoptr(RpActiveTcpConn) active_conn =
        rp_active_tcp_conn_new(&rproxy->m_active_connections,
                                RP_NETWORK_CONNECTION(g_steal_pointer(&connection)),
                                RP_STREAM_INFO(g_steal_pointer(&stream_info)),
                                rproxy->m_thread_ctx);
    rproxy->m_active_connections = g_list_append(rproxy->m_active_connections,
                                                    g_steal_pointer(&active_conn));
    return EVHTP_RES_OK;
}

static evhtp_res
downstream_dump_request(evhtp_request_t * r, const char * host, void * arg)
{
    LOGD("(%p, %p(%s), %p)", r, host, host, arg);
    evhtp_send_reply(r, EVHTP_RES_NOTFOUND);

    return EVHTP_RES_ERROR;
}

/**
 * @brief the evthr backlog callback used by evthr to "calculate" the most
 *         appropriate threadpool worker to handle new connections.
 *
 * @param thr
 *
 * @return an integer number that indicates how "busy" |thr| is at the moment
 *          (the higher the number, the "busier" it is).
 */
#ifdef WITH_BACKLOG_GETTER
static int
get_backlog_(evthr_t* thr)
{
    NOISY_MSG_("(%p)", thr);

    rproxy_t* self = evthr_get_aux(thr);
    g_assert(self != NULL);

    return self->n_pending + self->n_processing;
}
#endif

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

static inline void
create_dispatcher(rproxy_t* rproxy, const gchar* name)
{
    RpTimeSystem* time_system = RP_TIME_SYSTEM(rp_real_time_system_new());
    rproxy->m_dispatcher = RP_DISPATCHER(rp_dispatcher_impl_new(name,
                                                                g_steal_pointer(&time_system),
                                                                rproxy->thr));
}

static inline void
create_cluster_manager(rproxy_t* rproxy)
{
    LOGD("(%p)", rproxy);

    g_autoptr(RpClusterFactory) factory = RP_CLUSTER_FACTORY(
        rp_static_cluster_factory_new());
    g_autoptr(RpClusterFactory) dfp_factory = RP_CLUSTER_FACTORY(
        rp_dfp_cluster_factory_new());
    RpGenericFactoryContext* generic_context = RP_GENERIC_FACTORY_CONTEXT(factory_context);
    RpServerFactoryContext* server_context = rp_generic_factory_context_server_factory_context(generic_context);
    g_autoptr(RpClusterManager) cluster_manager = RP_CLUSTER_MANAGER(
        rp_prod_cluster_manager_factory_new(server_context, RP_INSTANCE(server)));

    for (lztq_elem* elem = lztq_first(rproxy->rules); elem; elem = lztq_next(elem))
    {
        rule_t* rule = lztq_elem_data(elem);
        RpDiscoveryType_e cluster_discovery_type = translate_discovery_type(rule->config);
        RpClusterFactory* cluster_factory = cluster_discovery_type == RpDiscoveryType_STRICT_DNS ? dfp_factory : factory;
        gchar* name = g_strdup_printf("%p", rule->config);

        NOISY_MSG_("rule %p, rule->config %p, name %p(%s), dispatcher %p",
            rule, rule->config, name, name, rproxy->m_dispatcher);

        RpClusterCfg cluster = {
            .preconnect_policy = {
                .per_upstream_preconnect_ratio = 1.0,
                .predictive_preconnect_ratio = 1.0
            },
            .name = name,
            .cluster_discovery_type = cluster_discovery_type,
            .connect_timeout_secs = 5,
            .per_connection_buffer_limit_bytes = 1024*1024,
            .lb_policy = translate_lb_policy(rule->config),
            .dns_lookup_family = RpDnsLookupFamily_AUTO,
            .connection_pool_per_downstream_connection = false,
            .lb_endpoints = rule->upstreams,
            .factory = cluster_factory,
            .dispatcher = rproxy->m_dispatcher,
            .rule = rule
        };
        rp_cluster_manager_add_or_update_cluster(cluster_manager, &cluster, "");
    }

    rproxy->m_cluster_manager = g_steal_pointer(&cluster_manager);
    rproxy->m_context = server_context;
}

/**
 * @brief the evthr init callback. Setup rproxy event base and initialize
 *         upstream connections.
 *
 * @param htp
 * @param thr
 * @param arg
 */
static void
rproxy_thread_init(evhtp_t* htp, evthr_t* thr, void* arg)
{
    LOGD("(%p(%p), %p(%p), %p)", htp, htp->evbase, thr, evthr_get_base(thr), arg);

    g_return_if_fail(htp != NULL);
    g_return_if_fail(thr != NULL);
    g_return_if_fail(arg != NULL);

    g_object_ref(factory_context);
    g_object_ref(transport_socket_factory);
    g_object_ref(default_conn_pool_factory);
    g_object_ref(default_client_connection_factory);

    thread_ctx_t* thread_ctx = arg;
    g_atomic_rc_box_acquire(thread_ctx);

    rproxy_cfg_t* rproxy_cfg = thread_ctx->m_rproxy_cfg;
    server_cfg_t* server_cfg = thread_ctx->m_server_cfg;

    LOGD("server %p(%s)", server_cfg, server_cfg->name);

    evbase_t* evbase = evthr_get_base(thr);
    g_assert(evbase != NULL);

    rproxy_t* rproxy = g_new0(rproxy_t, 1);
    g_assert(rproxy != NULL);
    LOGD("rproxy %p", rproxy);

    rproxy->m_thread_ctx = thread_ctx;
    rproxy->worker_num = server_cfg_get_worker_num(server_cfg);

#ifdef WITH_LOGGER
    rproxy->req_log = logger_init(server_cfg->req_log_cfg, 0);
    rproxy->err_log = logger_init(server_cfg->err_log_cfg,
                                    LZLOG_OPT_WNAME | LZLOG_OPT_WPID | LZLOG_OPT_WDATE);
#endif//WITH_LOGGER

    rproxy->upstreams = lztq_new();
    g_assert(rproxy->upstreams != NULL);

    rproxy->rules = lztq_new();
    g_assert(rproxy->rules != NULL);

    /* create a dns_base which is used for various resolution functions, e.g.,
     * bufferevent_socket_connect_hostname()
     */
    rproxy->dns_base = evdns_base_new(evbase, 1);
    g_assert(rproxy->dns_base != NULL);

    rproxy->config     = rproxy_cfg;
    rproxy->server_cfg = server_cfg;
    rproxy->evbase     = evbase;
    rproxy->htp        = htp;
    rproxy->thr        = thr;

    g_autofree gchar* name = g_strdup_printf("%s(%u)", server_cfg->name, rproxy->worker_num);

    create_dispatcher(rproxy, name);

    RpRouteConfiguration route_config = {
        .name = server_cfg->name,//TODO...use |name| from above.(?)
        .virtual_hosts = server_cfg->vhosts,
        .max_direct_response_body_size_bytes = 4*1024,
        .ignore_port_in_host_matching = true,
        .ignore_path_paramaters_in_path_matching = false
    };
    RpHttpConnectionManagerCfg proto_config = {
        .codec_type = "HTTP1",
        .max_request_headers_kb = DEFAULT_MAX_REQUEST_HEADERS_KB,
        .http_protocol_options.max_headers_count = DEFAULT_MAX_HEADERS_COUNT,
        .route_config = route_config,
        .rules = rproxy->rules
    };
    rproxy->m_filter_config = rp_http_connection_manager_config_new(&proto_config,
                                                                    RP_FACTORY_CONTEXT(factory_context));

    /* create a pre-accept callback which will disconnect the client
     * immediately if the max-pending request queue is over the configured
     * limit.
     */
    evhtp_set_pre_accept_cb(htp, downstream_pre_accept, rproxy);
    /* Create a post-accept callback which will configure and allocate a newly-
     * accepted connection for processing.
     */
    evhtp_set_post_accept_cb(htp, downstream_post_accept, rproxy);

    /* create a upstream_t instance for each configured upstream */
    int res = lztq_for_each(server_cfg->upstreams, add_upstream, rproxy);
    g_assert(res == 0);

    /* set aux data argument to this threads specific rproxy_t structure */
    evthr_set_aux(thr, rproxy);

#ifdef WITH_BACKLOG_GETTER
    /* ser the thread backlog getter callback to our own */
    evthr_set_backlog_getter(thr, get_backlog_);
#endif

    /* for each virtual server, iterate over each rule_cfg and create a
     * rule_t structure.
     *
     * Since each of these rule_t's are unique pointers, we append them
     * to the global rproxy->rules list (evhtp takes care of the vhost
     * matching, and the rule_cfg is passed as the argument to
     * upstream_request_start_hook).
     *
     * Each rule_t has a upstreams list containing pointers to
     * (already allocated) upstream_t structures.
     */
    res = lztq_for_each(server_cfg->vhosts, add_vhost, rproxy);
    g_assert(res == 0);
#if 0
    lztq_elem* vhost_cfg_elem = lztq_first(server_cfg->vhosts);
    while (vhost_cfg_elem)
    {
        vhost_cfg_t* vhost_cfg = lztq_elem_data(vhost_cfg_elem);
        LOGD("vhost_cfg %p(%s)", vhost_cfg, vhost_cfg->server_name);

        vhost_t* vhost = g_new0(vhost_t, 1);
        g_assert(vhost != NULL);

        /* initialize the vhost specific logging, these are used if a rule
         * does not have its own logging configuration. This allows for rule
         * specific logs, and falling back to a global one.
         */
#ifdef WITH_LOGGER
        vhost->req_log = logger_init(vhost_cfg->req_log, 0);
        vhost->err_log = logger_init(vhost_cfg->err_log, 0);
#endif//WITH_LOGGER
        vhost->config  = vhost_cfg;
        vhost->rproxy  = rproxy;

        res = lztq_for_each(vhost_cfg->rule_cfgs, map_vhost_rules_to_upstreams, vhost);
        g_assert(res == 0);

        vhost_cfg_elem = lztq_next(vhost_cfg_elem);
    }
#endif//0

    if (server_cfg->default_rule_cfg)
    {
        server_cfg->default_rule = g_new0(rule_t, 1);
        g_assert(server_cfg->default_rule != NULL);
        map_default_rule_to_upstreams(server_cfg->default_rule, server_cfg->default_rule_cfg, rproxy);
        evhtp_set_gencb(htp, NULL, server_cfg->default_rule_cfg);
    }

    create_cluster_manager(rproxy);

    const rproxy_hooks_t* hooks = thread_ctx->m_hooks;
    if (hooks && hooks->on_thread_init)
    {
        LOGD("calling hooks->on_thread_init(%p, %p)", rproxy, hooks->on_thread_init_arg);
        hooks->on_thread_init(rproxy, hooks->on_thread_init_arg);
    }

    return;
} /* rproxy_thread_init */

static evhtp_t* create_htp(evbase_t* evbase, server_cfg_t* server_cfg);

static void
htp_worker_thread_init(evthr_t* thr, void* arg)
{
    LOGD("(%p, %p)", thr, arg);

    struct thread_ctx* thread_ctx = arg;
    struct server_cfg* server_cfg = thread_ctx->m_server_cfg;
    struct event_base* evbase = evthr_get_base(thr);
    struct evhtp* htp = create_htp(evbase, server_cfg);
    if (!htp)
    {
        g_error("alloc failed");
        evthr_stop(thr);
        return;
    }

    evhtp_enable_flag(htp, EVHTP_FLAG_ENABLE_REUSEPORT|EVHTP_FLAG_ENABLE_DEFER_ACCEPT);

    if (evhtp_bind_socket(htp,
                            server_cfg->bind_addr,
                            server_cfg->bind_port,
                            server_cfg->listen_backlog) < 0)
    {
        g_error("unable to bind to %s:%d (%s)",
                server_cfg->bind_addr,
                server_cfg->bind_port,
                strerror(errno));
        exit(-1);
    }

    if (server_cfg->disable_server_nagle)
    {
        LOGD("disabling nagle");

        /* disable the tcp nagle algorithm for the listener socket */
        evutil_socket_t sock = evconnlistener_get_fd(htp->server);
        g_assert(sock >= 0);

        setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (int[]) { 1 }, sizeof(int));
    }

    rproxy_thread_init(htp, thr, arg);

} /* htp_worker_thread_init */

static void
rproxy_thread_exit(evhtp_t* htp, evthr_t* thr, void* arg)
{
    LOGD("(%p, %p, %p)", htp, thr, arg);

    rproxy_t* rproxy = evthr_get_aux(thr);
    thread_ctx_t* thread_ctx = arg;

    const rproxy_hooks_t* hooks = thread_ctx->m_hooks;
    if (hooks && hooks->on_thread_exit)
    {
        LOGD("calling hooks->on_thread_exit(%p, %p)", rproxy, hooks->on_thread_exit_arg);
        hooks->on_thread_exit(rproxy, hooks->on_thread_exit_arg);
    }

    g_clear_object(&rproxy->m_filter_config);
    g_clear_object(&rproxy->m_dispatcher);

    g_object_unref(factory_context);
    g_object_unref(transport_socket_factory);
    g_object_unref(default_conn_pool_factory);
    g_object_unref(default_client_connection_factory);

    g_atomic_rc_box_release(thread_ctx);

} /* rproxy_thread_exit */

static void
htp_worker_thread_exit(evthr_t* thr, void* arg)
{
    LOGD("(%p, %p)", thr, arg);

    rproxy_t* rproxy = evthr_get_aux(thr);
    rproxy_thread_exit(rproxy->htp, thr, arg);
} /* htp_worker_thread_exit */

/**
 * @brief Set an evhtp callback based on information in a single rule_cfg_t
 *        structure. Based on the rule type, we either use set_cb, set_regex_cb,
 *        or set_glob_cb. Only one real callback set is an on_hostname hook.
 *
 * @param elem
 * @param arg
 *
 * @return
 */
static int
add_callback_rule(lztq_elem* elem, void* arg)
{
    LOGD("(%p, %p)", elem, arg);

    evhtp_t* htp = arg;
    g_assert(htp != NULL);

    rule_cfg_t* rule = lztq_elem_data(elem);
    LOGD("rule %p(%s), matchstr %p(%s)", rule, rule->name, rule->matchstr, rule->matchstr);

    evhtp_callback_t* cb = NULL;
    switch (rule->type)
    {
        case rule_type_exact:
            LOGD("exact");
            cb = evhtp_set_cb(htp, rule->matchstr, NULL, rule);
            break;
        case rule_type_regex:
            LOGD("regex");
            cb = evhtp_set_regex_cb(htp, rule->matchstr, NULL, rule);
            break;
        case rule_type_glob:
            LOGD("glob");
            cb = evhtp_set_glob_cb(htp, rule->matchstr, NULL, rule);
            break;
        case rule_type_default:
            LOGD("default");
        default:
            /* default rules will match anything */
            cb = evhtp_set_glob_cb(htp, "*", NULL, rule);
            break;
    }

    if (cb == NULL)
    {
        g_error("Could not compile evhtp callback for pattern \"%s\"", rule->matchstr);
// REVISIT: should not be fatal.(?)
        exit(EXIT_FAILURE);
    }

    return 0;
}

static int
add_vhost_name(lztq_elem* elem, void* arg)
{
    LOGD("(%p, %p)", elem, arg);

    evhtp_t* htp_vhost = arg;
    g_assert(htp_vhost != NULL);

    char* name = lztq_elem_data(elem);
    g_assert(name != NULL);

    NOISY_MSG_("calling evhtp_add_alias(%p, %p(%s))", htp_vhost, name, name);
    return evhtp_add_alias(htp_vhost, name);
}

static int
add_evhtp_vhost(lztq_elem* elem, void* arg)
{
    LOGD("(%p, %p)", elem, arg);

    /* create a new child evhtp for this vhost */
    evhtp_t* htp = arg;
    evhtp_t* htp_vhost = evhtp_new(htp->evbase, NULL);

    /* disable 100-continue responses, we let the upstreams deal with this.
     */
    evhtp_disable_flag(htp_vhost, EVHTP_FLAG_ENABLE_100_CONT);

    /* for each rule, create a evhtp callback with the defined type */
    vhost_cfg_t* vcfg = lztq_elem_data(elem);
    lztq_for_each(vcfg->rule_cfgs, add_callback_rule, htp_vhost);

    /* add the vhost using the name of the vhost config directive */
    evhtp_add_vhost(htp, vcfg->server_name, htp_vhost);
    LOGD("vhost server name %p(%s)", vcfg->server_name, vcfg->server_name);

    /* create an alias for the vhost for each configured alias directive */
    lztq_for_each(vcfg->aliases, add_vhost_name, htp_vhost);

    /* here we add a rule to rule them all. If no rules match, we want to be
     * able to shut the connection down so it doesn't consume any resources.
     *
     * If this is not done, evhtp will just keep consuming input data, and in
     * the case of a large POST request, memory will be eaten until complete.
     * This is bad.
     */
    {
        evhtp_callback_t* cb;

        if (!(cb = evhtp_set_glob_cb(htp_vhost, "*", NULL, htp_vhost)))
        {
            gint errnum = errno;
            g_error("Could not alloc callback: %s", g_strerror(errnum));
            exit(EXIT_FAILURE);
        }

        evhtp_callback_set_hook(cb, evhtp_hook_on_hostname,
                                    (evhtp_hook)downstream_dump_request,
                                    htp_vhost);
    }

    if (vcfg->ssl_cfg != NULL)
    {
        /* vhost specific ssl configuration found */
        evhtp_ssl_init(htp_vhost, vcfg->ssl_cfg);

        /* if CRL checking is enabled, create a new ssl_crl_ent_t and add it
         * to the evhtp_t's arguments. XXX: in the future we should create a
         * generic wrapper for various things we want to put in the evhtp
         * arguments, but for now, the only thing that we care about is the
         * CRL context.
         */
        if (vcfg->ssl_cfg->args)
        {
            ssl_crl_cfg_t* crl_cfg = vcfg->ssl_cfg->args;

            htp->arg = (void *)ssl_crl_ent_new(htp, crl_cfg);
            g_assert(htp->arg != NULL);
        }
    }
    else if (htp->ssl_ctx != NULL)
    {
        /* use the global SSL context */
        htp_vhost->ssl_ctx = htp->ssl_ctx;
        htp_vhost->ssl_cfg = htp->ssl_cfg;
        htp_vhost->arg     = htp->arg;
    }

    return 0;
} /* add_evhtp_vhost */

static evhtp_t*
create_htp(evbase_t* evbase, server_cfg_t* server_cfg)
{
    LOGD("(%p, %p(%s))", evbase, server_cfg, server_cfg->name);

    /* create a new evhtp base structure for just this server, all vhosts
     * will use this as a parent
     */
    evhtp_t* htp = evhtp_new(evbase, NULL);
    if (!htp)
    {
        g_error("alloc failed");
        return NULL;
    }

    /* we want to make sure 100-continue is not sent by evhtp, but the
     * upstreams themselves.
     */
    evhtp_disable_flag(htp, EVHTP_FLAG_ENABLE_100_CONT);

    if (server_cfg->ssl_cfg)
    {
        LOGD("configuring SSL support");
        /* enable SSL support on this server */
        evhtp_ssl_init(htp, server_cfg->ssl_cfg);

        /* if CRL checking is enabled, create a new ssl_crl_ent_t and add it
         * to the evhtp_t's arguments. XXX: in the future we should create a
         * generic wrapper for various things we want to put in the evhtp
         * arguments, but for now, the only thing that we care about is the
         * CRL context.
         */
        if (server_cfg->ssl_cfg->args)
        {
            ssl_crl_cfg_t* crl_cfg = server_cfg->ssl_cfg->args;

            htp->arg = ssl_crl_ent_new(htp, crl_cfg);
            g_assert(htp->arg != NULL);
        }
    }

    /* for each vhost, create a child virtual host and stick it in our main
     * evhtp structure.
     */
    lztq_for_each(server_cfg->vhosts, add_evhtp_vhost, htp);

    struct timeval* tv_read  = NULL;
    /* if configured, set our upstream connection's read/write timeouts */
    if (server_cfg->read_timeout.tv_sec || server_cfg->read_timeout.tv_usec)
    {
        LOGD("tv_read %zu.%zu seconds",
            server_cfg->read_timeout.tv_sec, server_cfg->read_timeout.tv_usec);
        tv_read = &server_cfg->read_timeout;
    }

    struct timeval* tv_write = NULL;
    if (server_cfg->write_timeout.tv_sec || server_cfg->write_timeout.tv_usec)
    {
        LOGD("tv_write %zu.%zu seconds",
            server_cfg->write_timeout.tv_sec, server_cfg->write_timeout.tv_usec);
        tv_write = &server_cfg->write_timeout;
    }

    if (tv_read || tv_write)
    {
        LOGD("setting evhtp timeouts");
        evhtp_set_timeouts(htp, tv_read, tv_write);
    }

    return htp;
} /* create_htp */

typedef struct add_server_ctx add_server_ctx_t;
struct add_server_ctx {
    const rproxy_hooks_t* hooks;
    rproxy_cfg_t* rproxy_cfg;
    evbase_t* evbase;
};

static int
add_server(lztq_elem* elem, void* arg)
{
    NOISY_MSG_("(%p, %p)", elem, arg);

    server_cfg_t* server_cfg = lztq_elem_data(elem);
    g_assert(server_cfg != NULL);

    g_info("Server %s(%s:%d)", server_cfg->name, server_cfg->bind_addr, server_cfg->bind_port);

    add_server_ctx_t* server_ctx = arg;
    thread_ctx_t* thread_ctx = g_atomic_rc_box_new0(thread_ctx_t);
    thread_ctx->m_hooks = server_ctx->hooks;
    thread_ctx->m_rproxy_cfg = server_ctx->rproxy_cfg;
    thread_ctx->m_server_cfg = server_cfg;

    // Use a single listener for all workers unless the
    // enable-workers-listen cfg option is set.
    if (server_cfg->enable_workers_listen)
    {
        evthr_pool_t* workers = evthr_pool_wexit_new(server_cfg->num_threads,
                                                        htp_worker_thread_init,
                                                        htp_worker_thread_exit,
                                                        thread_ctx);
        g_assert(workers != NULL);

        evthr_pool_start(workers);

        server_cfg->m_thr_pool = workers;
    }
    else
    {
        evhtp_t* htp = create_htp(server_ctx->evbase, server_cfg);
        g_assert(htp != NULL);

        /* use a worker thread pool for connections, and for each
            * thread that is initialized call the rproxy_thread_init
            * function. rproxy_thread_init will create a new rproxy_t
            * instance for each of the threads
            */
        evhtp_use_threads_wexit(htp,
                                rproxy_thread_init,
                                rproxy_thread_exit,
                                server_cfg->num_threads,
                                thread_ctx);

        if (evhtp_bind_socket(htp,
                                server_cfg->bind_addr,
                                server_cfg->bind_port,
                                server_cfg->listen_backlog) < 0)
        {
            int err = errno;
            g_error("unable to bind to %s:%d (%s)",
                    server_cfg->bind_addr,
                    server_cfg->bind_port,
                    g_strerror(err));
            exit(-1);
        }

        if (server_cfg->disable_server_nagle)
        {
            LOGD("disabling nagle");

            // Disable the tcp nagle algorithm for the listener socket.
            evutil_socket_t sock = evconnlistener_get_fd(htp->server);
            g_assert(sock >= 0);
            setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (int[]) { 1 }, sizeof(int));
        }

        server_cfg->m_htp = htp;
    }

    server_cfg->m_arg = thread_ctx;

    return 0;
}
static bool
rproxy_init(evbase_t* evbase, rproxy_cfg_t* cfg, const rproxy_hooks_t* hooks)
{
    LOGD("(%p, %p, %p)", evbase, cfg, hooks);

    g_return_val_if_fail(evbase != NULL, false);
    g_return_val_if_fail(cfg != NULL, false);
    g_return_val_if_fail(hooks != NULL, false);

    if (hooks->on_cfg_init && !hooks->on_cfg_init(cfg, hooks->on_cfg_init_arg))
    {
        g_error("alloc failed");
        return false;
    }

    add_server_ctx_t server_ctx = {
        .hooks = hooks,
        .rproxy_cfg = cfg,
        .evbase = evbase
    };

    // Iterate over each server_cfg, and set up evhtp stuff.
    lztq_for_each(cfg->servers, add_server, &server_ctx);
#if 0
    for (lztq_elem* elem = lztq_first(cfg->servers); elem; elem = lztq_next(elem))
    {
        server_cfg_t* server_cfg = lztq_elem_data(elem);
        g_assert(server_cfg != NULL);

        LOGD("server %p(%s:%d)", server_cfg, server_cfg->bind_addr, server_cfg->bind_port);

        thread_ctx_t* thread_ctx = g_atomic_rc_box_new0(thread_ctx_t);
        thread_ctx->m_hooks = hooks;
        thread_ctx->m_rproxy_cfg = cfg;
        thread_ctx->m_server_cfg = server_cfg;

        // Use a single listener for all workers unless the
        // enable-workers-listen cfg option is set.
        if (server_cfg->enable_workers_listen)
        {
            evthr_pool_t* workers = evthr_pool_wexit_new(server_cfg->num_threads,
                                                            htp_worker_thread_init,
                                                            htp_worker_thread_exit,
                                                            thread_ctx);
            g_assert(workers != NULL);

            evthr_pool_start(workers);

            server_cfg->m_thr_pool = workers;
        }
        else
        {
            evhtp_t* htp = create_htp(evbase, server_cfg);
            g_assert(htp != NULL);

            /* use a worker thread pool for connections, and for each
             * thread that is initialized call the rproxy_thread_init
             * function. rproxy_thread_init will create a new rproxy_t
             * instance for each of the threads
             */
            evhtp_use_threads_wexit(htp,
                                    rproxy_thread_init,
                                    rproxy_thread_exit,
                                    server_cfg->num_threads,
                                    thread_ctx);

            if (evhtp_bind_socket(htp,
                                    server_cfg->bind_addr,
                                    server_cfg->bind_port,
                                    server_cfg->listen_backlog) < 0)
            {
                g_error("unable to bind to %s:%d (%s)",
                        server_cfg->bind_addr,
                        server_cfg->bind_port,
                        strerror(errno));
                exit(-1);
            }

            if (server_cfg->disable_server_nagle)
            {
                LOGD("disabling nagle");

                // Disable the tcp nagle algorithm for the listener socket.
                evutil_socket_t sock = evconnlistener_get_fd(htp->server);
                g_assert(sock >= 0);
                setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (int[]) { 1 }, sizeof(int));
            }

            server_cfg->m_htp = htp;
        }

        server_cfg->m_arg = thread_ctx;
    }
#endif//0

    NOISY_MSG_("done");
    return true;
} /* rproxy_init */

static void
rproxy_report_rusage(rproxy_rusage_t * rusage)
{
    LOGD("(%p)", rusage);

    g_return_if_fail(rusage != NULL);

    g_info("Configured resource usage information");
    g_info("  Number of upstream connections : %u", rusage->total_num_connections);
    g_info("  Number of threads                : %u", rusage->total_num_threads);
    g_info("  Number of pending connections    : %u\n", rusage->total_max_pending);

    g_info("Detailed resource rundown\n");
    g_info("  A thread requires 2 fds            : %u", rusage->total_num_threads * 2);
    g_info("  A ds connection is 1 fd * nthreads : %u",
           (rusage->total_num_connections * rusage->total_num_threads));
    g_info("  Base fd resources needed           : %u\n",
           ((rusage->total_num_connections) * (rusage->total_num_threads * 2)));

    g_info("  In a worst-case scenario where all pending queues are filled, you "
           "will need a bit more than %u descriptors available\n",
           ((rusage->total_num_connections) * (rusage->total_num_threads * 2)) +
           rusage->total_max_pending);

#ifndef NO_RLIMITS
    {
        struct rlimit limit;
        unsigned int  needed_fds;

        if (getrlimit(RLIMIT_NOFILE, &limit) == -1)
        {
            g_error("getrlimi() failed");
            return;
        }

        needed_fds = ((rusage->total_num_connections) *
                      (rusage->total_num_threads * 2)) + rusage->total_max_pending;

        g_info("  Your *CURRENT* rlimit is : %u", (unsigned int)limit.rlim_cur);
        g_info("  Your *MAXIMUM* rlimit is : %u\n", (unsigned int)limit.rlim_max);

        if ((unsigned int)limit.rlim_cur < needed_fds)
        {
            g_warning("Needed resources exceed the current limits!");

            if ((unsigned int)limit.rlim_max > needed_fds)
            {
                g_warning("The good news is that you do have the "
                       "ability to obtain the resources! Try increasing the "
                       "option 'max-nofiles' in your configuration.");
            }
            else
            {
                g_warning("You must consult your operating systems "
                       "documentation in order to increase the resources "
                       "needed.");
            }
        }
    }
#endif
} /* rproxy_report_rusage */

static void
sigint_cb(int fd G_GNUC_UNUSED, short event, void* arg)
{
    printf("\n");
    LOGD("(%d, %d, %p)", fd, event, arg);

    g_info("Shutting down");

    rproxy_cfg_t* cfg = arg;
    g_assert(cfg != NULL);

    evbase_t* evbase = cfg->evbase;
    g_assert(evbase != NULL);

    const rproxy_hooks_t* hooks = rp_instance_hooks(RP_INSTANCE(server));
    if (hooks->on_sigint) hooks->on_sigint(cfg, hooks->on_sigint_arg);

    /* iterate over each server_cfg, and tear down up evhtp stuff */
    lztq_elem * serv_elem = lztq_first(cfg->servers);
    while (serv_elem)
    {
        server_cfg_t* server_cfg = lztq_elem_data(serv_elem);
        g_assert(server_cfg != NULL);

        if (server_cfg->m_thr_pool)
        {
            evthr_pool_t* thr_pool = server_cfg->m_thr_pool;
            LOGD("stopping pool %p", thr_pool);
            evthr_pool_stop(thr_pool);
            g_clear_pointer(&server_cfg->m_thr_pool, evthr_pool_free);
        }
        else
        {
            LOGD("freeing htp %p", server_cfg->m_htp);
            g_clear_pointer(&server_cfg->m_htp, evhtp_free);
        }

        LOGD("releasing %p", server_cfg->m_arg);
        g_atomic_rc_box_release(server_cfg->m_arg);

        serv_elem = lztq_next(serv_elem);
    }

    struct timeval tv = {
        .tv_sec = 0,
        .tv_usec = 100000
    }; // 100 ms.
    event_base_loopexit(evbase, &tv);
}

static void
create_server_instance(const rproxy_hooks_t* hooks)
{
    NOISY_MSG_("(%p)", hooks);
    server = rp_instance_impl_new(hooks);
    rp_instance_base_initialize(RP_INSTANCE_BASE(server));
}

static void
create_rp_instances(const rproxy_hooks_t* hooks)
{
    NOISY_MSG_("(%p)", hooks);
    // Create a single RpInstanceImpl instance.
    create_server_instance(hooks);
    // Create a single RpFactoryContext instance.
    factory_context = rp_factory_context_impl_new(RP_INSTANCE(server));
    // Create a single RpTransportSocketFactoryImpl instance.
    transport_socket_factory = rp_transport_socket_factory_impl_new(NULL);
    // Create a global, default RpPerHostGenericConnPoolFactory instance.
    default_conn_pool_factory = rp_per_host_generic_conn_pool_factory_new();
    // Create a global, default RpDefaultClientConnectionFactory instance.
    default_client_connection_factory = rp_default_client_connection_factory_new();
}

static void
clear_rp_instances(void)
{
    NOISY_MSG_("()");
    g_clear_object(&default_conn_pool_factory);
    g_clear_object(&default_client_connection_factory);
    g_clear_object(&transport_socket_factory);
    g_clear_object(&factory_context);
    g_clear_object(&server);
}

bool
rproxy_add_server_cfg(rproxy_cfg_t* rproxy_cfg, const char* server_cfg_buf)
{
    LOGD("(%p, %p(%s))", rproxy_cfg, server_cfg_buf, server_cfg_buf);

    g_return_val_if_fail(rproxy_cfg != NULL, false);
    g_return_val_if_fail(server_cfg_buf != NULL, false);
    g_return_val_if_fail(server_cfg_buf[0] != 0, false);

    return rproxy_cfg_parse_server_buf(rproxy_cfg, server_cfg_buf);
}

int
rproxy_main(int argc, char** argv, const rproxy_hooks_t* hooks)
{
    LOGD("(%d, %p, %p)", argc, argv, hooks);

    if (argc < 2)
    {
        printf("RProxy Version: %s (Libevhtp Version: %s, Libevent Version: %s, OpenSSL Version: %s)\n",
               RPROXY_VERSION, EVHTP_VERSION, event_get_version(), SSLeay_version(SSLEAY_VERSION));
        g_error("Usage: %s <config>", argv[0]);
        return -1;
    }

    rproxy_cfg_t* rproxy_cfg = rproxy_cfg_parse(argv[1]);
    if (!rproxy_cfg)
    {
        g_error("Error parsing file %s", argv[1]);
        return -1;
    }

#ifdef USE_MALLOPT
    if (rproxy_cfg->mem_trimsz)
    {
        LOGD("using mallopt");
        mallopt(M_TRIM_THRESHOLD, rproxy_cfg->mem_trimsz);
    }
#endif

    if (util_set_rlimits(rproxy_cfg->max_nofile) < 0)
    {
        g_error("set rlimits failed");
        exit(-1);
    }

    /* calculate and report on resources needed for the current configuration */
    rproxy_report_rusage(&rproxy_cfg->rusage);

    if (rproxy_cfg->daemonize == true)
    {
        if (util_daemonize(rproxy_cfg->rootdir, 1) < 0)
        {
            g_error("deamonize failed");
            exit(-1);
        }
    }

    // Create the rp instance singletons.
    create_rp_instances(hooks);

    evbase_t* evbase = event_base_new();
    if (!evbase)
    {
        g_error("Error creating event_base: %s\n", g_strerror(errno));
        rproxy_cfg_free(rproxy_cfg);
        return -1;
    }

    // Associate the evbase with the cfg object to use during shutdown.
    rproxy_cfg->evbase = evbase;

    // Install Ctrl+C/SIGINT handler for graceful shutdown.
    struct event* ev_sigint = evsignal_new(evbase, SIGINT, sigint_cb, rproxy_cfg);
    evsignal_add(ev_sigint, NULL);

    if (!rproxy_init(evbase, rproxy_cfg, hooks))
    {
        g_error("rproxy_init() failed");
        rproxy_cfg_free(rproxy_cfg);
        return -1;
    }

    if (rproxy_cfg->user || rproxy_cfg->group) {
        util_dropperms(rproxy_cfg->user, rproxy_cfg->group);
    }

    g_info("Entering main loop");

    // Run the main event loop until it is instructed to exit.
    event_base_loop(evbase, 0);

    event_free(ev_sigint);

    // Clear the rp instance singletons.
    clear_rp_instances();

    LOGD("done");
    return 0;
} /* rproxy_main */
