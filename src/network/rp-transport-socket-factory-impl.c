/*
 * rp-transport-socket-factory-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_transport_socket_factory_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_transport_socket_factory_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "network/rp-raw-buffer-socket.h"
#include "network/rp-transport-socket-factory-impl.h"

struct _RpTransportSocketFactoryImpl {
    GObject parent_instance;

    SHARED_PTR(downstream_t) m_downstream;
};

static void transport_socket_factory_base_iface_init(RpTransportSocketFactoryBaseInterface* iface);
static void upstream_transport_socket_factory_iface_init(RpUpstreamTransportSocketFactoryInterface* iface);
static void downstream_transport_socket_factory_iface_init(RpDownstreamTransportSocketFactoryInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpTransportSocketFactoryImpl, rp_transport_socket_factory_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_TRANSPORT_SOCKET_FACTORY_BASE, transport_socket_factory_base_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_UPSTREAM_TRANSPORT_SOCKET_FACTORY, upstream_transport_socket_factory_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_DOWNSTREAM_TRANSPORT_SOCKET_FACTORY, downstream_transport_socket_factory_iface_init)
)

static bool
implements_secure_transport_i(RpTransportSocketFactoryBase* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_TRANSPORT_SOCKET_FACTORY_IMPL(self)->m_downstream->ssl_ctx != NULL;
}

static inline void
transport_socket_factory_base_iface_init(RpTransportSocketFactoryBaseInterface* iface)
{
    LOGD("(%p)", iface);
    iface->implements_secure_transport = implements_secure_transport_i;
}

static RpNetworkTransportSocket*
create_transport_socket_i(RpUpstreamTransportSocketFactory* self, RpHostDescription* host)
{
    NOISY_MSG_("(%p, %p)", self, host);

    RpTransportSocketFactoryImpl* me = RP_TRANSPORT_SOCKET_FACTORY_IMPL(self);
    downstream_t* downstream = me->m_downstream;

rproxy_t* rproxy = downstream->rproxy;
NOISY_MSG_("downstream %p, rproxy %p", downstream, rproxy);

evbase_t* evbase = evthr_get_base(rproxy->thr);
NOISY_MSG_("evbase %p, %p, thr %p", evbase, downstream->evbase, rproxy->thr);

    SSL* ssl;
    evbev_t* bev;
    if (downstream->ssl_ctx)
    {
        ssl = SSL_new(downstream->ssl_ctx);
        char* sni = downstream->config->ssl_cfg->sni;
        if (sni && sni[0])
        {
            LOGD("setting sni \"%s\"", sni);
            SSL_set_tlsext_host_name(ssl, sni);
        }
        bev = bufferevent_openssl_socket_new(evbase, -1, ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_CLOSE_ON_FREE);
        SSL_set_app_data(ssl, bev);
    }
    else
    {
        bev = bufferevent_socket_new(evbase, -1, BEV_OPT_CLOSE_ON_FREE);
        ssl = NULL;
    }

    return RP_NETWORK_TRANSPORT_SOCKET(rp_raw_buffer_socket_new(RpHandleType_Connecting, bev, ssl, rproxy->dns_base));
}

static void
upstream_transport_socket_factory_iface_init(RpUpstreamTransportSocketFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_transport_socket = create_transport_socket_i;
}

static RpNetworkTransportSocket*
create_downstream_transport_socket_i(RpDownstreamTransportSocketFactory* self, evhtp_connection_t* conn)
{
    NOISY_MSG_("(%p, %p)", self, conn);
    evbev_t* bev = evhtp_connection_take_ownership(conn);
    return RP_NETWORK_TRANSPORT_SOCKET(rp_raw_buffer_socket_new(RpHandleType_Accepting, bev, conn->ssl, NULL));
}

static void
downstream_transport_socket_factory_iface_init(RpDownstreamTransportSocketFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_downstream_transport_socket = create_downstream_transport_socket_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_transport_socket_factory_impl_parent_class)->dispose(obj);
}

static void
rp_transport_socket_factory_impl_class_init(RpTransportSocketFactoryImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_transport_socket_factory_impl_init(RpTransportSocketFactoryImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpTransportSocketFactoryImpl*
rp_transport_socket_factory_impl_new(SHARED_PTR(downstream_t) downstream)
{
    LOGD("(%p)", downstream);
    RpTransportSocketFactoryImpl* self = g_object_new(RP_TYPE_TRANSPORT_SOCKET_FACTORY_IMPL, NULL);
    self->m_downstream = downstream;
    return self;
}
