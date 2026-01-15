/*
 * rp-fixed-http-conn-pool-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_fixed_http_conn_pool_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_fixed_http_conn_pool_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "common/conn_pool/rp-conn-pool-base-active-client.h"
#include "rp-http-utility.h"
#include "rp-fixed-http-conn-pool-impl.h"

struct _RpFixedHttpConnPoolImpl {
    RpHttpConnPoolImplBase parent_instance;

    RpCreateCodecFn m_codec_fn;
    RpCreateClientFn m_client_fn;

    void* m_origin;

    evhtp_proto* m_protocols;
    evhtp_proto m_protocol;
};

G_DEFINE_FINAL_TYPE(RpFixedHttpConnPoolImpl, rp_fixed_http_conn_pool_impl, RP_TYPE_HTTP_CONN_POOL_IMPL_BASE)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_fixed_http_conn_pool_impl_parent_class)->dispose(obj);
}

OVERRIDE RpConnectionPoolActiveClientPtr
instantiate_active_client(RpConnPoolImplBase* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_FIXED_HTTP_CONN_POOL_IMPL(self)->m_client_fn(RP_HTTP_CONN_POOL_IMPL_BASE(self));
}

static void
conn_pool_impl_base_class_init(RpConnPoolImplBaseClass* klass)
{
    LOGD("(%p)", klass);
    klass->instantiate_active_client = instantiate_active_client;
}

OVERRIDE RpCodecClient*
create_codec_client(RpHttpConnPoolImplBase* self, RpCreateConnectionDataPtr data)
{
    NOISY_MSG_("(%p, %p)", self, data);
    return RP_FIXED_HTTP_CONN_POOL_IMPL(self)->m_codec_fn(data, self);
}

static const char*
protocol_description_i(RpHttpConnectionPoolInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return http_utility_get_protocol_string(RP_FIXED_HTTP_CONN_POOL_IMPL(self)->m_protocol);
}

static void
http_connection_pool_instance_iface_init(RpHttpConnectionPoolInstanceInterface* iface)
{
    LOGD("(%p)", iface);
    iface->protocol_description = protocol_description_i;
}

static void
http_conn_pool_impl_base_class_init(RpHttpConnPoolImplBaseClass* klass)
{
    LOGD("(%p)", klass);
    conn_pool_impl_base_class_init(RP_CONN_POOL_IMPL_BASE_CLASS(klass));
    http_connection_pool_instance_iface_init(g_type_interface_peek(klass, RP_TYPE_HTTP_CONNECTION_POOL_INSTANCE));
    klass->create_codec_client = create_codec_client;
}

static void
rp_fixed_http_conn_pool_impl_class_init(RpFixedHttpConnPoolImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    http_conn_pool_impl_base_class_init(RP_HTTP_CONN_POOL_IMPL_BASE_CLASS(klass));
}

static void
rp_fixed_http_conn_pool_impl_init(RpFixedHttpConnPoolImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpFixedHttpConnPoolImpl*
rp_fixed_http_conn_pool_impl_new(RpHost* host, RpResourcePriority_e priority, RpDispatcher* dispatcher,
                                    RpCreateClientFn client_fn, RpCreateCodecFn codec_fn, evhtp_proto* protocols, void* origin)
{
    LOGD("(%p, %d, %p, %p, %p, %p, %p)",
        host, priority, dispatcher, client_fn, codec_fn, protocols, origin);
    g_return_val_if_fail(RP_IS_HOST(host), NULL);
    g_return_val_if_fail(RP_IS_DISPATCHER(dispatcher), NULL);
    g_return_val_if_fail(client_fn != NULL, NULL);
    g_return_val_if_fail(codec_fn != NULL, NULL);
    RpFixedHttpConnPoolImpl* self = g_object_new(RP_TYPE_FIXED_HTTP_CONN_POOL_IMPL,
                                                    "host", host,
                                                    "dispatcher", dispatcher,
                                                    NULL);
    self->m_origin = origin;
    self->m_client_fn = client_fn;
    self->m_codec_fn = codec_fn;
    self->m_protocols = protocols;
    self->m_protocol = self->m_protocols[0];
    return self;
}

void
rp_fixed_http_conn_pool_impl_set_origin(RpFixedHttpConnPoolImpl* self, void* origin)
{
    LOGD("(%p, %p)", self, origin);
    g_return_if_fail(RP_IS_FIXED_HTTP_CONN_POOL_IMPL(self));
    self->m_origin = origin;
}
