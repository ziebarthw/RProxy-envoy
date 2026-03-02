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

#include "rp-upstream.h"
#include "thread_local/rp-thread-local-impl.h"
#include "network/rp-raw-buffer-socket.h"

struct _RpRawBufferSocketFactory {
    GObject parent_instance;

    const upstream_cfg_t* m_upstream_cfg;
    const server_cfg_t* m_server_cfg;
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
    if (me->m_server_cfg)
    {
        NOISY_MSG_("server cfg");
        return me->m_server_cfg != NULL;
    }
    if (me->m_upstream_cfg)
    {
        NOISY_MSG_("upstream cfg");
        return me->m_upstream_cfg->ssl_cfg != NULL;
    }
    return false;
}

static void
transport_socket_factory_base_iface_init(RpTransportSocketFactoryBaseInterface* iface)
{
    LOGD("(%p)", iface);
    iface->implements_secure_transport = implements_secure_transport_i;
}

static RpNetworkTransportSocketPtr
create_transport_socket_i(RpUpstreamTransportSocketFactory* self, RpDispatcher* dispatcher, RpHostDescriptionConstSharedPtr host)
{
    NOISY_MSG_("(%p, %p, %p)", self, dispatcher, host);

    RpRawBufferSocketFactory* me = RP_RAW_BUFFER_SOCKET_FACTORY(self);
    const upstream_cfg_t* upstream_cfg = me->m_upstream_cfg;
    evbase_t* evbase = rp_dispatcher_base(dispatcher);
    upstream_t* upstream = rp_host_description_metadata(host);

    SSL* ssl = NULL;
    evbev_t* bev;
    if (upstream_cfg->ssl_cfg)
    {
        ssl = SSL_new(upstream->ssl_ctx);
        char* sni = upstream_cfg->ssl_cfg->sni;
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
            rp_raw_buffer_socket_new(RpHandleType_Connecting, bev, ssl));
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
            rp_raw_buffer_socket_new(RpHandleType_Accepting, bev, conn->ssl));
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
rp_raw_buffer_socket_factory_create_upstream_factory(const upstream_cfg_t* upstream_cfg)
{
    LOGD("(%p)", upstream_cfg);
    RpRawBufferSocketFactory* self = g_object_new(RP_TYPE_RAW_BUFFER_SOCKET_FACTORY, NULL);
    self->m_upstream_cfg = upstream_cfg;
    return self;
}

RpRawBufferSocketFactory*
rp_raw_buffer_socket_factory_create_downstream_factory(const server_cfg_t* server_cfg)
{
    LOGD("(%p)", server_cfg);
    RpRawBufferSocketFactory* self = g_object_new(RP_TYPE_RAW_BUFFER_SOCKET_FACTORY, NULL);
    self->m_server_cfg = server_cfg;
    return self;
}
