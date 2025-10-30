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

    downstream_t* m_downstream;
};

enum
{
    PROP_0, // Reserved.
    PROP_DOWNSTREAM,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

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
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_DOWNSTREAM:
            g_value_set_pointer(value, RP_TRANSPORT_SOCKET_FACTORY_IMPL(obj)->m_downstream);
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
        case PROP_DOWNSTREAM:
            RP_TRANSPORT_SOCKET_FACTORY_IMPL(obj)->m_downstream = g_value_get_pointer(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
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
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_DOWNSTREAM] = g_param_spec_pointer("downstream",
                                                    "Downstream",
                                                    "RProxy Downstream Instance",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_transport_socket_factory_impl_init(RpTransportSocketFactoryImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpTransportSocketFactoryImpl*
rp_transport_socket_factory_impl_new(downstream_t* downstream)
{
    LOGD("(%p)", downstream);
if (downstream)
{
    NOISY_MSG_("downstream %p, rproxy %p", downstream, downstream->rproxy);
}
    return g_object_new(RP_TYPE_TRANSPORT_SOCKET_FACTORY_IMPL,
                        "downstream", downstream,
                        NULL);
}
