/*
 * rp-codec-client-prod.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_codec_client_prod_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_codec_client_prod_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "http1/rp-http1-client-connection-impl.h"
#include "rp-conn-manager-config.h"
#include "rp-codec-client-prod.h"

struct _RpCodecClientProd {
    RpCodecClient parent_instance;

};

G_DEFINE_FINAL_TYPE(RpCodecClientProd, rp_codec_client_prod, RP_TYPE_CODEC_CLIENT)

static inline RpCodecType_e
get_codec_type(RpCodecClientProd* self)
{
    RpCodecType_e type;
    g_object_get(G_OBJECT(self), "type", &type, NULL);
    return type;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_codec_client_prod_parent_class)->dispose(obj);
}

static void
rp_codec_client_prod_class_init(RpCodecClientProdClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_codec_client_prod_init(RpCodecClientProd* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpCodecClientProd*
constructed(RpCodecClientProd* self, RpNetworkClientConnection* connection, bool should_connect)
{
    NOISY_MSG_("(%p, %p, %u)", self, connection, should_connect);

    RpHttpClientConnection* codec = NULL;

    switch (get_codec_type(self))
    {
        case RpCodecType_HTTP1:
            codec = RP_HTTP_CLIENT_CONNECTION(
                rp_http1_client_connection_impl_new(RP_NETWORK_CONNECTION(connection),
                                                    RP_HTTP_CONNECTION_CALLBACKS(self),
                                                    &RpHttp1Settings,
                                                    DEFAULT_MAX_REQUEST_HEADERS_KB,
                                                    DEFAULT_MAX_HEADERS_COUNT,
                                                    false/*TODO...initialize*/)
            );
            break;
        case RpCodecType_HTTP2:
            g_info("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
            break;
        case RpCodecType_HTTP3:
            g_info("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
            break;
    }

    LOGD("codec %p", codec);

    g_object_set(G_OBJECT(self), "codec", codec, NULL);

    if (should_connect)
    {
        NOISY_MSG_("should connect....");
        rp_codec_client_connect(RP_CODEC_CLIENT(self));
    }

    return self;
}

RpCodecClientProd*
rp_codec_client_prod_new(RpCodecType_e type, RpNetworkClientConnection* connection,
                            RpHostDescription* host, RpDispatcher* dispatcher, bool should_connect_on_creation)
{
    LOGD("(%d, %p, %p, %p, %u)", type, connection, host, dispatcher, should_connect_on_creation);
    g_return_val_if_fail(RP_IS_NETWORK_CLIENT_CONNECTION(connection), NULL);
    RpCodecClientProd* self = g_object_new(RP_TYPE_CODEC_CLIENT_PROD,
                        "type", type,
                        "connection", connection,
                        "host", host,
                        "dispatcher", dispatcher,
                        NULL);
    return constructed(self, connection, should_connect_on_creation);
}
