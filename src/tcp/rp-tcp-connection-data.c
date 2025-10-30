/*
 * rp-tcp-connection-data.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_tcp_connection_data_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_tcp_connection_data_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "tcp/rp-tcp-connection-data.h"

struct _RpTcpConnectionData {
    GObject parent_instance;

    RpActiveTcpClient* m_parent;
    RpNetworkClientConnection* m_connection;
};

static void tcp_conn_pool_connection_data_iface_init(RpTcpConnPoolConnectionDataInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpTcpConnectionData, rp_tcp_connection_data, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_TCP_CONN_POOL_CONNECTION_DATA, tcp_conn_pool_connection_data_iface_init)
)

static RpNetworkClientConnection*
connection_i(RpTcpConnPoolConnectionData* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_TCP_CONNECTION_DATA(self)->m_connection;
}

static void
add_upstream_callbacks_i(RpTcpConnPoolConnectionData* self, RpTcpConnPoolUpstreamCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    rp_active_tcp_client_set_callbacks_(RP_TCP_CONNECTION_DATA(self)->m_parent, callbacks);
}

static void
tcp_conn_pool_connection_data_iface_init(RpTcpConnPoolConnectionDataInterface* iface)
{
    LOGD("(%p)", iface);
    iface->connection = connection_i;
    iface->add_upstream_callbacks = add_upstream_callbacks_i;
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_tcp_connection_data_parent_class)->constructed(obj);

    RpTcpConnectionData* self = RP_TCP_CONNECTION_DATA(obj);
    rp_active_tcp_client_set_tcp_connection_data_(self->m_parent, self);
}

OVERRIDE void
dispose(GObject* obj)
{
    LOGD("(%p)", obj);

    RpTcpConnectionData* self = RP_TCP_CONNECTION_DATA(obj);
    if (self->m_parent)
    {
        NOISY_MSG_("clearing clalbacks");
        rp_active_tcp_client_clear_callbacks(self->m_parent);
    }

    G_OBJECT_CLASS(rp_tcp_connection_data_parent_class)->dispose(obj);
}

static void
rp_tcp_connection_data_class_init(RpTcpConnectionDataClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = constructed;
    object_class->dispose = dispose;
}

static void
rp_tcp_connection_data_init(RpTcpConnectionData* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpTcpConnectionData*
rp_tcp_connection_data_new(RpActiveTcpClient* parent, RpNetworkClientConnection* connection)
{
    LOGD("(%p, %p)", parent, connection);
    g_return_val_if_fail(RP_IS_ACTIVE_TCP_CLIENT(parent), NULL);
    g_return_val_if_fail(RP_IS_NETWORK_CLIENT_CONNECTION(connection), NULL);
    RpTcpConnectionData* self = g_object_new(RP_TYPE_TCP_CONNECTION_DATA, NULL);
    self->m_parent = parent;
    self->m_connection = connection;
    return self;
}

void
rp_tcp_connection_data_release(RpTcpConnectionData* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_TCP_CONNECTION_DATA(self));
    self->m_parent = NULL;
}
