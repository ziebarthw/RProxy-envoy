/*
 * rp-raw-buffer-socket-factory.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_raw_buffer_socket_factory_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_raw_buffer_socket_factory_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "thread_local/rp-thread-local-impl.h"
#include "network/rp-raw-buffer-socket.h"

struct _RpRawBufferSocketFactory {
    GObject parent_instance;

    SHARED_PTR(upstream_t) m_upstream;
    SHARED_PTR(server_cfg_t) m_server_cfg;
};

static void transport_socket_factory_base_iface_init(RpTransportSocketFactoryBaseInterface* iface);
static void upstream_transport_socket_factory_iface_init(RpUpstreamTransportSocketFactoryInterface* iface);
static void downstream_transport_socket_factory_iface_init(RpDownstreamTransportSocketFactoryInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpRawBufferSocketFactory, rp_raw_buffer_socket_factory, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_TRANSPORT_SOCKET_FACTORY_BASE, transport_socket_factory_base_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_UPSTREAM_TRANSPORT_SOCKET_FACTORY, upstream_transport_socket_factory_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_DOWNSTREAM_TRANSPORT_SOCKET_FACTORY, downstream_transport_socket_factory_iface_init)
)

static bool
implements_secure_transport_i(RpTransportSocketFactoryBase* self)
{
    NOISY_MSG_("(%p)", self);
    RpRawBufferSocketFactory* me = RP_RAW_BUFFER_SOCKET_FACTORY(self);
    return me->m_server_cfg ?
        me->m_server_cfg->ssl_cfg != NULL : me->m_upstream->ssl_ctx != NULL;
}

static void
transport_socket_factory_base_iface_init(RpTransportSocketFactoryBaseInterface* iface)
{
    LOGD("(%p)", iface);
    iface->implements_secure_transport = implements_secure_transport_i;
}

static RpNetworkTransportSocketPtr
create_transport_socket_i(RpUpstreamTransportSocketFactory* self, RpHostDescription* host)
{
    NOISY_MSG_("(%p, %p)", self, host);

    RpRawBufferSocketFactory* me = RP_RAW_BUFFER_SOCKET_FACTORY(self);
    RpDispatcher* dispatcher = rp_thread_local_instance_impl_get_dispatcher();
    evbase_t* evbase = rp_dispatcher_base(dispatcher);
    evdns_base_t* dns_base = rp_dispatcher_dns_base(dispatcher);
    upstream_t* upstream = me->m_upstream;

NOISY_MSG_("upstream %p(%s)", upstream, upstream->config->name);

    SSL* ssl = NULL;
    evbev_t* bev;
    if (upstream->ssl_ctx)
    {
        ssl = SSL_new(upstream->ssl_ctx);
        char* sni = upstream->config->ssl_cfg->sni;
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
    }

    return RP_NETWORK_TRANSPORT_SOCKET(
            rp_raw_buffer_socket_new(RpHandleType_Connecting, bev, ssl, dns_base));
}

static void
upstream_transport_socket_factory_iface_init(RpUpstreamTransportSocketFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_transport_socket = create_transport_socket_i;
}

static RpNetworkTransportSocketPtr
create_downstream_transport_socket_i(RpDownstreamTransportSocketFactory* self, evhtp_connection_t* conn)
{
    NOISY_MSG_("(%p, %p)", self, conn);
    evbev_t* bev = evhtp_connection_take_ownership(conn);
    return RP_NETWORK_TRANSPORT_SOCKET(
            rp_raw_buffer_socket_new(RpHandleType_Accepting, bev, conn->ssl, NULL));
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
    G_OBJECT_CLASS(rp_raw_buffer_socket_factory_parent_class)->dispose(obj);
}

static void
rp_raw_buffer_socket_factory_class_init(RpRawBufferSocketFactoryClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_raw_buffer_socket_factory_init(RpRawBufferSocketFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpRawBufferSocketFactory*
rp_raw_buffer_socket_factory_create_upstream_factory(upstream_t* upstream)
{
    LOGD("(%p)", upstream);
    RpRawBufferSocketFactory* self = g_object_new(RP_TYPE_RAW_BUFFER_SOCKET_FACTORY, NULL);
    self->m_upstream = upstream;
    return self;
}

RpRawBufferSocketFactory*
rp_raw_buffer_socket_factory_create_downstream_factory(server_cfg_t* server_cfg)
{
    LOGD("(%p)", server_cfg);
    RpRawBufferSocketFactory* self = g_object_new(RP_TYPE_RAW_BUFFER_SOCKET_FACTORY, NULL);
    self->m_server_cfg = server_cfg;
    return self;
}
