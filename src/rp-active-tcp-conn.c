/*
 * rp-active-tcp-conn.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_active_tcp_conn_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_active_tcp_conn_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include "rp-active-tcp-conn.h"

struct _RpActiveTcpConn {
    GObject parent_instance;

    GList** m_active_connections;
    UNIQUE_PTR(RpStreamInfo) m_stream_info;
    UNIQUE_PTR(RpNetworkConnection) m_connection;
    SHARED_PTR(thread_ctx_t) m_thread_ctx;
};

static void network_connection_callbacks_iface_init(RpNetworkConnectionCallbacksInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpActiveTcpConn, rp_active_tcp_conn, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_CONNECTION_CALLBACKS, network_connection_callbacks_iface_init)
)

static void
remove_connection(RpActiveTcpConn* self)
{
    RpDispatcher* dispatcher = rp_network_connection_dispatcher(self->m_connection);
    *self->m_active_connections = g_list_remove(*self->m_active_connections, self);
    rp_dispatcher_deferred_delete(dispatcher, G_OBJECT(self));
    //TODO...if (active_connection.connections_.empty())
}

void
on_event_i(RpNetworkConnectionCallbacks* self, RpNetworkConnectionEvent_e event)
{
    NOISY_MSG_("(%p, %d)", self, event);

    if (event == RpNetworkConnectionEvent_LocalClose ||
        event == RpNetworkConnectionEvent_RemoteClose)
    {
        NOISY_MSG_("calling remove_connection(%p)", self);
        //TODO...stream_info_->setDownstream...
        RpActiveTcpConn* me = RP_ACTIVE_TCP_CONN(self);
        g_atomic_int_dec_and_test(&me->m_thread_ctx->n_processing);
        remove_connection(me);
    }
}

static void
network_connection_callbacks_iface_init(RpNetworkConnectionCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_event = on_event_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpActiveTcpConn* self = RP_ACTIVE_TCP_CONN(obj);
NOISY_MSG_("clearing connection %p", self->m_connection);
    g_clear_object(&self->m_connection);
    g_clear_object(&self->m_stream_info);

    G_OBJECT_CLASS(rp_active_tcp_conn_parent_class)->dispose(obj);
}

static void
rp_active_tcp_conn_class_init(RpActiveTcpConnClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_active_tcp_conn_init(RpActiveTcpConn* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpActiveTcpConn*
constructed(RpActiveTcpConn* self)
{
    NOISY_MSG_("(%p)", self);

    rp_network_connection_add_connection_callbacks(self->m_connection,
                                                    RP_NETWORK_CONNECTION_CALLBACKS(self));
    return self;
}

RpActiveTcpConn*
rp_active_tcp_conn_new(GList** active_connections, UNIQUE_PTR(RpNetworkConnection) new_connection, UNIQUE_PTR(RpStreamInfo) stream_info, SHARED_PTR(thread_ctx_t) thread_ctx)
{
    LOGD("(%p, %p, %p, %p)", active_connections, new_connection, stream_info, thread_ctx);
    g_return_val_if_fail(active_connections != NULL, NULL);
    g_return_val_if_fail(RP_IS_NETWORK_CONNECTION(new_connection), NULL);
    g_return_val_if_fail(RP_IS_STREAM_INFO(stream_info), NULL);
    RpActiveTcpConn* self = g_object_new(RP_TYPE_ACTIVE_TCP_CONN, NULL);
    self->m_active_connections = active_connections;
    self->m_connection = g_steal_pointer(&new_connection);
    self->m_stream_info = g_steal_pointer(&stream_info);
    self->m_thread_ctx = thread_ctx;
    return constructed(self);
}
