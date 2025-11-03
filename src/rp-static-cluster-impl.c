/*
 * rp-static-cluster.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_static_cluster_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_static_cluster_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <netinet/in.h>
#include "network/rp-transport-socket-factory-impl.h"
#include "tcp/rp-conn-pool.h"
#include "upstream/rp-priority-state-manager.h"
#include "rproxy.h"
#include "rp-static-cluster-impl.h"

typedef struct _RpStaticClusterImplPrivate RpStaticClusterImplPrivate;
struct _RpStaticClusterImplPrivate {

    RpClusterFactoryContext* m_context;

    RpClusterManager* m_parent;
    RpConnectionPoolInstance* m_pool;

    GHashTable* m_host_map;

    guint32 m_overprovisioning_factor;
    bool m_weighted_priority_health;
};

enum
{
    PROP_0, // Reserved.
    PROP_CONTEXT,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void thread_local_cluster_iface_init(RpThreadLocalClusterInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpStaticClusterImpl, rp_static_cluster_impl, RP_TYPE_CLUSTER_IMPL_BASE,
    G_ADD_PRIVATE(RpStaticClusterImpl)
    G_IMPLEMENT_INTERFACE(RP_TYPE_THREAD_LOCAL_CLUSTER, thread_local_cluster_iface_init)
)

#define PRIV(obj) \
    ((RpStaticClusterImplPrivate*)rp_static_cluster_impl_get_instance_private(RP_STATIC_CLUSTER_IMPL(obj)))

static inline GHashTable*
ensure_host_map(RpStaticClusterImpl* self)
{
    NOISY_MSG_("(%p)", self);
    RpStaticClusterImplPrivate* me = PRIV(self);
    if (me->m_host_map)
    {
        NOISY_MSG_("pre-allocated host map %p", me->m_host_map);
        return me->m_host_map;
    }
    me->m_host_map = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_object_unref);
    NOISY_MSG_("allocated host map %p", me->m_host_map);
    return me->m_host_map;
}

static RpHostSelectionResponse
choose_host_i(RpThreadLocalCluster* self, RpLoadBalancerContext* context G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p)", self, context);

    const RpClusterCfg* config = rp_cluster_impl_base_config_(RP_CLUSTER_IMPL_BASE(self));
    upstream_t* upstream = upstream_get(config->rule);
    RpHost* host = g_hash_table_lookup(ensure_host_map(RP_STATIC_CLUSTER_IMPL(self)), upstream);

#if 0
RpNetworkConnection* connection = rp_load_balancer_context_downstream_connection(context);
RpConnectionInfoProvider* provider = rp_network_connection_connection_info_provider(connection);
struct sockaddr* remote_addr = rp_connection_info_provider_remote_address(provider);
char obuf[256];
NOISY_MSG_("remote address %p(%s)", remote_addr, util_format_sockaddr_port(remote_addr, obuf, sizeof(obuf)));
#endif//0


    return rp_host_selection_response_ctor(host, NULL, NULL);
}

static inline evhtp_proto
normalize_protocol(evhtp_proto protocol)
{
    NOISY_MSG_("(%d)", protocol);
    return protocol < EVHTP_PROTO_11 ? EVHTP_PROTO_11 : protocol;
}

static RpHttpConnectionPoolInstancePtr
http_conn_pool_impl(RpStaticClusterImpl* self, RpHost* host, RpResourcePriority_e priority, evhtp_proto downstream_protocol, RpLoadBalancerContext* context G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p, %d, %d, %p)", self, host, priority, downstream_protocol, context);
    RpStaticClusterImplPrivate* me = PRIV(self);
//TODO...upstream_protocols = host->cluster().upstreamHttpProtocol(downstream_protocol);
    evhtp_proto upstream_protocols[] = {normalize_protocol(downstream_protocol), EVHTP_PROTO_INVALID};
    const RpClusterCfg* config = rp_cluster_impl_base_config_(RP_CLUSTER_IMPL_BASE(self));
    return rp_cluster_manager_factory_allocate_conn_pool(RP_CLUSTER_MANAGER_FACTORY(me->m_parent),
                                                                                    config->dispatcher,
                                                                                    host,
                                                                                    priority,
                                                                                    upstream_protocols);
}

static RpTcpConnPoolInstancePtr
prod_cluster_manager_factory_allocate_tcp_conn_pool(RpDispatcher* dispatcher, RpHost* host, RpResourcePriority_e priority)
{
    NOISY_MSG_("(%p, %p, %d)", dispatcher, host, priority);
    return RP_TCP_CONN_POOL_INSTANCE(rp_tcp_conn_pool_impl_new(dispatcher, host, priority));
}

static RpTcpConnPoolInstancePtr
tcp_conn_pool_impl(RpStaticClusterImpl* self, RpHost* host, RpResourcePriority_e priority, RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %p, %d, %p)", self, host, priority, context);
    const RpClusterCfg* config = rp_cluster_impl_base_config_(RP_CLUSTER_IMPL_BASE(self));
    return prod_cluster_manager_factory_allocate_tcp_conn_pool(config->dispatcher,
                                                                host,
                                                                priority);
}

static void
on_new_stream(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
}

static RpHttpPoolData*
http_conn_pool_i(RpThreadLocalCluster* self, RpHost* host, RpResourcePriority_e priority, evhtp_proto downstream_protocol, RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %p, %d, %d, %p)",
        self, host, priority, downstream_protocol, context);
    RpStaticClusterImplPrivate* me = PRIV(self);
    RpHttpConnectionPoolInstancePtr pool = http_conn_pool_impl(RP_STATIC_CLUSTER_IMPL(self),
                                                                host,
                                                                priority,
                                                                downstream_protocol,
                                                                context);
    me->m_pool = RP_CONNECTION_POOL_INSTANCE(pool);
    return rp_http_pool_data_new(on_new_stream, self, pool);
}

static void
on_new_connection(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
}

static RpTcpPoolData*
tcp_conn_pool_i(RpThreadLocalCluster* self, RpHost* host, RpResourcePriority_e priority, RpLoadBalancerContext* context)
{
    NOISY_MSG_("(%p, %p, %d, %p)", self, host, priority, context);
    RpStaticClusterImplPrivate* me = PRIV(self);
    RpTcpConnPoolInstancePtr pool = tcp_conn_pool_impl(RP_STATIC_CLUSTER_IMPL(self), host, priority, context);
    me->m_pool = RP_CONNECTION_POOL_INSTANCE(pool);
    return rp_tcp_pool_data_new(on_new_connection, self, RP_TCP_CONN_POOL_INSTANCE(me->m_pool));
}

static RpClusterInfo*
info_i(RpThreadLocalCluster* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_cluster_info(RP_CLUSTER(self));
}

static void
thread_local_cluster_iface_init(RpThreadLocalClusterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->choose_host = choose_host_i;
    iface->http_conn_pool = http_conn_pool_i;
    iface->tcp_conn_pool = tcp_conn_pool_i;
    iface->info = info_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_CONTEXT:
            g_value_set_object(value, PRIV(obj)->m_context);
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
        case PROP_CONTEXT:
            PRIV(obj)->m_context = g_value_get_object(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

struct sockaddr_storage
sockaddr_from_cfg(upstream_cfg_t* cfg)
{
    struct sockaddr_storage sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.ss_family = AF_UNSPEC;
    if (cfg->host)
    {
        g_autoptr(GInetAddress) address = g_inet_address_new_from_string(cfg->host);
        switch (g_inet_address_get_family(address))
        {
            case G_SOCKET_FAMILY_IPV4:
            {
                struct sockaddr_in* sin = (struct sockaddr_in*)&sockaddr;
                sin->sin_family = AF_INET;
                sin->sin_port = htons(cfg->port);
                memcpy(&sin->sin_addr, g_inet_address_to_bytes(address), g_inet_address_get_native_size(address));
                break;
            }
            case G_SOCKET_FAMILY_IPV6:
            {
                struct sockaddr_in6* sin6 = (struct sockaddr_in6*)&sockaddr;
                sin6->sin6_family = AF_INET6;
                sin6->sin6_port = htons(cfg->port);
                memcpy(&sin6->sin6_addr, g_inet_address_to_bytes(address), g_inet_address_get_native_size(address));
                break;
            }
            default:
                break;
        }
    }
    return sockaddr;
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_static_cluster_impl_parent_class)->constructed(obj);

    RpStaticClusterImpl* self = RP_STATIC_CLUSTER_IMPL(obj);
    GHashTable* host_map = ensure_host_map(self);
    RpStaticClusterImplPrivate* me = PRIV(self);
#if 0
    RpServerFactoryContext* server_context = rp_cluster_factory_context_server_factory_context(me->m_context);
    RpLocalInfo* local_info = rp_common_factory_context_local_info(RP_COMMON_FACTORY_CONTEXT(server_context));
#endif//0
    RpClusterInfo* cluster_info = rp_cluster_info(RP_CLUSTER(self));
    const RpClusterCfg* config = rp_cluster_impl_base_config_(RP_CLUSTER_IMPL_BASE(obj));
    lztq* lb_endpoints = config->lb_endpoints;
    for (lztq_elem* elem = lztq_first(lb_endpoints); elem; elem = lztq_next(elem))
    {
        upstream_t* upstream = lztq_elem_data(elem);
        upstream_cfg_t* cfg = upstream->config;
        RpTransportSocketFactoryImpl* socket_factory = rp_transport_socket_factory_impl_new(upstream);
        struct sockaddr_storage sockaddr = sockaddr_from_cfg(cfg);
        const char* hostname = cfg->name; // REVISIT??
        RpHostImpl* host = rp_host_impl_new(cluster_info,
                                            RP_UPSTREAM_TRANSPORT_SOCKET_FACTORY(socket_factory),
                                            hostname,
                                            (struct sockaddr*)&sockaddr,
                                            1/*initial_weight*/,
                                            true/*disable_health_check*/);

        g_hash_table_insert(host_map, upstream, host);
    }

    me->m_parent = rp_cluster_factory_context_cluster_manager(me->m_context);
    me->m_context = NULL; // Avoid dangling pointer.
}

