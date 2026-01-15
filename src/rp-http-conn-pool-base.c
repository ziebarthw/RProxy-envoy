/*
 * rp-http-conn-pool-base.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_http_conn_pool_base_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_http_conn_pool_base_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-codec-client.h"
#include "rp-http-conn-pool-base-active-client.h"
#include "rp-http-pending-stream.h"
#include "rp-router.h"
#include "rp-http-conn-pool-base.h"

typedef struct _RpHttpConnPoolImplBasePrivate RpHttpConnPoolImplBasePrivate;
struct _RpHttpConnPoolImplBasePrivate {
    void* m_origin;
};

enum
{
    PROP_0, // Reserved.
    PROP_ORIGIN,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void http_connection_pool_instance_iface_init(RpHttpConnectionPoolInstanceInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpHttpConnPoolImplBase, rp_http_conn_pool_impl_base, RP_TYPE_CONN_POOL_IMPL_BASE,
    G_ADD_PRIVATE(RpHttpConnPoolImplBase)
    G_IMPLEMENT_INTERFACE(RP_TYPE_HTTP_CONNECTION_POOL_INSTANCE, http_connection_pool_instance_iface_init)
)

#define PRIV(obj) \
    ((RpHttpConnPoolImplBasePrivate*) rp_http_conn_pool_impl_base_get_instance_private(RP_HTTP_CONN_POOL_IMPL_BASE(obj)))

static void
add_idle_callback_i(RpConnectionPoolInstance* self, RpIdleCb cb)
{
    NOISY_MSG_("(%p, %p)", self, cb);
    rp_conn_pool_impl_base_add_idle_callback_impl(RP_CONN_POOL_IMPL_BASE(self), cb);
}

static bool
is_idle_i(RpConnectionPoolInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_conn_pool_impl_base_is_idle_impl(RP_CONN_POOL_IMPL_BASE(self));
}

static void
drain_connections_i(RpConnectionPoolInstance* self, RpDrainBehavior_e drain_behavior)
{
    NOISY_MSG_("(%p, %d)", self, drain_behavior);
    rp_conn_pool_impl_base_drain_connections_impl(RP_CONN_POOL_IMPL_BASE(self), drain_behavior);
}

static RpHostDescription*
host_i(RpConnectionPoolInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HOST_DESCRIPTION(rp_conn_pool_impl_base_host(RP_CONN_POOL_IMPL_BASE(self)));
}

static bool
maybe_preconnect_i(RpConnectionPoolInstance* self, float ratio)
{
    NOISY_MSG_("(%p, %.1f)", self, ratio);
    return rp_conn_pool_impl_base_maybe_preconnect_impl(RP_CONN_POOL_IMPL_BASE(self), ratio);
}

static void
connection_pool_instance_iface_init(RpConnectionPoolInstanceInterface* iface)
{
    LOGD("(%p)", iface);
    iface->add_idle_callback = add_idle_callback_i;
    iface->is_idle = is_idle_i;
    iface->drain_connections = drain_connections_i;
    iface->host = host_i;
    iface->maybe_preconnect = maybe_preconnect_i;
}

static RpCancellable*
new_stream_i(RpHttpConnectionPoolInstance* self, RpResponseDecoder* response_decoder,
                RpHttpConnPoolCallbacks* callbacks, RpHttpConnPoolInstStreamOptionsPtr options)
{
    NOISY_MSG_("(%p, %p, %p, %p)", self, response_decoder, callbacks, options);
    struct _RpHttpAttachContext context = rp_http_attach_context_ctor(response_decoder, callbacks);
    RpCancellable* cancellable = rp_conn_pool_impl_base_new_stream_impl(RP_CONN_POOL_IMPL_BASE(self),
                                                                        (RpConnectionPoolAttachContextPtr)&context,
                                                                        options->m_can_send_early_data);
    return cancellable;
}

static bool
has_active_connections_i(RpHttpConnectionPoolInstance* self)
{
    NOISY_MSG_("(%p)", self);
    RpConnPoolImplBase* me = RP_CONN_POOL_IMPL_BASE(self);
    return (rp_conn_pool_impl_base_has_pending_streams(me) ||
            rp_conn_pool_impl_base_has_active_streams(me));
}

static void
http_connection_pool_instance_iface_init(RpHttpConnectionPoolInstanceInterface* iface)
{
    LOGD("(%p)", iface);
    connection_pool_instance_iface_init(&iface->parent_iface);
    iface->new_stream = new_stream_i;
    iface->has_active_connections = has_active_connections_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_ORIGIN:
            g_value_set_pointer(value, PRIV(obj)->m_origin);
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
        case PROP_ORIGIN:
            PRIV(obj)->m_origin = g_value_get_pointer(value);
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

    rp_conn_pool_impl_base_destruct_all_connections(RP_CONN_POOL_IMPL_BASE(obj));

    G_OBJECT_CLASS(rp_http_conn_pool_impl_base_parent_class)->dispose(obj);
}

OVERRIDE void
on_pool_failure(RpConnPoolImplBase* self, RpHostDescription* host_description,
                const char* failure_reason, RpPoolFailureReason_e reason, RpConnectionPoolAttachContextPtr context)
{
    NOISY_MSG_("(%p, %p, %p(%s), %d, %p)",
        self, host_description, failure_reason, failure_reason, reason, context);
    RpHttpConnPoolCallbacks* callbacks = ((RpHttpAttachContextPtr)context)->m_callbacks;
    rp_http_conn_pool_callbacks_on_pool_failure(callbacks, reason, failure_reason, host_description);
}

OVERRIDE void
on_pool_ready(RpConnPoolImplBase* self, RpConnectionPoolActiveClientPtr client, RpConnectionPoolAttachContextPtr context)
{
    NOISY_MSG_("(%p, %p, %p)", self, client, context);

    RpHttpConnPoolBaseActiveClient* http_client = RP_HTTP_CONN_POOL_BASE_ACTIVE_CLIENT(client);
    RpHttpAttachContextPtr http_context = (RpHttpAttachContextPtr)context;
    RpResponseDecoder* response_decoder = http_context->m_decoder;
    RpHttpConnPoolCallbacks* callbacks = http_context->m_callbacks;
    RpRequestEncoder* new_encoder = rp_http_conn_pool_base_active_client_new_stream_encoder(http_client, response_decoder);
    RpCodecClient* codec_client = rp_http_conn_pool_base_active_client_codec_client_(RP_HTTP_CONN_POOL_BASE_ACTIVE_CLIENT(client));
    rp_http_conn_pool_callbacks_on_pool_ready(callbacks,
                                                new_encoder,
                                                rp_connection_pool_active_client_get_real_host_description(client),
                                                rp_codec_client_stream_info(codec_client),
                                                rp_codec_client_protocol(codec_client));
}

OVERRIDE RpCancellable*
new_pending_stream(RpConnPoolImplBase* self, RpConnectionPoolAttachContextPtr context, bool can_send_early_data)
{
    NOISY_MSG_("(%p, %p, %u)", self, context, can_send_early_data);
    RpResponseDecoder* decoder = ((RpHttpAttachContextPtr)context)->m_decoder;
    RpHttpConnPoolCallbacks* callbacks = ((RpHttpAttachContextPtr)context)->m_callbacks;
    RpHttpPendingStream* pending_stream = rp_http_pending_stream_new(self, decoder, callbacks, can_send_early_data);
    return rp_conn_pool_impl_base_add_pending_stream(self, RP_PENDING_STREAM(g_steal_pointer(&pending_stream)));
}

static inline void
conn_pool_impl_base_class_init(RpConnPoolImplBaseClass* klass)
{
    LOGD("(%p)", klass);
    klass->on_pool_failure = on_pool_failure;
    klass->on_pool_ready = on_pool_ready;
    klass->new_pending_stream = new_pending_stream;
}

static void
rp_http_conn_pool_impl_base_class_init(RpHttpConnPoolImplBaseClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    conn_pool_impl_base_class_init(RP_CONN_POOL_IMPL_BASE_CLASS(klass));

    obj_properties[PROP_ORIGIN] = g_param_spec_pointer("origin",
                                                    "Origin",
                                                    "Optional Origin Instance",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_http_conn_pool_impl_base_init(RpHttpConnPoolImplBase* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}
