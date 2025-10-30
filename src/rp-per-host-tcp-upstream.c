/*
 * rp-per-host-tcp-upstream.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_per_host_tcp_upstream_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_per_host_tcp_upstream_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-headers.h"
#include "rp-per-host-tcp-upstream.h"

struct _RpPerHostTcpUpstream {
    RpTcpUpstream parent_instance;

};

// https://github.com/envoyproxy/envoy/blob/main/test/integration/upstreams/per_host_upstream_config.h
// class PerHostTcpUpstream : public Extensions::Upstreams::Http::Tcp::TcpUpstream
G_DEFINE_FINAL_TYPE(RpPerHostTcpUpstream, rp_per_host_tcp_upstream, RP_TYPE_TCP_UPSTREAM)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_per_host_tcp_upstream_parent_class)->dispose(obj);
}

static void
rp_per_host_tcp_upstream_class_init(RpPerHostTcpUpstreamClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_per_host_tcp_upstream_init(RpPerHostTcpUpstream* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpPerHostTcpUpstream*
rp_per_host_tcp_upstream_new(RpUpstreamToDownstream* upstream_request, RpTcpConnPoolConnectionData* upstream)
{
    LOGD("(%p, %p)", upstream_request, upstream);
    g_return_val_if_fail(RP_IS_UPSTREAM_TO_DOWNSTREAM(upstream_request), NULL);
    g_return_val_if_fail(RP_IS_TCP_CONN_POOL_CONNECTION_DATA(upstream), NULL);
    return g_object_new(RP_TYPE_PER_HOST_TCP_UPSTREAM,
                        "upstream-request", upstream_request,
                        "upstream", upstream,
                        NULL);
}