OVERRIDE void
dispose(GObject* obj)
{
    LOGD("(%p)", obj);

    RpStaticClusterImplPrivate* me = PRIV(obj);
    g_clear_pointer(&me->m_host_map, g_hash_table_unref);

    G_OBJECT_CLASS(rp_static_cluster_impl_parent_class)->dispose(obj);
}

static RpInitializationPhase_e
initialize_phase_i(RpCluster* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return RpInitializationPhase_Primary;
}

static void
cluster_iface_init(RpClusterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->initialize_phase = initialize_phase_i;
}

OVERRIDE void
start_pre_init(RpClusterImplBase* self)
{
    NOISY_MSG_("(%p)", self);

    rp_cluster_impl_base_on_pre_init_complete(self);
}

static void
cluster_impl_base_class_init(RpClusterImplBaseClass* klass)
{
    LOGD("(%p)", klass);
    cluster_iface_init(g_type_interface_peek(klass, RP_TYPE_CLUSTER));
    klass->start_pre_init = start_pre_init;
}

static void
rp_static_cluster_impl_class_init(RpStaticClusterImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    cluster_impl_base_class_init(RP_CLUSTER_IMPL_BASE_CLASS(klass));

    obj_properties[PROP_CONTEXT] = g_param_spec_object("context",
                                                    "Context",
                                                    "ClusterFactoryContext Instance",
                                                    RP_TYPE_CLUSTER_FACTORY_CONTEXT,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_static_cluster_impl_init(RpStaticClusterImpl* self G_GNUC_UNUSED)
{
    LOGD("(%p)", self);
}

RpStaticClusterImpl*
rp_static_cluster_impl_new(RpClusterCfg* config, RpClusterFactoryContext* context, RpStatusCode_e* creation_status)
{
    LOGD("(%p, %p, %p)", config, context, creation_status);
    if (creation_status) *creation_status = RpStatusCode_Ok;
    return g_object_new(RP_TYPE_STATIC_CLUSTER_IMPL,
                        "config", config,
                        "context", context,
                        "cluster-context", context,
                        NULL);
}

void
rp_static_cluster_impl_add_downstream(RpStaticClusterImpl* self, upstream_t* upstream, RpHost* host)
{
    LOGD("(%p, %p, %p)", self, upstream, host);
    g_return_if_fail(RP_STATIC_CLUSTER_IMPL(self));
    g_hash_table_insert(ensure_host_map(self), upstream, host);
}
