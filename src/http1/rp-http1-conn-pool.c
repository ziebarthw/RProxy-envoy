/*
 * rp-http1-conn-pool.c
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

#include "rp-codec-client-prod.h"
#include "rp-fixed-http-conn-pool-impl.h"
#include "http1/rp-stream-wrapper.h"
#include "http1/rp-http1-conn-pool.h"

struct _RpHttp1CpActiveClient {
    RpHttpConnPoolBaseActiveClient parent_instance;

    RpHttp1StreamWrapper* m_stream_wrapper;
    RpHttpConnPoolImplBase* m_parent;
};

enum
{
    PROP_0, // Reserved.
    PROP_PARENT,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_FINAL_TYPE(RpHttp1CpActiveClient, rp_http1_cp_active_client, RP_TYPE_HTTP_CONN_POOL_BASE_ACTIVE_CLIENT)

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_PARENT:
            g_value_set_object(value, RP_HTTP1_CP_ACTIVE_CLIENT(obj)->m_parent);
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
        case PROP_PARENT:
            RP_HTTP1_CP_ACTIVE_CLIENT(obj)->m_parent = g_value_get_object(value);
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
    G_OBJECT_CLASS(rp_http1_cp_active_client_parent_class)->dispose(obj);
}

OVERRIDE bool
closing_with_incomplete_stream(RpConnectionPoolActiveClient* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttp1CpActiveClient* me = RP_HTTP1_CP_ACTIVE_CLIENT(self);
    return me->m_stream_wrapper &&
            !rp_http1_stream_wrapper_decode_complete_(me->m_stream_wrapper);
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
            rp_conn_pool_impl_base_dispatcher(RP_CONN_POOL_IMPL_BASE(me->m_parent)), G_OBJECT(me->m_stream_wrapper));
        me->m_stream_wrapper = NULL;
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
    me->m_stream_wrapper = rp_http1_stream_wrapper_new(response_decoder, me);
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
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    http_conn_pool_base_active_client_class_init(RP_HTTP_CONN_POOL_BASE_ACTIVE_CLIENT_CLASS(klass));

    obj_properties[PROP_PARENT] = g_param_spec_object("parent",
                                                    "Parent",
                                                    "Parent HttpConnPoolImplBase Instance",
                                                    RP_TYPE_HTTP_CONN_POOL_IMPL_BASE,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
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
    return g_object_new(RP_TYPE_HTTP1_CP_ACTIVE_CLIENT,
                        "parent", parent,
                        "lifetime-stream-limit", max_requests_per_connection(parent),
                        "effective-concurrent-streams", 1,
                        "concurrent-stream-limit", 1,
                        "opt-data", data,
                        NULL);
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
NOISY_MSG_("codec %p", codec);
    return RP_CODEC_CLIENT(codec);
}

RpHttpConnectionPoolInstancePtr
http1_allocate_conn_pool(RpDispatcher* dispatcher, RpHost* host, RpResourcePriority_e priority)
{
    LOGD("(%p, %p, %d)", dispatcher, host, priority);
    evhtp_proto protocols[] = {EVHTP_PROTO_11, EVHTP_PROTO_INVALID};
NOISY_MSG_("protocols %p, protocols[0] %d", protocols, protocols[0]);
    RpFixedHttpConnPoolImpl* pool = rp_fixed_http_conn_pool_impl_new(host,
                                                                        priority,
                                                                        dispatcher,
                                                                        client_fn,
                                                                        codec_fn,
                                                                        protocols,
                                                                        NULL);
    return RP_HTTP_CONNECTION_POOL_INSTANCE(pool);
}
