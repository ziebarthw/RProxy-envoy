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

#include "macrologger.h"

#if (defined(rproxy_NOISY) || defined(ALL_NOISY)) && !defined(NO_rproxy_NOISY)
#   define NOISY_MSG_ LOGD
#   define IF_NOISY_(x, ...) x##__VA_ARGS__
#else
#   define NOISY_MSG_(x, ...)
#   define IF_NOISY_(x, ...)
#endif

#include "rproxy.h"
#include "rule.h"
#include "tpoolctx.h"
#include "trafficstats.h"
#include "vhost.h"

#include "clusters/static/rp-static-cluster.h"
#include "clusters/strict_dns/rp-strict-dns-cluster.h"
#include "dynamic_forward_proxy/rp-cluster.h"
#include "event/rp-dispatcher-impl.h"
#include "event/rp-real-time-system.h"
#include "network/rp-accepted-socket-impl.h"
#include "network/rp-default-client-conn-factory.h"
#include "network/rp-socket-interface-impl.h"
#include "router/rp-route-provider-manager.h"
#include "router/rp-router-config-impl.h"
#include "router/rp-router-filter.h"
#include "server/rp-server-instance-impl.h"
#include "server/rp-factory-context-impl.h"
#include "stream_info/rp-stream-info-impl.h"
#include "thread_local/rp-thread-local-impl.h"
#include "transport_sockets/rp-raw-buffer.h"
#include "upstream/rp-cluster-factory-impl.h"
#include "upstream/rp-cluster-manager-impl.h"
#include "upstream/rp-cluster-provided-lb-factory.h"
#include "upstream/rp-upstream-impl.h"
#include "rp-active-tcp-conn.h"
#include "rp-cluster-configuration.h"
#include "rp-codec-client-prod.h"
#include "rp-headers.h"
#include "rp-http-conn-manager-config.h"
#include "rp-http-conn-manager-impl.h"
#include "rp-net-server-conn-impl.h"
#include "rp-per-host-upstream.h"

static RpFactoryContext* factory_context = NULL; // It *appears* this is the listener_manger (or listener?) in envoy.???
static GMutex thread_waiter_mutex;
static GCond thread_ready_cond;
static GCond thread_go_cond;
static int ready_workers = 0;
static int total_workers = 0;
static bool go_ahead = false;

RpPerHostGenericConnPoolFactory* default_conn_pool_factory = NULL;
RpDefaultClientConnectionFactory* default_client_connection_factory = NULL;
RpClusterFactory* default_static_cluster_factory = NULL;
RpClusterFactory* default_strict_dns_cluster_factory = NULL;
RpClusterFactory* default_dfp_cluster_factory = NULL;
RpNetworkAddressSocketInterfaceImplFactory* default_network_address_socket_interface_impl_factory = NULL;
RpUpstreamTransportSocketConfigFactory* default_upstream_transport_socket_config_factory = NULL;
RpDownstreamTransportSocketConfigFactory* default_downstream_transport_socket_config_factory = NULL;
RpDfpClusterStoreFactory* default_cluster_store_factory = NULL;
RpTypedLoadBalancerFactory* default_cluster_provided_lb_factory = NULL;
RpRouteConfigProviderManagerFactory* default_route_config_provider_manager_factory = NULL;

/**
 * @brief allocates a new upstream_t, and appends it to the
 *        rproxy->upstreams list. This is callback for the
 *        g_slist_for_each function from rproxy_thread_init().
 *
 * @param elem
 * @param arg
 *
 * @return
 */
static void
add_upstream(gpointer data, gpointer arg)
{
    LOGD("(%p, %p)", data, arg);

    rproxy_t* rproxy = arg;
    g_assert(rproxy != NULL);

    upstream_cfg_t* ds_cfg = data;
    upstream_t* upstream = upstream_new(ds_cfg);
    rproxy->upstreams = g_slist_prepend(rproxy->upstreams, upstream);
}

static int
map_rule_to_upstreams(rule_t* rule, rule_cfg_t* rule_cfg, rproxy_t* rproxy)
{
    NOISY_MSG_("(%p, %p(%s), %p)", rule, rule_cfg, rule_cfg->name, rproxy);

    /* for each string in the rule_cfg's upstreams section, find the matching
     * upstream_t and append it.
     */
    for (GSList* itr = rule_cfg->upstream_names; itr; itr = itr->next)
    {
        const char* up_name = itr->data;
        g_assert(up_name != NULL);

        upstream_t* up;
        if (!(up = upstream_find_by_name(rproxy->upstreams, up_name)))
        {
            /* could not find a upstream_t which has this name! */
            g_error("Could not find upstream named \"%s\"!", up_name);
            exit(EXIT_FAILURE);
        }

        rule->upstreams = g_slist_prepend(rule->upstreams, up);
    }

    /* upstream_request_start_hook is passed only the rule_cfg as an argument.
     * This function will call find_rule_from_cfg to get the actual rule from
     * the global rproxy->rules list. Since we compare pointers, it is safe to
     * keep this as one single list.
     */
    rproxy->rules = g_slist_prepend(rproxy->rules, rule);

    return 0;
} /* map_rule_to_upstreams */

