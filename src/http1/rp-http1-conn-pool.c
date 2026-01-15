/*
 * rp-http1-conn-pool.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_conn_pool_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_conn_pool_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-codec-client-prod.h"
#include "rp-fixed-http-conn-pool-impl.h"
#include "http1/rp-active-client-stream-wrapper.h"
#include "http1/rp-stream-wrapper.h"
#include "http1/rp-http1-conn-pool.h"

struct _RpHttp1CpActiveClient {
    RpHttpConnPoolBaseActiveClient parent_instance;

    UNIQUE_PTR(RpActiveClientStreamWrapper) m_stream_wrapper;
    RpHttpConnPoolImplBase* m_parent;
};

G_DEFINE_FINAL_TYPE(RpHttp1CpActiveClient, rp_http1_cp_active_client, RP_TYPE_HTTP_CONN_POOL_BASE_ACTIVE_CLIENT)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpHttp1CpActiveClient* self = RP_HTTP1_CP_ACTIVE_CLIENT(obj);
    g_clear_object(&self->m_stream_wrapper);

    G_OBJECT_CLASS(rp_http1_cp_active_client_parent_class)->dispose(obj);
}

OVERRIDE bool
closing_with_incomplete_stream(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttp1CpActiveClient* me = RP_HTTP1_CP_ACTIVE_CLIENT(self);
    return me->m_stream_wrapper &&
            !rp_active_client_stream_wrapper_decode_complete_(me->m_stream_wrapper);
}

OVERRIDE guint32
num_active_streams(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HTTP1_CP_ACTIVE_CLIENT(self)->m_stream_wrapper ? 1 : 0;
}

OVERRIDE void
release_resources(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttp1CpActiveClient* me = RP_HTTP1_CP_ACTIVE_CLIENT(self);
    if (me->m_stream_wrapper)
    {
        rp_dispatcher_deferred_delete(
            rp_conn_pool_impl_base_dispatcher(RP_CONN_POOL_IMPL_BASE(me->m_parent)), G_OBJECT(g_steal_pointer(&me->m_stream_wrapper)));
    }
    RP_CONNECTION_POOL_ACTIVE_CLIENT_CLASS(rp_http1_cp_active_client_parent_class)->release_resources(self);
}

static inline void
connection_pool_active_client_class_init(RpConnectionPoolActiveClientClass* klass)
{
    LOGD("(%p)", klass);
    klass->closing_with_incomplete_stream = closing_with_incomplete_stream;
    klass->num_active_streams = num_active_streams;
    klass->release_resources = release_resources;
}

OVERRIDE RpRequestEncoder*
new_stream_encoder(RpHttpConnPoolBaseActiveClient* self, RpResponseDecoder* response_decoder)
{
    NOISY_MSG_("(%p, %p)", self, response_decoder);
    RpHttp1CpActiveClient* me = RP_HTTP1_CP_ACTIVE_CLIENT(self);
    g_assert(!me->m_stream_wrapper);
    me->m_stream_wrapper = rp_active_client_stream_wrapper_new(response_decoder, me);
    return RP_REQUEST_ENCODER(me->m_stream_wrapper);
}

static inline void
http_conn_pool_base_active_client_class_init(RpHttpConnPoolBaseActiveClientClass* klass)
{
    LOGD("(%p)", klass);
    connection_pool_active_client_class_init(RP_CONNECTION_POOL_ACTIVE_CLIENT_CLASS(klass));
    klass->new_stream_encoder = new_stream_encoder;
}

static void
rp_http1_cp_active_client_class_init(RpHttp1CpActiveClientClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    http_conn_pool_base_active_client_class_init(RP_HTTP_CONN_POOL_BASE_ACTIVE_CLIENT_CLASS(klass));
}

static void
rp_http1_cp_active_client_init(RpHttp1CpActiveClient* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline guint64
max_requests_per_connection(RpHttpConnPoolImplBase* parent)
{
    NOISY_MSG_("(%p)", parent);
    return rp_cluster_info_max_requests_per_connection(
            rp_host_description_cluster(
                rp_connection_pool_instance_host(RP_CONNECTION_POOL_INSTANCE(parent))));
}

RpHttp1CpActiveClient*
rp_http1_cp_active_client_new(RpHttpConnPoolImplBase* parent, RpCreateConnectionDataPtr data)
{
    LOGD("(%p, %p)", parent, data);
    g_return_val_if_fail(RP_IS_HTTP_CONN_POOL_IMPL_BASE(parent), NULL);
    RpHttp1CpActiveClient* self = g_object_new(RP_TYPE_HTTP1_CP_ACTIVE_CLIENT,
                                                "parent", parent,
                                                "lifetime-stream-limit", max_requests_per_connection(parent),
                                                "effective-concurrent-streams", 1,
                                                "concurrent-stream-limit", 1,
                                                "opt-data", data,
                                                NULL);
    self->m_parent = parent;
    return self;
}

RpHttpConnPoolImplBase*
rp_http1_cp_active_client_parent(RpHttp1CpActiveClient* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP1_CP_ACTIVE_CLIENT(self), NULL);
    return self->m_parent;
}

void
rp_http1_cp_active_client_stream_wrapper_reset(RpHttp1CpActiveClient* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_HTTP1_CP_ACTIVE_CLIENT(self));
    g_clear_object(&self->m_stream_wrapper);
}

static RpConnectionPoolActiveClientPtr
client_fn(RpHttpConnPoolImplBase* pool)
{
    NOISY_MSG_("(%p)", pool);
    return RP_CONNECTION_POOL_ACTIVE_CLIENT(rp_http1_cp_active_client_new(pool, NULL));
}

static RpCodecClientPtr
codec_fn(RpCreateConnectionDataPtr data, RpHttpConnPoolImplBase* pool)
{
    NOISY_MSG_("(%p, %p)", data, pool);
    RpDispatcher* dispatcher = rp_conn_pool_impl_base_dispatcher(RP_CONN_POOL_IMPL_BASE(pool));
    RpCodecClientProd* codec = rp_codec_client_prod_new(RpCodecType_HTTP1,
                                                        RP_NETWORK_CLIENT_CONNECTION(data->m_connection),
                                                        data->m_host_description,
                                                        dispatcher,
                                                        /*true*/false);
    return RP_CODEC_CLIENT(codec);
}

RpHttpConnectionPoolInstancePtr
http1_allocate_conn_pool(RpDispatcher* dispatcher, RpHost* host, RpResourcePriority_e priority)
{
    LOGD("(%p, %p, %d)", dispatcher, host, priority);
    evhtp_proto protocols[] = {EVHTP_PROTO_11, EVHTP_PROTO_INVALID};
    RpFixedHttpConnPoolImpl* pool = rp_fixed_http_conn_pool_impl_new(host,
                                                                        priority,
                                                                        dispatcher,
                                                                        client_fn,
                                                                        codec_fn,
                                                                        protocols,
                                                                        NULL);
    return RP_HTTP_CONNECTION_POOL_INSTANCE(pool);
}
