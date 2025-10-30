/*
 * rp-conn-pool.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_conn_pool_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_conn_pool_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "tcp/rp-active-tcp-client.h"
#include "tcp/rp-tcp-connection-data.h"
#include "tcp/rp-conn-pool.h"

struct _RpTcpConnPoolImpl {
    RpConnPoolImplBase parent_instance;

};

static void tcp_conn_pool_instance_iface_init(RpTcpConnPoolInstanceInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpTcpConnPoolImpl, rp_tcp_conn_pool_impl, RP_TYPE_CONN_POOL_IMPL_BASE,
    G_IMPLEMENT_INTERFACE(RP_TYPE_TCP_CONN_POOL_INSTANCE, tcp_conn_pool_instance_iface_init)
)

static inline void
close_connections(GList** list)
{
    NOISY_MSG_("(%p)", list);
    while (*list)
    {
        RpConnectionPoolActiveClient* client = RP_CONNECTION_POOL_ACTIVE_CLIENT((*list)->data);
        rp_connection_pool_active_client_close(client);
    }
}

static void
close_connections_i(RpTcpConnPoolInstance* self)
{
    NOISY_MSG_("(%p)", self);
    close_connections(rp_conn_pool_impl_base_ready_clients_(RP_CONN_POOL_IMPL_BASE(self)));
    close_connections(rp_conn_pool_impl_base_busy_clients_(RP_CONN_POOL_IMPL_BASE(self)));
    close_connections(rp_conn_pool_impl_base_connecting_clients_(RP_CONN_POOL_IMPL_BASE(self)));
}

static RpCancellable*
new_connection_i(RpTcpConnPoolInstance* self, RpTcpConnPoolCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    RpTcpAttachContext context = rp_tcp_attach_context_ctor(callbacks);
    return rp_conn_pool_impl_base_new_stream_impl(RP_CONN_POOL_IMPL_BASE(self),
                                                    (RpConnectionPoolAttachContextPtr)&context,
                                                    /*can_send_early_data*/false);
}

static void
tcp_conn_pool_instance_iface_init(RpTcpConnPoolInstanceInterface* iface)
{
    LOGD("(%p)", iface);
    iface->close_connections = close_connections_i;
    iface->new_connection = new_connection_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_tcp_conn_pool_impl_parent_class)->dispose(obj);
}

OVERRIDE RpCancellable*
new_pending_stream(RpConnPoolImplBase* self, RpConnectionPoolAttachContextPtr context, bool can_send_early_data)
{
    NOISY_MSG_("(%p, %p, %u)", self, context, can_send_early_data);
    RpTcpPendingStream* pending_stream = rp_tcp_pending_stream_new(self, can_send_early_data, (RpTcpAttachContext*)context);
    return rp_conn_pool_impl_base_add_pending_stream(self, RP_PENDING_STREAM(pending_stream));
}

OVERRIDE RpConnectionPoolActiveClientPtr
instantiate_active_client(RpConnPoolImplBase* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CONNECTION_POOL_ACTIVE_CLIENT(
        rp_active_tcp_client_new(self, rp_conn_pool_impl_base_host(self), 1/*TODO..., idle_timeout*/));
}

OVERRIDE void
on_pool_ready(RpConnPoolImplBase* self, RpConnectionPoolActiveClientPtr client, RpConnectionPoolAttachContextPtr context)
{
    NOISY_MSG_("(%p, %p, %p)", self, client, context);
    RpActiveTcpClient* tcp_client = RP_ACTIVE_TCP_CLIENT(client);
    rp_active_tcp_client_read_enable_if_new(tcp_client);
    RpTcpConnPoolCallbacks* callbacks = ((RpTcpAttachContext*)context)->m_callbacks;
    RpTcpConnPoolConnectionData* connection_data = RP_TCP_CONN_POOL_CONNECTION_DATA(
                                                    rp_tcp_connection_data_new(tcp_client,
                                                        rp_active_tcp_client_connection_(tcp_client)));
    rp_tcp_conn_pool_callbacks_on_pool_ready(callbacks,
                                                g_steal_pointer(&connection_data),
                                                rp_connection_pool_active_client_get_real_host_description(client));

    if (!connection_data)
    {
        //TODO...idle timer stuff.
    }
}

OVERRIDE void
on_pool_failure(RpConnPoolImplBase* self, RpHostDescription* host_description, const char* failure_reason, RpPoolFailureReason_e reason, RpConnectionPoolAttachContextPtr context)
{
    NOISY_MSG_("(%p, %p, %p(%s), %d, %p)",
        self, host_description, failure_reason, failure_reason, reason, context);
    RpTcpConnPoolCallbacks* callbacks = ((RpTcpAttachContext*)context)->m_callbacks;
    rp_tcp_conn_pool_callbacks_on_pool_failure(callbacks, reason, failure_reason, host_description);
}

static void
conn_pool_impl_base_class_init(RpConnPoolImplBaseClass* klass)
{
    LOGD("(%p)", klass);
    klass->new_pending_stream = new_pending_stream;
    klass->instantiate_active_client = instantiate_active_client;
    klass->on_pool_ready = on_pool_ready;
    klass->on_pool_failure = on_pool_failure;
}

static void
rp_tcp_conn_pool_impl_class_init(RpTcpConnPoolImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    conn_pool_impl_base_class_init(RP_CONN_POOL_IMPL_BASE_CLASS(klass));
}

static void
rp_tcp_conn_pool_impl_init(RpTcpConnPoolImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpTcpConnPoolImpl*
rp_tcp_conn_pool_impl_new(RpDispatcher* dispatcher, RpHost* host, RpResourcePriority_e priority G_GNUC_UNUSED)
{
    LOGD("(%p, %p, %d)", dispatcher, host, priority);
    g_return_val_if_fail(RP_IS_DISPATCHER(dispatcher), NULL);
    g_return_val_if_fail(RP_IS_HOST(host), NULL);
    return g_object_new(RP_TYPE_TCP_CONN_POOL_IMPL,
                        "dispatcher", dispatcher,
                        "host", host,
                        NULL);
}
