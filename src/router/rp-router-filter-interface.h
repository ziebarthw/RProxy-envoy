/*
 * rp-router-filter-interface.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-http-filter.h"
#include "rp-host-description.h"
#include "rp-cluster-manager.h"

G_BEGIN_DECLS

typedef struct _RpUpstreamRequest RpUpstreamRequest;
typedef struct _RpRouterFilterConfig RpRouterFilterConfig;

/*
 * The interface the UpstreamRequest has to interact with the router filter.
 */
#define RP_TYPE_ROUTER_FILTER_INTERFACE rp_router_filter_interface_get_type()
G_DECLARE_INTERFACE(RpRouterFilterInterface, rp_router_filter_interface, RP, ROUTER_FILTER_INTERFACE, GObject)

struct _RpRouterFilterInterfaceInterface {
    GTypeInterface parent_iface;

    void (*on_upstream_1xx_headers)(RpRouterFilterInterface*, evhtp_headers_t*, RpUpstreamRequest*);
    void (*on_upstream_headers)(RpRouterFilterInterface*, guint64, evhtp_headers_t*, RpUpstreamRequest*, bool);
    void (*on_upstream_data)(RpRouterFilterInterface*, evbuf_t*, RpUpstreamRequest*, bool);
    void (*on_upstream_trailers)(RpRouterFilterInterface*, evhtp_headers_t*, RpUpstreamRequest*);
    void (*on_upstream_reset)(RpRouterFilterInterface*, RpStreamResetReason_e, const char*, RpUpstreamRequest*);
    void (*on_upstream_host_selected)(RpRouterFilterInterface*, RpHostDescription*, bool);
    void (*on_per_try_timeout)(RpRouterFilterInterface*, RpUpstreamRequest*);
    void (*on_per_try_idle_timeout)(RpRouterFilterInterface*, RpUpstreamRequest*);
    void (*on_stream_max_duration_reached)(RpRouterFilterInterface*, RpUpstreamRequest*);
    RpStreamDecoderFilterCallbacks* (*callbacks)(RpRouterFilterInterface*);
    RpClusterInfoConstSharedPtr (*cluster)(RpRouterFilterInterface*);
    RpRouterFilterConfig* (*config)(RpRouterFilterInterface*);
    //TODO...
    evhtp_headers_t* (*downstream_headers)(RpRouterFilterInterface*);
    evhtp_headers_t* (*downstream_trailers)(RpRouterFilterInterface*);
    bool (*downstream_response_started)(RpRouterFilterInterface*);
    bool (*downstream_end_stream)(RpRouterFilterInterface*);
    guint32 (*attempt_count)(RpRouterFilterInterface*);
};

