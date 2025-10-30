/*
 * rp-per-host-http-upstream.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_per_host_http_upstream_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_per_host_http_upstream_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-per-host-http-upstream.h"

struct _RpPerHostHttpUpstream {
    RpHttpUpstream parent_instance;

    RpHostDescription* m_host;
};

enum
{
    PROP_0, // Reserved.
    PROP_HOST,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

// https://github.com/envoyproxy/envoy/blob/main/test/integration/upstreams/per_host_upstream_config.h
// class PerHostHttpUpstream : public Extensions::Upstreams::Http::Http::HttpUpstream
G_DEFINE_FINAL_TYPE(RpPerHostHttpUpstream, rp_per_host_http_upstream, RP_TYPE_HTTP_UPSTREAM)

OVERRIDE void
set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_HOST:
            RP_PER_HOST_HTTP_UPSTREAM(obj)->m_host = g_value_get_object(value);
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
    G_OBJECT_CLASS(rp_per_host_http_upstream_parent_class)->dispose(obj);
}

static void
rp_per_host_http_upstream_class_init(RpPerHostHttpUpstreamClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_HOST] = g_param_spec_object("host",
                                                    "Host",
                                                    "HostDescription Instance",
                                                    RP_TYPE_HOST_DESCRIPTION,
                                                    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_per_host_http_upstream_init(RpPerHostHttpUpstream* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpPerHostHttpUpstream*
rp_per_host_http_upstream_new(RpUpstreamToDownstream* upstream_request, RpRequestEncoder* request_encoder, RpHostDescription* host)
{
    LOGD("(%p, %p, %p)", upstream_request, request_encoder, host);
    g_return_val_if_fail(RP_IS_UPSTREAM_TO_DOWNSTREAM(upstream_request), NULL);
    g_return_val_if_fail(RP_IS_REQUEST_ENCODER(request_encoder), NULL);
    g_return_val_if_fail(RP_IS_HOST_DESCRIPTION(host), NULL);
    return g_object_new(RP_TYPE_PER_HOST_HTTP_UPSTREAM,
                        "upstream-request", upstream_request,
                        "request-encoder", request_encoder,
                        "host", host,
                        NULL);
}
