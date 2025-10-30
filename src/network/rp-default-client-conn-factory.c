/*
 * rp-default-client-conn-factory.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_default_client_conn_factory_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_default_client_conn_factory_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include "rp-net-client-conn-impl.h"
#include "network/rp-default-client-conn-factory.h"

struct _RpDefaultClientConnectionFactory {
    GObject parent_instance;
};

static void client_connection_factory_iface_init(RpClientConnectionFactoryInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpDefaultClientConnectionFactory, rp_default_client_connection_factory, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_CLIENT_CONNECTION_FACTORY, client_connection_factory_iface_init)
)

static RpNetworkClientConnection*
create_client_connection_i(RpClientConnectionFactory* self G_GNUC_UNUSED, RpDispatcher* dispatcher,
                            struct sockaddr* address, struct sockaddr* source_address, RpNetworkTransportSocket* transport_socket)
{
    NOISY_MSG_("(%p, %p, %p, %p, %p)",
        self, dispatcher, address, source_address, transport_socket);
    return RP_NETWORK_CLIENT_CONNECTION(
        rp_network_client_connection_impl_new(dispatcher, address, source_address, transport_socket));
}

static void
client_connection_factory_iface_init(RpClientConnectionFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_client_connection = create_client_connection_i;
}

OVERRIDE void
dispose(GObject* object)
{
    NOISY_MSG_("(%p)", object);
    G_OBJECT_CLASS(rp_default_client_connection_factory_parent_class)->dispose(object);
}

static void
rp_default_client_connection_factory_class_init(RpDefaultClientConnectionFactoryClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_default_client_connection_factory_init(RpDefaultClientConnectionFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpDefaultClientConnectionFactory*
rp_default_client_connection_factory_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_DEFAULT_CLIENT_CONNECTION_FACTORY, NULL);
}
