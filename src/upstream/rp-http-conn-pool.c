/*
 * rp-http-conn-pool.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_http_conn_pool_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_http_conn_pool_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-http-conn-pool.h"
#include "rp-thread-local-cluster.h"
#include "upstream/rp-http-upstream.h"
#include "upstream/rp-http-conn-pool.h"

typedef struct _RpHttpConnPoolPrivate RpHttpConnPoolPrivate;
struct _RpHttpConnPoolPrivate {

    RpHost* m_host;
    RpThreadLocalCluster* m_thread_local_cluster;
    RpLoadBalancerContext* m_context;

    RpHttpPoolData* m_pool_data;
    RpCancellable* m_conn_pool_stream_handle;
    RpGenericConnectionPoolCallbacks* m_callbacks;

    RpUpstreamProtocol_e m_upstream_protocol;
    RpResourcePriority_e m_priority;

    evhtp_proto m_downstream_protocol;
};

enum
{
    PROP_0, // Reserved.
    PROP_HOST,
    PROP_THREAD_LOCAL_CLUSTER,
    PROP_UPSTREAM_PROTOCOL,
    PROP_PRIORITY,
    PROP_DOWNSTREAM_PROTOCOL,
    PROP_CONTEXT,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void generic_conn_pool_iface_init(RpGenericConnPoolInterface* iface);

// https://github.com/envoyproxy/envoy/blob/main/source/extensions/upstreams/http/http/upstream_request.h
// class HttpConnPool : public Router::GenericConnPool, public Envoy::Http::ConnectionPool::Callbacks

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpHttpConnPool, rp_http_conn_pool, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpHttpConnPool)
    G_IMPLEMENT_INTERFACE(RP_TYPE_GENERIC_CONN_POOL, generic_conn_pool_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_HTTP_CONN_POOL_CALLBACKS, NULL)
)

#define PRIV(obj) \
    ((RpHttpConnPoolPrivate*) rp_http_conn_pool_get_instance_private(RP_HTTP_CONN_POOL(obj)))

static void
new_stream_i(RpGenericConnPool* self, RpGenericConnectionPoolCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    RpHttpConnPoolPrivate* me = PRIV(self);
    me->m_callbacks = callbacks;

    RpUpstreamToDownstream* stream = rp_generic_connection_pool_callbacks_upstream_to_downstream(callbacks);
    RpHttpConnPoolInstStreamOptions options = rp_upstream_to_downstream_upstream_stream_options(stream);
    RpCancellable* handle = rp_http_pool_data_new_stream(me->m_pool_data,
                                                            RP_RESPONSE_DECODER(stream),
                                                            RP_HTTP_CONN_POOL_CALLBACKS(self),
                                                            &options);
    me->m_conn_pool_stream_handle = handle;
}

static RpHostDescription*
host_i(RpGenericConnPool* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HOST_DESCRIPTION(rp_http_pool_data_host(PRIV(self)->m_pool_data));
}

static bool
cancel_any_pending_stream_i(RpGenericConnPool* self)
{
    NOISY_MSG_("(%p)", self);
    RpHttpConnPoolPrivate* me = PRIV(self);
    if (me->m_conn_pool_stream_handle)
    {
        rp_cancellable_cancel(me->m_conn_pool_stream_handle, RpCancelPolicy_Default);
        g_clear_object(&me->m_conn_pool_stream_handle);
        return true;
    }
    return false;
}

static void
generic_conn_pool_iface_init(RpGenericConnPoolInterface* iface)
{
    NOISY_MSG_("(%p)", iface);
    iface->new_stream = new_stream_i;
    iface->host = host_i;
    iface->cancel_any_pending_stream = cancel_any_pending_stream_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_HOST:
            g_value_set_object(value, PRIV(obj)->m_host);
            break;
        case PROP_THREAD_LOCAL_CLUSTER:
            g_value_set_object(value, PRIV(obj)->m_thread_local_cluster);
            break;
        case PROP_UPSTREAM_PROTOCOL:
            g_value_set_int(value, PRIV(obj)->m_upstream_protocol);
            break;
        case PROP_PRIORITY:
            g_value_set_int(value, PRIV(obj)->m_priority);
            break;
        case PROP_DOWNSTREAM_PROTOCOL:
            g_value_set_int(value, PRIV(obj)->m_downstream_protocol);
            break;
        case PROP_CONTEXT:
            g_value_set_object(value, PRIV(obj)->m_context);
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
        case PROP_HOST:
            PRIV(obj)->m_host = g_value_get_object(value);
            break;
        case PROP_THREAD_LOCAL_CLUSTER:
            PRIV(obj)->m_thread_local_cluster = g_value_get_object(value);
            break;
        case PROP_UPSTREAM_PROTOCOL:
            PRIV(obj)->m_upstream_protocol = g_value_get_int(value);
            break;
        case PROP_PRIORITY:
            PRIV(obj)->m_priority = g_value_get_int(value);
            break;
        case PROP_DOWNSTREAM_PROTOCOL:
            PRIV(obj)->m_downstream_protocol = g_value_get_int(value);
            break;
        case PROP_CONTEXT:
            PRIV(obj)->m_context = g_value_get_object(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_http_conn_pool_parent_class)->constructed(obj);

    RpHttpConnPoolPrivate* me = PRIV(obj);
    me->m_pool_data = rp_thread_local_cluster_http_conn_pool(me->m_thread_local_cluster,
                                                                me->m_host,
                                                                me->m_priority,
                                                                me->m_downstream_protocol,
                                                                me->m_context);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpHttpConnPoolPrivate* me = PRIV(obj);
    g_clear_object(&me->m_pool_data);

    G_OBJECT_CLASS(rp_http_conn_pool_parent_class)->dispose(obj);
}

static void
rp_http_conn_pool_class_init(RpHttpConnPoolClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    obj_properties[PROP_HOST] = g_param_spec_object("host",
                                                    "Host",
                                                    "Host Instance",
                                                    RP_TYPE_HOST,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_THREAD_LOCAL_CLUSTER] = g_param_spec_object("thread-local-cluster",
                                                    "Thread local cluster",
                                                    "ThreadLocalCluster Instance",
                                                    RP_TYPE_THREAD_LOCAL_CLUSTER,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_CONTEXT] = g_param_spec_object("context",
                                                    "Context",
                                                    "LocaBalancerContext Instance",
                                                    RP_TYPE_LOAD_BALANCER_CONTEXT,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_UPSTREAM_PROTOCOL] = g_param_spec_int("upstream-protocol",
                                                    "Upstream protocol",
                                                    "Upstream Protocol",
                                                    RpUpstreamProtocol_HTTP,
                                                    RpUpstreamProtocol_UDP,
                                                    RpUpstreamProtocol_HTTP,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_PRIORITY] = g_param_spec_int("priority",
                                                    "Priority",
                                                    "ResourcePriority",
                                                    RpResourcePriority_Default,
                                                    RpResourcePriority_High,
                                                    RpResourcePriority_Default,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_DOWNSTREAM_PROTOCOL] = g_param_spec_int("downstream-protocol",
                                                    "Downstream protocol",
                                                    "Downstream Protocol",
                                                    EVHTP_PROTO_INVALID,
                                                    EVHTP_PROTO_11,
                                                    EVHTP_PROTO_INVALID,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_http_conn_pool_init(RpHttpConnPool* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpCancellable**
rp_http_conn_pool_conn_pool_stream_handle_(RpHttpConnPool* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_CONN_POOL(self), NULL);
    return &PRIV(self)->m_conn_pool_stream_handle;
}

RpGenericConnectionPoolCallbacks*
rp_http_conn_pool_callbacks_(RpHttpConnPool* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_CONN_POOL(self), NULL);
    return PRIV(self)->m_callbacks;
}