/**
 * @brief match up names in the list of upstream_cfg_t's in rule_cfg->upstreams
 *        to the upstream_t's in the rproxy->upstreams list. If found,
 *        create a rule_t and appends it to the rproxy->rules list.
 *
 * @param data a GSList elem with the type of vhost_cfg_t *
 * @param arg  the vhost_t *
 *
 * @return
 */
static void
map_vhost_rules_to_upstreams(gpointer data, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", data, arg);

    vhost_t* vhost = arg;
    rproxy_t* rproxy = vhost->rproxy;
    rule_cfg_t* rule_cfg = data;
    NOISY_MSG_("vhost %p(%s), rule_cfg %p(%s)", vhost, vhost->config->server_name, rule_cfg, rule_cfg->name);

    rule_t* rule = rule_new(rule_cfg, vhost);
    vhost->config->rules = g_slist_append(vhost->config->rules, rule);
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
} /* map_vhost_rules_to_upstreams */

static void
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
} /* map_default_rule_to_upstreams */

static void
add_vhost(gpointer data, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", data, arg);

    rproxy_t* rproxy = arg;
    vhost_cfg_t* vhost_cfg = data;
    vhost_t* vhost = vhost_new(vhost_cfg);
    NOISY_MSG_("vhost %p, vhost_cfg %p(%s)", vhost, vhost_cfg, vhost_cfg->server_name);

    rproxy->vhosts = g_slist_prepend(rproxy->vhosts, vhost);

    /* initialize the vhost specific logging, these are used if a rule
        * does not have its own logging configuration. This allows for rule
        * specific logs, and falling back to a global one.
        */
#ifdef WITH_LOGGER
    vhost->req_log = logger_init(vhost_cfg->req_log, 0);
    vhost->err_log = logger_init(vhost_cfg->err_log, 0);
#endif//WITH_LOGGER
    vhost->rproxy  = rproxy;

    vhost_cfg->server_cfg = rproxy->server_cfg;

    g_slist_foreach(vhost_cfg->rule_cfgs, map_vhost_rules_to_upstreams, vhost);
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
downstream_pre_accept(evhtp_connection_t* up_conn, gpointer arg)
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
    if ((n_processing_read(rproxy->m_tpool_ctx->n_processing) + 1) > rproxy->server_cfg->max_pending)
    {
        g_error("Dropped connection, too many pending requests");
        return EVHTP_RES_ERROR;
    }

    n_processing_inc(rproxy->m_tpool_ctx->n_processing);

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

static inline RpDownstreamTransportSocketFactoryPtr
ensure_transport_socket_factory(rproxy_t* rproxy)
{
    NOISY_MSG_("(%p)", rproxy);
    if (rproxy->m_transport_socket_factory)
    {
        NOISY_MSG_("pre-allocated transport socket factory %p", rproxy->m_transport_socket_factory);
        return rproxy->m_transport_socket_factory;
    }
    RpTransportSocketFactoryContextPtr context = rp_factory_context_get_transport_socket_factory_context(factory_context);
    rproxy->m_transport_socket_factory = rp_downstream_transport_socket_config_factory_create_transport_socket_factory(
                                                                default_downstream_transport_socket_config_factory,
                                                                rproxy->server_cfg,
                                                                context);
    NOISY_MSG_("allocated transport socket factory %p", rproxy->m_transport_socket_factory);
    return rproxy->m_transport_socket_factory;
}

static inline RpNetworkTransportSocketPtr
create_transport_socket(rproxy_t* rproxy, evhtp_connection_t* conn)
{
    NOISY_MSG_("(%p)", conn);
    RpDownstreamTransportSocketFactoryPtr factory = ensure_transport_socket_factory(rproxy);
    return rp_downstream_transport_socket_factory_create_downstream_transport_socket(factory, conn);
}

static evhtp_res
downstream_post_accept(evhtp_connection_t* up_conn, gpointer arg)
{
    LOGD("(%p, %p)", up_conn, arg);

    rproxy_t* rproxy = arg;
    NOISY_MSG_("rproxy %p, fd %d", rproxy, bufferevent_getfd(evhtp_connection_get_bev(up_conn)));

    stats_inc(g_traffic_stats.downstream_cx_total);
    stats_inc(g_traffic_stats.downstream_cx_active);

    RpConnectionManagerConfig* config = RP_CONNECTION_MANAGER_CONFIG(rproxy->m_filter_config);
    RpServerFactoryContext* server_context =
        rp_generic_factory_context_server_factory_context(RP_GENERIC_FACTORY_CONTEXT(factory_context));
    RpCommonFactoryContext* context = RP_COMMON_FACTORY_CONTEXT(server_context);
    g_autoptr(RpNetworkTransportSocket) transport_socket = create_transport_socket(rproxy, up_conn);
    RpIoHandle* io_handle = rp_network_transport_socket_create_io_handle(transport_socket);
    g_autoptr(RpAcceptedSocketImpl) socket = rp_accepted_socket_impl_new(io_handle, up_conn);
    RpConnectionInfoSetterSharedPtr info = rp_socket_connection_info_provider(RP_SOCKET(socket));
    g_autoptr(RpStreamInfoImpl) stream_info =
        rp_stream_info_impl_new(EVHTP_PROTO_INVALID,
                                RP_CONNECTION_INFO_PROVIDER(info),
                                RpFilterStateLifeSpan_Connection,
                                NULL);
    g_autoptr(RpNetworkServerConnectionImpl) connection =
        rp_network_server_connection_impl_new(rproxy->m_dispatcher,
                                                RP_CONNECTION_SOCKET(g_steal_pointer(&socket)),
                                                g_steal_pointer(&transport_socket),
                                                RP_STREAM_INFO(stream_info));
    g_autoptr(RpHttpConnectionManagerImpl) hcm =
        rp_http_connection_manager_impl_new(config,
                                            rp_common_factory_context_local_info(context),
                                            rp_common_factory_context_cluster_manager(context));
    rp_network_filter_manager_add_read_filter(RP_NETWORK_FILTER_MANAGER(connection),
                                                RP_NETWORK_READ_FILTER(hcm));
    g_clear_object(&hcm); // Ownership transferred to network filter manager.
    g_autoptr(RpActiveTcpConn) active_conn =
        rp_active_tcp_conn_new(&rproxy->m_active_connections,
                                RP_NETWORK_CONNECTION(g_steal_pointer(&connection)),
                                RP_STREAM_INFO(g_steal_pointer(&stream_info)),
                                rproxy->m_tpool_ctx);
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

static inline RpDispatcherPtr
create_dispatcher(const gchar* name, evthr_t* thr)
{
    RpTimeSystem* time_system = RP_TIME_SYSTEM(rp_real_time_system_new());
    return rp_dispatcher_impl_new(name, g_steal_pointer(&time_system), thr);
}

rproxy_t*
rproxy_new(rproxy_t* parent, rproxy_cfg_t* config)
{
    LOGD("(%p, %p)", parent, config);
    g_return_val_if_fail(parent || config, NULL);
    g_return_val_if_fail(!parent || !config, NULL);
    g_autoptr(rproxy_t) self = g_new0(rproxy_t, 1);
    self->m_parent = parent;
    self->config = parent ? parent->config : config;
    NOISY_MSG_("self %p, %zu bytes", self, sizeof(*self));
    return g_steal_pointer(&self);
}

void
rproxy_free(rproxy_t* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(self != NULL);
    if (!self->m_parent)
    {
        g_clear_pointer(&self->config, rproxy_cfg_free);
        g_slist_free_full(g_steal_pointer(&self->rules), (GDestroyNotify)rule_free);
        g_slist_free_full(g_steal_pointer(&self->upstreams), (GDestroyNotify)upstream_free);
        g_slist_free_full(g_steal_pointer(&self->vhosts), (GDestroyNotify)vhost_free);
    }
    g_free(self);
}

static void
continue_thread_init_cb(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);

    evthr_t* thr = arg;
    rproxy_t* rproxy = evthr_get_aux(thr);
    tpool_ctx_t* tpool_ctx = rproxy->m_tpool_ctx;

    switch (rproxy->m_state)
    {
        case RpWorkerContinue_CONN_MANAGER:
        {
            server_cfg_t* server_cfg = tpool_ctx->m_server_cfg;
            RpRouteConfiguration route_config = {
                .name = server_cfg->name,//TODO...use |name| from above.(?)
                .virtual_hosts = server_cfg->vhost_cfgs,
                .max_direct_response_body_size_bytes = 4*1024,
                .ignore_port_in_host_matching = true,
                .ignore_path_paramaters_in_path_matching = false
            };
            RpHttpConnectionManagerCfg proto_config = {
                .codec_type = "HTTP1",
                .max_request_headers_kb = DEFAULT_MAX_REQUEST_HEADERS_KB,
                .http_protocol_options.max_headers_count = DEFAULT_MAX_HEADERS_COUNT,
                .route_config = route_config,
                .rules = rproxy->m_parent->rules
            };
            rproxy->m_filter_config = rp_http_connection_manager_config_new(&proto_config,
                                                                            factory_context);
            rproxy->m_state = RpWorkerContinue_DONE;
            rp_dispatcher_base_post(RP_DISPATCHER_BASE(rproxy->m_dispatcher), continue_thread_init_cb, arg);
            break;
        }
        case RpWorkerContinue_DONE:
        {
            const rproxy_hooks_t* hooks = tpool_ctx->m_hooks;
            if (hooks && hooks->on_thread_init)
            {
                NOISY_MSG_("calling hooks->on_thread_init(%p, %p)", rproxy, hooks->on_thread_init_arg);
                hooks->on_thread_init(rproxy, hooks->on_thread_init_arg);
            }
            break;
        }
        default:
            break;
    }
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
rproxy_thread_init(evhtp_t* htp, evthr_t* thr, gpointer arg)
{
    LOGD("(%p(%p), %p(%p), %p)", htp, htp->evbase, thr, evthr_get_base(thr), arg);

    g_return_if_fail(htp != NULL);
    g_return_if_fail(thr != NULL);
    g_return_if_fail(arg != NULL);

    g_object_ref(factory_context);
    g_object_ref(default_conn_pool_factory);
    g_object_ref(default_client_connection_factory);
    g_object_ref(default_static_cluster_factory);
    g_object_ref(default_strict_dns_cluster_factory);
    g_object_ref(default_dfp_cluster_factory);
    g_object_ref(default_upstream_transport_socket_config_factory);
    g_object_ref(default_downstream_transport_socket_config_factory);
    g_object_ref(default_network_address_socket_interface_impl_factory);
    g_object_ref(default_cluster_store_factory);
    g_object_ref(default_cluster_provided_lb_factory);
    g_object_ref(default_route_config_provider_manager_factory);

    tpool_ctx_t* tpool_ctx = arg;
    g_atomic_rc_box_acquire(tpool_ctx);

    rproxy_t* parent = tpool_ctx->m_parent;
    server_cfg_t* server_cfg = tpool_ctx->m_server_cfg;
    RpThreadLocalInstance* tls = tpool_ctx->m_tls;

    NOISY_MSG_("server %p(%s)", server_cfg, server_cfg->name);

    rproxy_t* rproxy = rproxy_new(parent, NULL);
    g_assert(rproxy != NULL);
    NOISY_MSG_("rproxy %p", rproxy);

    /* create a pre-accept callback which will disconnect the client
     * immediately if the max-pending request queue is over the configured
     * limit.
     */
    evhtp_set_pre_accept_cb(htp, downstream_pre_accept, rproxy);
    /* Create a post-accept callback which will configure and allocate a newly-
     * accepted connection for processing.
     */
    evhtp_set_post_accept_cb(htp, downstream_post_accept, rproxy);

    /* set aux data argument to this threads specific rproxy_t structure */
    evthr_set_aux(thr, rproxy);

    rproxy->m_tpool_ctx = tpool_ctx;
    rproxy->worker_num = server_cfg_get_worker_num(server_cfg);

    rproxy->server_cfg = server_cfg;
    rproxy->htp        = htp;
    rproxy->m_state    = RpWorkerContinue_CONN_MANAGER;

    g_autofree gchar* name = g_strdup_printf("%s(%u)", server_cfg->name, rproxy->worker_num);

    rproxy->m_dispatcher = create_dispatcher(name, thr);

    rp_thread_local_instance_register_thread(tls, rproxy->m_dispatcher, false);
    g_mutex_lock(&thread_waiter_mutex);
    ++ready_workers;
    g_cond_signal(&thread_ready_cond);

    while (!go_ahead)
    {
        NOISY_MSG_("waiting for go-ahead....");
        g_cond_wait(&thread_go_cond, &thread_waiter_mutex);
    }
    g_mutex_unlock(&thread_waiter_mutex);
    NOISY_MSG_("starting");

    if (server_cfg->default_rule_cfg)
    {
        server_cfg->default_rule = rule_new(server_cfg->default_rule_cfg, NULL);
        map_default_rule_to_upstreams(server_cfg->default_rule, server_cfg->default_rule_cfg, rproxy);
        evhtp_set_gencb(htp, NULL, server_cfg->default_rule_cfg);
    }

    rp_dispatcher_base_post(RP_DISPATCHER_BASE(rproxy->m_dispatcher), continue_thread_init_cb, thr);

} /* rproxy_thread_init */

static evhtp_t* create_htp(evbase_t* evbase, server_cfg_t* server_cfg);

// Wrapper around rproxy_thread_init() to initialize each thread's listener
// (all worker threads listen for, and accept, new connections).
static void
htp_worker_thread_init(evthr_t* thr, gpointer arg)
{
    LOGD("(%p, %p)", thr, arg);

    struct tpool_ctx* tpool_ctx = arg;
    struct server_cfg* server_cfg = tpool_ctx->m_server_cfg;
    struct event_base* evbase = evthr_get_base(thr);
    struct evhtp* htp = create_htp(evbase, server_cfg);
    if (!htp)
    {
        g_error("alloc failed");
        evthr_stop(thr);
        return;
    }

    // Set ENABLE_REUSEPORT so that all worker threads can listen and accept
    // new connections. The O.S. determines which thread will get the socket.
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

#ifndef TAILQ_FOREACH_SAFE
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)        \
    for ((var) = TAILQ_FIRST((head));                     \
         (var) && ((tvar) = TAILQ_NEXT((var), field), 1); \
         (var) = (tvar))
#endif
static void
rproxy_evhtp_free(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);

    evhtp_t* evhtp = arg;
    evhtp_t* evhtp_vhost;
    evhtp_t* tmp;
    TAILQ_FOREACH_SAFE(evhtp_vhost, &evhtp->vhosts, next_vhost, tmp)
    {
        TAILQ_REMOVE(&evhtp->vhosts, evhtp_vhost, next_vhost);
        NOISY_MSG_("freeing htp %p for parent %p", evhtp_vhost, evhtp_vhost->parent);
        g_clear_pointer(&evhtp_vhost, evhtp_free);
    }
    NOISY_MSG_("freeing htp %p", evhtp);
    g_clear_pointer(&evhtp, evhtp_free);
}

static void
rproxy_thread_exit(evhtp_t* htp, evthr_t* thr, gpointer arg)
{
    LOGD("(%p, %p, %p)", htp, thr, arg);

    rproxy_t* rproxy = evthr_get_aux(thr);
    tpool_ctx_t* tpool_ctx = arg;

    const rproxy_hooks_t* hooks = tpool_ctx->m_hooks;
    if (hooks && hooks->on_thread_exit)
    {
        LOGD("calling hooks->on_thread_exit(%p, %p)", rproxy, hooks->on_thread_exit_arg);
        hooks->on_thread_exit(rproxy, hooks->on_thread_exit_arg);
    }

    rp_dispatcher_shutdown(rproxy->m_dispatcher);

    g_clear_object(&rproxy->m_transport_socket_factory);
    g_clear_object(&rproxy->m_filter_config);
    g_clear_object(&rproxy->m_dispatcher);

    server_cfg_t* server_cfg = tpool_ctx->m_server_cfg;
    if (server_cfg->enable_workers_listen)
    {
        evhtp_unbind_socket(htp);
        g_clear_pointer(&htp, rproxy_evhtp_free);
    }

    g_clear_pointer(&rproxy, rproxy_free);
    evthr_set_aux(thr, NULL);

    g_object_unref(factory_context);
    g_object_unref(default_conn_pool_factory);
    g_object_unref(default_client_connection_factory);
    g_object_unref(default_static_cluster_factory);
    g_object_unref(default_strict_dns_cluster_factory);
    g_object_unref(default_dfp_cluster_factory);
    g_object_unref(default_upstream_transport_socket_config_factory);
    g_object_unref(default_downstream_transport_socket_config_factory);
    g_object_unref(default_network_address_socket_interface_impl_factory);
    g_object_unref(default_cluster_store_factory);
    g_object_unref(default_cluster_provided_lb_factory);
    g_object_unref(default_route_config_provider_manager_factory);

    RpThreadLocalInstance* tls_ = tpool_ctx->m_tls;

    g_atomic_rc_box_release(tpool_ctx);

    rp_thread_local_instance_shutdown_thread(tls_);

    g_mutex_lock(&thread_waiter_mutex);
    --ready_workers;
    g_cond_signal(&thread_ready_cond);
    g_mutex_unlock(&thread_waiter_mutex);

} /* rproxy_thread_exit */

static void
htp_worker_thread_exit(evthr_t* thr, gpointer arg)
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
static void
add_callback_rule(gpointer data, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", data, arg);

    evhtp_t* htp = arg;
    g_assert(htp != NULL);

    rule_cfg_t* rule_cfg = data;
    NOISY_MSG_("rule_cfg %p(%s), matchstr %p(%s)", rule_cfg, rule_cfg->name, rule_cfg->matchstr, rule_cfg->matchstr);

    evhtp_callback_t* cb = NULL;
    switch (rule_cfg->type)
    {
        case rule_type_exact:
            NOISY_MSG_("exact");
            cb = evhtp_set_cb(htp, rule_cfg->matchstr, NULL, rule_cfg);
            break;
        case rule_type_regex:
            NOISY_MSG_("regex");
            cb = evhtp_set_regex_cb(htp, rule_cfg->matchstr, NULL, rule_cfg);
            break;
        case rule_type_glob:
            NOISY_MSG_("glob");
            cb = evhtp_set_glob_cb(htp, rule_cfg->matchstr, NULL, rule_cfg);
            break;
        case rule_type_default:
            NOISY_MSG_("default");
        default:
            /* default rules will match anything */
            cb = evhtp_set_glob_cb(htp, "*", NULL, rule_cfg);
            break;
    }

    if (cb == NULL)
    {
        g_error("Could not compile evhtp callback for pattern \"%s\"", rule_cfg->matchstr);
// REVISIT: should not be fatal.(?)
        exit(EXIT_FAILURE);
    }
}

static void
add_vhost_name(gpointer data, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", data, arg);

    evhtp_t* htp_vhost = arg;
    char* name = data;

    NOISY_MSG_("calling evhtp_add_alias(%p, %p(%s))", htp_vhost, name, name);
    evhtp_add_alias(htp_vhost, name);
}

static void
add_evhtp_vhost(gpointer data, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", data, arg);

    /* create a new child evhtp for this vhost */
    evhtp_t* htp = arg;
    evhtp_t* htp_vhost = evhtp_new(htp->evbase, NULL);

    /* disable 100-continue responses, we let the upstreams deal with this.
     */
    evhtp_disable_flag(htp_vhost, EVHTP_FLAG_ENABLE_100_CONT);

    /* for each rule, create a evhtp callback with the defined type */
    vhost_cfg_t* vcfg = data;
    g_slist_foreach(vcfg->rule_cfgs, add_callback_rule, htp_vhost);

    /* add the vhost using the name of the vhost config directive */
    evhtp_add_vhost(htp, vcfg->server_name, htp_vhost);
    NOISY_MSG_("vhost server name %p(%s)", vcfg->server_name, vcfg->server_name);

    /* create an alias for the vhost for each configured alias directive */
    g_slist_foreach(vcfg->aliases, add_vhost_name, htp_vhost);

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

    evhtp_ssl_cfg_t* ssl_cfg = vcfg->ssl_cfg ? vcfg->ssl_cfg : vcfg->server_cfg->ssl_cfg;
    if (ssl_cfg != NULL)
    {
        /* vhost specific ssl configuration found */
        evhtp_ssl_init(htp_vhost, ssl_cfg);

        /* if CRL checking is enabled, create a new ssl_crl_ent_t and add it
         * to the evhtp_t's arguments. XXX: in the future we should create a
         * generic wrapper for various things we want to put in the evhtp
         * arguments, but for now, the only thing that we care about is the
         * CRL context.
         */
        if (ssl_cfg->args)
        {
            ssl_crl_cfg_t* crl_cfg = ssl_cfg->args;

            htp->arg = (void *)ssl_crl_ent_new(/*htp*/htp_vhost, crl_cfg);
            g_assert(htp->arg != NULL);
        }
    }

} /* add_evhtp_vhost */

static void
bind_server_cfg_to_vhost_cfg(gpointer data, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", data, arg);

    vhost_cfg_t* self = data;
    server_cfg_t* server_cfg = arg;

    self->server_cfg = server_cfg;
}

static evhtp_t*
create_htp(evbase_t* evbase, server_cfg_t* server_cfg)
{
    NOISY_MSG_("(%p, %p(%s))", evbase, server_cfg, server_cfg->name);

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
    g_slist_foreach(server_cfg->vhost_cfgs, add_evhtp_vhost, htp);

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
    rproxy_t* parent;
    evbase_t* evbase;
    RpThreadLocalInstance* tls;
    int num_threads;
};

static void
add_server(gpointer data, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", data, arg);

    server_cfg_t* server_cfg = data;

    g_info("Server %s(%s:%d)", server_cfg->name, server_cfg->bind_addr, server_cfg->bind_port);

    add_server_ctx_t* server_ctx = arg;
    rproxy_t* rproxy = server_ctx->parent;
    tpool_ctx_t* tpool_ctx = g_atomic_rc_box_new0(tpool_ctx_t);
    tpool_ctx->m_hooks = server_ctx->hooks;
    tpool_ctx->m_parent = server_ctx->parent;
    tpool_ctx->m_server_cfg = server_cfg;
    tpool_ctx->m_tls = server_ctx->tls;

    rproxy->server_cfg = server_cfg;

    /* Bind the server_cfg to each vhost_cfg in the list.
     */
    g_slist_foreach(server_cfg->vhost_cfgs, bind_server_cfg_to_vhost_cfg, server_cfg);

    if (server_cfg->enable_workers_listen)
    {
        /* use a worker thread pool for connections where each worker listens
         * for new connections.
         */
        evthr_pool_t* workers = evthr_pool_wexit_new(server_cfg->num_threads,
                                                        htp_worker_thread_init,
                                                        htp_worker_thread_exit,
                                                        tpool_ctx);
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
         * instance for each of the threads.
         */
        evhtp_use_threads_wexit(htp,
                                rproxy_thread_init,
                                rproxy_thread_exit,
                                server_cfg->num_threads,
                                tpool_ctx);

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

    server_ctx->num_threads += server_cfg->num_threads;

    /* create a upstream_t instance for each configured upstream */
    g_slist_foreach(server_cfg->upstream_cfgs, add_upstream, rproxy);

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
    g_slist_foreach(server_cfg->vhost_cfgs, add_vhost, rproxy);

    server_cfg->m_arg = tpool_ctx;
}

static void
stop_server(gpointer data, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", data, arg);

    server_cfg_t* server_cfg = data;
    if (server_cfg->m_thr_pool)
    {
        evthr_pool_t* thr_pool = server_cfg->m_thr_pool;
        NOISY_MSG_("stopping pool %p", thr_pool);
        evthr_pool_stop(thr_pool);
        g_clear_pointer(&server_cfg->m_thr_pool, evthr_pool_free);
    }
    else
    {
        evhtp_unbind_socket(server_cfg->m_htp);
        g_clear_pointer(&server_cfg->m_htp, rproxy_evhtp_free);
    }

    if (server_cfg->m_arg)
    {
        NOISY_MSG_("releasing %p", server_cfg->m_arg);
        g_atomic_rc_box_release(server_cfg->m_arg);
        server_cfg->m_arg = NULL;
    }
}

// Main thread: Wait for all workers to initialize, then release barrier
static void
wait_for_all_workers_initialized_and_signal(void)
{
    NOISY_MSG_("()");
    g_mutex_lock(&thread_waiter_mutex);
    while (ready_workers < total_workers)
    {
        NOISY_MSG_("waiting for %d more workers (ready: %d/%d)",
                total_workers - ready_workers, ready_workers, total_workers);
        g_cond_wait(&thread_ready_cond, &thread_waiter_mutex);
    }
    NOISY_MSG_("%d workers initialized, starting! Releasing barrier.", total_workers);
    go_ahead = true;
    g_cond_broadcast(&thread_go_cond);  // Wake ALL waiting workers
    g_mutex_unlock(&thread_waiter_mutex);
}

// Main thread: Wait for all workers to stop.
static void
wait_for_all_workers_stopped(void)
{
    NOISY_MSG_("()");
    g_mutex_lock(&thread_waiter_mutex);
    while (ready_workers)
    {
        NOISY_MSG_("waiting for %d more workers (ready: %d/%d)",
                ready_workers, ready_workers, total_workers);
        g_cond_wait(&thread_ready_cond, &thread_waiter_mutex);
    }
    g_mutex_unlock(&thread_waiter_mutex);
}

static bool
rproxy_init(rproxy_t* self, evbase_t* evbase, const rproxy_hooks_t* hooks, RpServerInstance* server, RpThreadLocalInstance* tls)
{
    LOGD("(%p, %p, %p, %p, %p)", self, evbase, hooks, server, tls);

    g_return_val_if_fail(self != NULL, false);
    g_return_val_if_fail(evbase != NULL, false);
    g_return_val_if_fail(hooks != NULL, false);
    g_return_val_if_fail(RP_IS_THREAD_LOCAL_INSTANCE(tls), false);

    add_server_ctx_t server_ctx = {
        .hooks = hooks,
        .parent = self,
        .evbase = evbase,
        .tls = tls,
        .num_threads = 0
    };

    // Note: it is important that this be called *after* the rproxy_cfg
    //       processing has been done; otherwise, the necessary prep work will
    //       not have been accomplished.
    if (hooks->on_cfg_init && !hooks->on_cfg_init(self, hooks->on_cfg_init_arg))
    {
        g_error("alloc failed");
        return NULL;
    }

    rproxy_cfg_t* cfg = self->config;
    // Iterate over each server_cfg, and set up evhtp stuff.
    g_slist_foreach(cfg->servers, add_server, &server_ctx);

    self->upstreams = g_slist_reverse(self->upstreams);
    self->rules = g_slist_reverse(self->rules);

    total_workers = server_ctx.num_threads;

    g_autoptr(GError) err = NULL;
    if (!rp_cluster_manager_initialize(rp_server_instance_cluster_manager(server), self, &err))
    {
        g_error("error %d(%s)", err->code, err->message);
        return -1;
    }

    wait_for_all_workers_initialized_and_signal();

    NOISY_MSG_("done");
    return true;
} /* rproxy_init */

static void
rproxy_exit(rproxy_t* self)
{
    NOISY_MSG_("(%p)", self);

    // Iterate over each server_cfg, and tear down up evhtp stuff.
    rproxy_cfg_t* rproxy_cfg = self->config;
    g_slist_foreach(rproxy_cfg->servers, stop_server, NULL);
} /* rproxy_exit */

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

static inline RpSingletonManager*
singleton_manager_from_factory_context(RpFactoryContext* context)
{
    return rp_common_factory_context_singleton_manager(RP_COMMON_FACTORY_CONTEXT(
            rp_generic_factory_context_server_factory_context(RP_GENERIC_FACTORY_CONTEXT(context))));
}

static void
create_rp_instances(RpServerInstance* server, RpThreadLocalInstance* tls)
{
    NOISY_MSG_("(%p, %p)", server, tls);
    // Create a single RpFactoryContext instance.
    factory_context = RP_FACTORY_CONTEXT(
                        rp_factory_context_impl_new(RP_SERVER_INSTANCE(server)));
    // Create a global, default RpPerHostGenericConnPoolFactory instance.
    default_conn_pool_factory = rp_per_host_generic_conn_pool_factory_new();
    // Create a global, default RpDefaultClientConnectionFactory instance.
    default_client_connection_factory = rp_default_client_connection_factory_new();
    // Create a global, default RpClusterFactory.
    default_static_cluster_factory = RP_CLUSTER_FACTORY(rp_static_cluster_factory_new());
    // Create a global, default RpStrictDnsClusterFactory.
    default_strict_dns_cluster_factory = RP_CLUSTER_FACTORY(rp_strict_dns_cluster_factory_new());
    // Create a global, default dynamic RpClusterFactory.
    default_dfp_cluster_factory = RP_CLUSTER_FACTORY(rp_dfp_cluster_factory_new());
    // Create a global, default upstream transport socket config factory.
    default_upstream_transport_socket_config_factory = RP_UPSTREAM_TRANSPORT_SOCKET_CONFIG_FACTORY(
                                                        rp_upstream_raw_buffer_socket_factory_new());
    // Create a global, default downstream transport socket config factory.
    default_downstream_transport_socket_config_factory = RP_DOWNSTREAM_TRANSPORT_SOCKET_CONFIG_FACTORY(
                                                            rp_downstream_raw_buffer_socket_factory_new());
    // Create a global, default network address socket interface factory.
    RpSingletonManager* singleton_manager = singleton_manager_from_factory_context(factory_context);
    default_network_address_socket_interface_impl_factory = rp_network_address_socket_interface_impl_factory_new(singleton_manager);
    default_cluster_store_factory = rp_dfp_cluster_store_factory_new(singleton_manager);
    default_cluster_provided_lb_factory = rp_cluster_provided_lb_factory_new();
    default_route_config_provider_manager_factory =
        rp_route_config_provider_manager_factory_new(singleton_manager);
}

static void
clear_rp_instances(void)
{
    NOISY_MSG_("()");
    g_clear_object(&default_dfp_cluster_factory);
    g_clear_object(&default_static_cluster_factory);
    g_clear_object(&default_conn_pool_factory);
    g_clear_object(&default_client_connection_factory);
    g_clear_object(&factory_context);
    g_clear_object(&default_upstream_transport_socket_config_factory);
    g_clear_object(&default_downstream_transport_socket_config_factory);
    g_clear_object(&default_network_address_socket_interface_impl_factory);
    g_clear_object(&default_cluster_store_factory);
    g_clear_object(&default_cluster_provided_lb_factory);
    g_clear_object(&default_route_config_provider_manager_factory);
}

static void
main_thread_init_cb(evthr_t* self, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", self, arg);
}

static void
main_thread_exit_cb(evthr_t* self, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", self, arg);
}

bool
rproxy_add_server_cfg(rproxy_t* rproxy, const char* server_cfg_buf)
{
    LOGD("(%p, %p(%s))", rproxy, server_cfg_buf, server_cfg_buf);

    g_return_val_if_fail(rproxy != NULL, false);
    g_return_val_if_fail(server_cfg_buf != NULL, false);
    g_return_val_if_fail(server_cfg_buf[0] != 0, false);

    rproxy_cfg_t* rproxy_cfg = rproxy->config;

    if (!rproxy_cfg_parse_server_buf(rproxy_cfg, server_cfg_buf))
    {
        LOGE("failed");
        return false;
    }

    IF_NOISY_(server_cfg_t* server_cfg = (g_slist_last(rproxy_cfg->servers))->data;)
    NOISY_MSG_("server cfg %p(%s)", server_cfg, server_cfg->name);

    return true;
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

    g_mutex_init(&thread_waiter_mutex);
    g_cond_init(&thread_ready_cond);
    g_cond_init(&thread_go_cond);

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

    // Create a "main" thread.
    evthr_t* main_thread = evthr_wexit_new(main_thread_init_cb,
                                            main_thread_exit_cb,
                                            rproxy_cfg);
    // This is clunky, but necessary because libevhtp allocates the thread's
    // evbase when started. (It doesn't free it when stopped.)
    evthr_start(main_thread);
    evthr_stop(main_thread);

    evbase_t* evbase = evthr_get_base(main_thread);

    // Create a parent rproxy instance to which all child rproxy instances will
    // bind.
    g_autoptr(rproxy_t) rproxy = rproxy_new(NULL, rproxy_cfg);
    NOISY_MSG_("rproxy %p", rproxy);

    g_autoptr(RpThreadLocalInstance) tls = RP_THREAD_LOCAL_INSTANCE(rp_thread_local_instance_impl_new());
    g_autoptr(RpServerInstance) server = rp_server_instance_impl_new(rproxy, tls, main_thread);

    // This will initialize the primary components, such as ClusterManagerImpl.
    rp_server_instance_base_initialize(RP_SERVER_INSTANCE_BASE(server));

    // Create the rp instance singletons.
    create_rp_instances(server, tls);

    if (!rproxy_init(rproxy, evbase, hooks, server, tls))
    {
        g_error("rproxy_init() failed");
        rproxy_free(rproxy);
        rproxy_cfg_free(rproxy_cfg);
        return -1;
    }

    if (rproxy_cfg->user || rproxy_cfg->group)
    {
        util_dropperms(rproxy_cfg->user, rproxy_cfg->group);
    }

    g_info("Entering main loop");

    // Run the main event loop until it is instructed to exit.
    rp_server_instance_run(server);

    if (hooks->on_sigint) hooks->on_sigint(rproxy, hooks->on_sigint_arg);

    rproxy_exit(rproxy);

    // Clear the rp instance singletons.
    clear_rp_instances();

    evthr_free(main_thread);

    wait_for_all_workers_stopped();

    g_mutex_clear(&thread_waiter_mutex);
    g_cond_clear(&thread_ready_cond);
    g_cond_clear(&thread_go_cond);

    g_clear_object(&server);
    g_clear_object(&tls);

    LOGD("done");
    return 0;
} /* rproxy_main */