static inline void
rp_router_filter_interface_on_upstream_1xx_headers(RpRouterFilterInterface* self, evhtp_headers_t* response_headers, RpUpstreamRequest* upstream_request)
{
    if (RP_IS_ROUTER_FILTER_INTERFACE(self))
    {
        RP_ROUTER_FILTER_INTERFACE_GET_IFACE(self)->on_upstream_1xx_headers(self, response_headers, upstream_request);
    }
}
static inline void
rp_router_filter_interface_on_upstream_headers(RpRouterFilterInterface* self, guint64 response_code,
                                                evhtp_headers_t* response_headers, RpUpstreamRequest* upstream_request, bool end_stream)
{
    if (RP_IS_ROUTER_FILTER_INTERFACE(self))
    {
        RP_ROUTER_FILTER_INTERFACE_GET_IFACE(self)->on_upstream_headers(self, response_code, response_headers, upstream_request, end_stream);
    }
}
static inline void
rp_router_filter_interface_on_upstream_data(RpRouterFilterInterface* self, evbuf_t* data,
                                            RpUpstreamRequest* upstream_request, bool end_stream)
{
    if (RP_IS_ROUTER_FILTER_INTERFACE(self))
    {
        RP_ROUTER_FILTER_INTERFACE_GET_IFACE(self)->on_upstream_data(self, data, upstream_request, end_stream);
    }
}
static inline void
rp_router_filter_interface_on_upstream_trailers(RpRouterFilterInterface* self, evhtp_headers_t* trailers, RpUpstreamRequest* upstream_request)
{
    if (RP_IS_ROUTER_FILTER_INTERFACE(self))
    {
        RP_ROUTER_FILTER_INTERFACE_GET_IFACE(self)->on_upstream_trailers(self, trailers, upstream_request);
    }
}
static inline void
rp_router_filter_interface_on_upstream_reset(RpRouterFilterInterface* self, RpStreamResetReason_e reset_reason,
                                                const char* transport_failure, RpUpstreamRequest* upstream_request)
{
    if (RP_IS_ROUTER_FILTER_INTERFACE(self))
    {
        RP_ROUTER_FILTER_INTERFACE_GET_IFACE(self)->on_upstream_reset(self, reset_reason, transport_failure, upstream_request);
    }
}
static inline void
rp_router_filter_interface_on_upstream_host_selected(RpRouterFilterInterface* self, RpHostDescription* host, bool pool_success)
{
    if (RP_IS_ROUTER_FILTER_INTERFACE(self))
    {
        RP_ROUTER_FILTER_INTERFACE_GET_IFACE(self)->on_upstream_host_selected(self, host, pool_success);
    }
}
static inline void
rp_router_filter_interface_on_per_try_timeout(RpRouterFilterInterface* self, RpUpstreamRequest* upstream_request)
{
    if (RP_IS_ROUTER_FILTER_INTERFACE(self))
    {
        RP_ROUTER_FILTER_INTERFACE_GET_IFACE(self)->on_per_try_timeout(self, upstream_request);
    }
}
static inline void
rp_router_filter_interface_on_per_try_idle_timeout(RpRouterFilterInterface* self, RpUpstreamRequest* upstream_request)
{
    if (RP_IS_ROUTER_FILTER_INTERFACE(self))
    {
        RP_ROUTER_FILTER_INTERFACE_GET_IFACE(self)->on_per_try_idle_timeout(self, upstream_request);
    }
}
static inline void
rp_router_filter_interface_on_stream_max_duration_reached(RpRouterFilterInterface* self, RpUpstreamRequest* upstream_request)
{
    if (RP_IS_ROUTER_FILTER_INTERFACE(self))
    {
        RP_ROUTER_FILTER_INTERFACE_GET_IFACE(self)->on_stream_max_duration_reached(self, upstream_request);
    }
}
static inline RpStreamDecoderFilterCallbacks*
rp_router_filter_interface_callbacks(RpRouterFilterInterface* self)
{
    return RP_IS_ROUTER_FILTER_INTERFACE(self) ?
        RP_ROUTER_FILTER_INTERFACE_GET_IFACE(self)->callbacks(self) : NULL;
}
static inline RpClusterInfoConstSharedPtr
rp_router_filter_interface_cluster(RpRouterFilterInterface* self)
{
    return RP_IS_ROUTER_FILTER_INTERFACE(self) ?
        RP_ROUTER_FILTER_INTERFACE_GET_IFACE(self)->cluster(self) : NULL;
}
static inline RpRouterFilterConfig*
rp_router_filter_interface_config(RpRouterFilterInterface* self)
{
    return RP_IS_ROUTER_FILTER_INTERFACE(self) ?
        RP_ROUTER_FILTER_INTERFACE_GET_IFACE(self)->config(self) : NULL;
}
static inline evhtp_headers_t*
rp_router_filter_interface_downstream_headers(RpRouterFilterInterface* self)
{
    return RP_IS_ROUTER_FILTER_INTERFACE(self) ?
        RP_ROUTER_FILTER_INTERFACE_GET_IFACE(self)->downstream_headers(self) :
        NULL;
}
static inline evhtp_headers_t*
rp_router_filter_interface_downstream_trailers(RpRouterFilterInterface* self)
{
    return RP_IS_ROUTER_FILTER_INTERFACE(self) ?
        RP_ROUTER_FILTER_INTERFACE_GET_IFACE(self)->downstream_trailers(self) :
        NULL;
}
static inline bool
rp_router_filter_interface_downstream_response_started(RpRouterFilterInterface* self)
{
    return RP_IS_ROUTER_FILTER_INTERFACE(self) ?
        RP_ROUTER_FILTER_INTERFACE_GET_IFACE(self)->downstream_response_started(self) :
        false;
}
static inline bool
rp_router_filter_interface_downstream_end_stream(RpRouterFilterInterface* self)
{
    return RP_IS_ROUTER_FILTER_INTERFACE(self) ?
        RP_ROUTER_FILTER_INTERFACE_GET_IFACE(self)->downstream_end_stream(self) :
        true;
}
static inline guint32
rp_router_filter_interface_attempt_count(RpRouterFilterInterface* self)
{
    return RP_IS_ROUTER_FILTER_INTERFACE(self) ?
        RP_ROUTER_FILTER_INTERFACE_GET_IFACE(self)->attempt_count(self) : 0;
}

G_END_DECLS
