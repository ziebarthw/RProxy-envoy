/*
 * rp-prod-cluster-manager-factory.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_prod_cluster_manager_factory_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_prod_cluster_manager_factory_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "http1/rp-http1-conn-pool.h"
#include "tcp/rp-conn-pool.h"
#include "upstream/rp-cluster-factory-impl.h"
#include "upstream/rp-cluster-manager-impl.h"

struct _RpProdClusterManagerFactory {
    GObject parent_instance;

    RpServerFactoryContext* m_context;
    RpServerInstance* m_server;
    RpThreadLocalInstance* m_tls;
};

static void cluster_manager_factory_iface_init(RpClusterManagerFactoryInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpProdClusterManagerFactory, rp_prod_cluster_manager_factory, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_CLUSTER_MANAGER_FACTORY, cluster_manager_factory_iface_init)
)

static RpHttpConnectionPoolInstancePtr
allocate_conn_pool_i(RpClusterManagerFactory* self, RpDispatcher* dispatcher, RpHost* host, RpResourcePriority_e priority, evhtp_proto* protocols)
{
    NOISY_MSG_("(%p, %p, %p, %d, %p)", self, dispatcher, host, priority, protocols);

    //TODO...more complex selection logic; just http1 for now...
    if (protocols[0] != EVHTP_PROTO_11)
    {
        LOGE("protocol %d not supported!", protocols[0]);
        return NULL;
    }
    return http1_allocate_conn_pool(dispatcher, host, priority);
}

static RpTcpConnPoolInstancePtr
allocate_tcp_conn_pool_i(RpClusterManagerFactory* self, RpDispatcher* dispatcher, RpHost* host, RpResourcePriority_e priority)
{
    NOISY_MSG_("(%p, %p, %p, %d)", self, dispatcher, host, priority);
    return RP_TCP_CONN_POOL_INSTANCE(rp_tcp_conn_pool_impl_new(dispatcher, host, priority));
}

static PairClusterSharedPtrThreadAwareLoadBalancerPtr
cluster_from_proto_i(RpClusterManagerFactory* self, const RpClusterCfg* cluster, RpClusterManager* cm, bool added_via_api)
{
    NOISY_MSG_("(%p, %p, %p, %u)", self, cluster, cm, added_via_api);
    RpProdClusterManagerFactory* me = RP_PROD_CLUSTER_MANAGER_FACTORY(self);
    return rp_cluster_factory_impl_base_create(cluster, me->m_context, cm, added_via_api);
}

static RpClusterManagerPtr
cluster_manager_from_proto_i(RpClusterManagerFactory* self, rproxy_t* bootstrap)
{
    NOISY_MSG_("(%p, %p)", self, bootstrap);

    RpProdClusterManagerFactory* me = RP_PROD_CLUSTER_MANAGER_FACTORY(self);
    RpCommonFactoryContext* context = RP_COMMON_FACTORY_CONTEXT(me->m_context);
    RpDispatcher* dispatcher = rp_common_factory_context_main_thread_dispatcher(context);
    RpStatusCode_e creation_status = RpStatusCode_Ok;
    g_autoptr(RpClusterManagerImpl) cluster_manager_impl = rp_cluster_manager_impl_new(bootstrap,
                                                                                        self,
                                                                                        context,
                                                                                        me->m_tls,
                                                                                        dispatcher,
                                                                                        me->m_server,
                                                                                        &creation_status);
    if (creation_status != RpStatusCode_Ok)
    {
        LOGE("failed");
        return NULL;
    }
    return RP_CLUSTER_MANAGER(g_steal_pointer(&cluster_manager_impl));
}

static void
cluster_manager_factory_iface_init(RpClusterManagerFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->cluster_manager_from_proto = cluster_manager_from_proto_i;
    iface->allocate_conn_pool = allocate_conn_pool_i;
    iface->allocate_tcp_conn_pool = allocate_tcp_conn_pool_i;
    iface->cluster_from_proto = cluster_from_proto_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_prod_cluster_manager_factory_parent_class)->dispose(obj);
}

static void
rp_prod_cluster_manager_factory_class_init(RpProdClusterManagerFactoryClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_prod_cluster_manager_factory_init(RpProdClusterManagerFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpProdClusterManagerFactory*
rp_prod_cluster_manager_factory_new(RpServerFactoryContext* context, RpThreadLocalInstance* tls, RpServerInstance* server)
{
    LOGD("(%p, %p, %p)", context, tls, server);

    g_return_val_if_fail(RP_IS_SERVER_FACTORY_CONTEXT(context), NULL);
    g_return_val_if_fail(RP_IS_THREAD_LOCAL_INSTANCE(tls), NULL);
    g_return_val_if_fail(RP_IS_SERVER_INSTANCE(server), NULL);

    RpProdClusterManagerFactory* self = g_object_new(RP_TYPE_PROD_CLUSTER_MANAGER_FACTORY, NULL);
    self->m_context = context;
    self->m_server = server;
    self->m_tls = tls;
    return self;
}
