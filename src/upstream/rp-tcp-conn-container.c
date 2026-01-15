/*
 * rp-tcp-conn-container.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_tcp_conn_container_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_tcp_conn_container_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-cluster-manager-impl.h"

struct _RpTcpConnContainer {
    GObject parent_instance;

    RpThreadLocalClusterManagerImpl* m_parent;
    RpHostConstSharedPtr m_host;
    RpNetworkClientConnection* m_connection;
};

static void network_connection_callbacks_iface_init(RpNetworkConnectionCallbacksInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpTcpConnContainer, rp_tcp_conn_container, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_CONNECTION_CALLBACKS, network_connection_callbacks_iface_init)
)

static void
on_event_i(RpNetworkConnectionCallbacks* self, RpNetworkConnectionEvent_e event)
{
    NOISY_MSG_("(%p, %d)", self, event);

    if (event == RpNetworkConnectionEvent_LocalClose ||
        event == RpNetworkConnectionEvent_RemoteClose)
    {
        NOISY_MSG_("closed");
        RpTcpConnContainer* me = RP_TCP_CONN_CONTAINER(self);
        rp_thread_local_cluster_manager_impl_remove_tcp_conn(me->m_parent, me->m_host, me->m_connection);
    }
}

static void
on_above_write_buffer_high_water_mark_i(RpNetworkConnectionCallbacks* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static void
on_below_write_buffer_low_watermark_i(RpNetworkConnectionCallbacks* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static void
network_connection_callbacks_iface_init(RpNetworkConnectionCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_event = on_event_i;
    iface->on_above_write_buffer_high_water_mark = on_above_write_buffer_high_water_mark_i;
    iface->on_below_write_buffer_low_watermark = on_below_write_buffer_low_watermark_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_tcp_conn_container_parent_class)->dispose(obj);
}

static void
rp_tcp_conn_container_class_init(RpTcpConnContainerClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_tcp_conn_container_init(RpTcpConnContainer* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpTcpConnContainer*
constructed(RpTcpConnContainer* self)
{
    NOISY_MSG_("(%p)", self);

    rp_network_connection_add_connection_callbacks(RP_NETWORK_CONNECTION(self->m_connection),
                                                    RP_NETWORK_CONNECTION_CALLBACKS(self));
    return self;
}

RpTcpConnContainer*
rp_tcp_conn_container_new(RpThreadLocalClusterManagerImpl* parent, RpHostConstSharedPtr host, RpNetworkClientConnection* connection)
{
    LOGD("(%p, %p, %p)", parent, host, connection);

    g_return_val_if_fail(RP_IS_THREAD_LOCAL_CLUSTER_MANAGER_IMPL(parent), NULL);
    g_return_val_if_fail(RP_IS_HOST(host), NULL);
    g_return_val_if_fail(RP_IS_NETWORK_CLIENT_CONNECTION(connection), NULL);

    RpTcpConnContainer* self = g_object_new(RP_TYPE_TCP_CONN_CONTAINER, NULL);
    self->m_parent = parent;
    self->m_host = host;
    self->m_connection = connection;
    return constructed(self);
}
