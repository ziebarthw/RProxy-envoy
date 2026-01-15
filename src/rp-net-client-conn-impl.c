/*
 * rp-net-client-conn-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_net_client_conn_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_net_client_conn_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "network/rp-client-socket-impl.h"
#include "stream_info/rp-stream-info-impl.h"
#include "stream_info/rp-upstream-info-impl.h"
#include "rp-net-client-conn-impl.h"

struct _RpNetworkClientConnectionImpl {
    RpNetworkConnectionImpl parent_instance;

    RpStreamInfoImpl* m_stream_info;
    RpNetworkAddressInstanceConstSharedPtr m_source_address;
};

enum
{
    PROP_0, // Reserved.
    PROP_SOURCE_ADDRESS,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void network_client_connection_iface_init(RpNetworkClientConnectionInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpNetworkClientConnectionImpl, rp_network_client_connection_impl, RP_TYPE_NETWORK_CONNECTION_IMPL,
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_CLIENT_CONNECTION, network_client_connection_iface_init)
)

static void
connect_i(RpNetworkClientConnection* self)
{
#if 0
    int err = errno;
#endif//0

    NOISY_MSG_("(%p)", self);
    RpNetworkClientConnectionImpl* me = RP_NETWORK_CLIENT_CONNECTION_IMPL(self);
    RpNetworkTransportSocket* transport_socket_ = rp_network_connection_impl_transport_socket_(RP_NETWORK_CONNECTION_IMPL(self));
    RpConnectionSocket* socket_ = rp_network_connection_impl_socket_(RP_NETWORK_CONNECTION_IMPL(self));
    int result = rp_network_transport_socket_connect(transport_socket_, socket_);

    RpStreamInfo* stream_info = RP_STREAM_INFO(me->m_stream_info);
    rp_upstream_timing_on_upstream_connect_start(
        rp_upstream_info_upstream_timing(
            rp_stream_info_upstream_info(stream_info)));

    if (result == 0)
    {
        NOISY_MSG_("write will become ready");
        return;
    }

    //TODO...check error conditions...
}

static void
network_client_connection_iface_init(RpNetworkClientConnectionInterface* iface)
{
    LOGD("(%p)", iface);
    iface->connect = connect_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_SOURCE_ADDRESS:
            g_value_set_object(value, RP_NETWORK_CLIENT_CONNECTION_IMPL(obj)->m_source_address);
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
        case PROP_SOURCE_ADDRESS:
        {
            RpNetworkAddressInstanceConstSharedPtr addr = g_value_get_object(value);
            if (addr)
            {
                RP_NETWORK_CLIENT_CONNECTION_IMPL(obj)->m_source_address = g_object_ref(addr);
            }
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_network_client_connection_impl_parent_class)->constructed(obj);

    RpNetworkClientConnectionImpl* self = RP_NETWORK_CLIENT_CONNECTION_IMPL(obj);
    RpSocket* socket_ = RP_SOCKET(rp_network_connection_impl_socket_(RP_NETWORK_CONNECTION_IMPL(self)));
    RpConnectionInfoProvider* provider = RP_CONNECTION_INFO_PROVIDER(rp_socket_connection_info_provider(socket_));
    self->m_stream_info = rp_stream_info_impl_new(EVHTP_PROTO_INVALID, provider, RpFilterStateLifeSpan_Connection, NULL);

    rp_stream_info_set_upstream_info(RP_STREAM_INFO(self->m_stream_info),
                                        RP_UPSTREAM_INFO(rp_upstream_info_impl_new()));

    RpNetworkAddressInstanceConstSharedPtr source = self->m_source_address;

    RpNetworkAddressInstanceConstSharedPtr local_address = rp_connection_info_provider_local_address(provider);
    if (local_address)
    {
        NOISY_MSG_("setting to local");
        source = local_address;
    }

    if (source)
    {
        //TODO...bind(...)
    }
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpNetworkClientConnectionImpl* self = RP_NETWORK_CLIENT_CONNECTION_IMPL(obj);
    g_clear_object(&self->m_source_address);

    G_OBJECT_CLASS(rp_network_client_connection_impl_parent_class)->dispose(obj);
}

#define NETWORK_CONNECTION(s) RP_NETWORK_CONNECTION(s)
#define SOCKFD(s) rp_network_connection_sockfd(NETWORK_CONNECTION(s))

OVERRIDE void
on_connected(RpNetworkConnectionImpl* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
    RpNetworkClientConnectionImpl* me = RP_NETWORK_CLIENT_CONNECTION_IMPL(self);
    RpStreamInfo* stream_info_ = RP_STREAM_INFO(me->m_stream_info);
    rp_upstream_timing_on_upstream_connect_complete(
        rp_upstream_info_upstream_timing(
            rp_stream_info_upstream_info(stream_info_)));
//TODO...???
NOISY_MSG_("calling parent on_connection(%p) on fd %d", self, SOCKFD(self));
RP_NETWORK_CONNECTION_IMPL_CLASS(rp_network_client_connection_impl_parent_class)->on_connected(self);
NOISY_MSG_("done on fd %d", SOCKFD(self));
}

static void
network_connection_impl_class_init(RpNetworkConnectionImplClass* klass)
{
    LOGD("(%p)", klass);
    klass->on_connected = on_connected;
}

static void
rp_network_client_connection_impl_class_init(RpNetworkClientConnectionImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = set_property;
    object_class->get_property = get_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    network_connection_impl_class_init(RP_NETWORK_CONNECTION_IMPL_CLASS(klass));

    obj_properties[PROP_SOURCE_ADDRESS] = g_param_spec_object("source-address",
                                                    "Source address",
                                                    "Source Address",
                                                    RP_TYPE_NETWORK_ADDRESS_INSTANCE,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_network_client_connection_impl_init(RpNetworkClientConnectionImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpNetworkClientConnectionImpl*
client_connection_impl_new(RpDispatcher* dispatcher, RpConnectionSocket* socket, RpNetworkAddressInstanceConstSharedPtr source_address, RpNetworkTransportSocket* transport_socket)
{
    LOGD("(%p, %p, %p, %p)", dispatcher, socket, source_address, transport_socket);
    RpNetworkClientConnectionImpl* self = g_object_new(RP_TYPE_NETWORK_CLIENT_CONNECTION_IMPL,
                                                        "dispatcher", dispatcher,
                                                        "socket", socket,
                                                        "source-address", source_address,
                                                        "transport-socket", transport_socket,
                                                        NULL);
    g_object_set(G_OBJECT(self), "stream-info", self->m_stream_info, NULL);
    return self;
}

static inline RpConnectionSocket*
create_connection_socket(RpNetworkTransportSocket* transport_socket, RpNetworkAddressInstanceConstSharedPtr addr)
{
    NOISY_MSG_("(%p, %p)", transport_socket, addr);
    RpIoHandle* io_handle = rp_network_transport_socket_create_io_handle(transport_socket);
    RpClientSocketImpl* connection_socket = rp_client_socket_impl_new(io_handle, addr);
    return RP_CONNECTION_SOCKET(connection_socket);
}

RpNetworkClientConnectionImpl*
rp_network_client_connection_impl_new(RpDispatcher* dispatcher, RpNetworkAddressInstanceConstSharedPtr remote_address,
                                        RpNetworkAddressInstanceConstSharedPtr source_address,
                                        RpNetworkTransportSocket* transport_socket)
{
    LOGD("(%p, %p, %p, %p)", dispatcher, remote_address, source_address, transport_socket);
    g_return_val_if_fail(RP_IS_DISPATCHER(dispatcher), NULL);
    g_return_val_if_fail(RP_IS_NETWORK_ADDRESS_INSTANCE(remote_address), NULL);
    g_return_val_if_fail(RP_IS_NETWORK_TRANSPORT_SOCKET(transport_socket), NULL);
    RpConnectionSocket* connection_socket = create_connection_socket(transport_socket, remote_address);
    return client_connection_impl_new(dispatcher,
                                        connection_socket,
                                        source_address,
                                        transport_socket);
}
