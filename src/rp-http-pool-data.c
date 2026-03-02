/*
 * rp-http-pool-data.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_http_pool_data_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_http_pool_data_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-http-pool-data.h"

struct _RpHttpPoolData {
    GObject parent_instance;

    RpOnNewStreamFn m_on_new_stream;
    RpHttpConnectionPoolInstance* m_pool;
    gpointer m_arg;
};

G_DEFINE_FINAL_TYPE(RpHttpPoolData, rp_http_pool_data, G_TYPE_OBJECT)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_http_pool_data_parent_class)->dispose(obj);
}

static void
rp_http_pool_data_class_init(RpHttpPoolDataClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_http_pool_data_init(RpHttpPoolData* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpHttpPoolData*
rp_http_pool_data_new(RpOnNewStreamFn on_new_stream, gpointer arg, RpHttpConnectionPoolInstance* pool)
{
    LOGD("(%p, %p, %p)", on_new_stream, arg, pool);
    RpHttpPoolData* self = g_object_new(RP_TYPE_HTTP_POOL_DATA, NULL);
    self->m_on_new_stream = on_new_stream;
    self->m_arg = arg;
    self->m_pool = pool;
    return self;
}

RpCancellable*
rp_http_pool_data_new_stream(RpHttpPoolData* self, RpResponseDecoder* response_decoder,
                                RpHttpConnPoolCallbacks* callbacks, RpHttpConnPoolInstStreamOptionsPtr stream_options)
{
    LOGD("(%p, %p, %p)", self, response_decoder, callbacks);
    g_return_val_if_fail(RP_IS_HTTP_POOL_DATA(self), NULL);
    g_return_val_if_fail(RP_IS_RESPONSE_DECODER(response_decoder), NULL);
    g_return_val_if_fail(RP_IS_HTTP_CONN_POOL_CALLBACKS(callbacks), NULL);
    self->m_on_new_stream(self->m_pool, self->m_arg);
    return rp_http_connection_pool_instance_new_stream(self->m_pool, response_decoder, callbacks, stream_options);
}

bool
rp_http_pool_data_has_active_connections(RpHttpPoolData* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_POOL_DATA(self), false);
    return rp_http_connection_pool_instance_has_active_connections(self->m_pool);
}

void
rp_http_pool_data_add_idle_callbacks(RpHttpPoolData* self, RpIdleCb cb)
{
    LOGD("(%p, %p)", self, cb);
    g_return_if_fail(RP_IS_HTTP_POOL_DATA(self));
    rp_connection_pool_instance_add_idle_callback(RP_CONNECTION_POOL_INSTANCE(self->m_pool), cb);
}

void
rp_http_pool_data_drain_connections(RpHttpPoolData* self, RpDrainBehavior_e drain_behavior)
{
    LOGD("(%p, %d)", self, drain_behavior);
    g_return_if_fail(RP_IS_HTTP_POOL_DATA(self));
    rp_connection_pool_instance_drain_connections(RP_CONNECTION_POOL_INSTANCE(self->m_pool), drain_behavior);
}

RpHostDescriptionConstSharedPtr
rp_http_pool_data_host(RpHttpPoolData* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_POOL_DATA(self), NULL);
    return rp_connection_pool_instance_host(RP_CONNECTION_POOL_INSTANCE(self->m_pool));
}
