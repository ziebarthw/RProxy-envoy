/*
 * rp-tcp-pool-data.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_tcp_pool_data_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_tcp_pool_data_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-tcp-pool-data.h"

struct _RpTcpPoolData {
    GObject parent_instance;

    RpOnNewConnectionFn m_on_new_connection;
    RpTcpConnPoolInstance* m_pool;
    gpointer m_arg;
};

G_DEFINE_FINAL_TYPE(RpTcpPoolData, rp_tcp_pool_data, G_TYPE_OBJECT)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_tcp_pool_data_parent_class)->dispose(obj);
}

static void
rp_tcp_pool_data_class_init(RpTcpPoolDataClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_tcp_pool_data_init(RpTcpPoolData* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpTcpPoolData*
rp_tcp_pool_data_new(RpOnNewConnectionFn on_new_connection, gpointer arg, RpTcpConnPoolInstance* pool)
{
    LOGD("(%p, %p, %p)", on_new_connection, arg, pool);
    RpTcpPoolData* self = g_object_new(RP_TYPE_TCP_POOL_DATA, NULL);
    self->m_on_new_connection = on_new_connection;
    self->m_arg = arg;
    self->m_pool = pool;
    return self;
}

RpCancellable*
rp_tcp_pool_data_new_connection(RpTcpPoolData* self, RpTcpConnPoolCallbacks* callbacks)
{
    LOGD("(%p, %p)", self, callbacks);
    g_return_val_if_fail(RP_IS_TCP_POOL_DATA(self), NULL);
    g_return_val_if_fail(RP_IS_TCP_CONN_POOL_CALLBACKS(callbacks), NULL);
    self->m_on_new_connection(self->m_pool, self->m_arg);
    return rp_tcp_conn_pool_instance_new_connection(self->m_pool, callbacks);
}

RpHostDescriptionConstSharedPtr
rp_tcp_pool_data_host(RpTcpPoolData* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_TCP_POOL_DATA(self), NULL);
    return rp_connection_pool_instance_host(RP_CONNECTION_POOL_INSTANCE(self->m_pool));
}