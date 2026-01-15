/*
 * rp-router.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <evhtp.h>
#include "rp-conn-pool.h"
#include "rp-codec.h"
#include "rp-http-conn-pool.h"
#include "rp-socket.h"
#include "rp-stream-info.h"
#include "rp-host-description.h"
#include "rp-resource-manager.h"

G_BEGIN_DECLS

typedef struct _RpHost RpHost;
typedef struct _RpLoadBalancerContext RpLoadBalancerContext;
typedef struct _RpThreadLocalCluster RpThreadLocalCluster;
typedef struct _RpRouteConfig RpRouteConfig;
typedef struct _RpRoute RpRoute;

/**
 * RouteCallback, returns one of these enums to the route matcher to indicate
 * if the matched route has been accepted or it wants the route matching to
 * continue.
 */
typedef enum {
    RpRouteMatchStatus_Continue,
    RpRouteMatchStatus_Accept
} RpRouteMatchStatus_e;

/**
 * RouteCallback is passed this enum to indicate if more routes are available for evaluation.
 */
typedef enum {
    RpRouteEvalStatus_HasMoreRoutes,
    RpRouteEvalStatus_NoMoreRoutes
} RpRouteEvalStatus_e;

/*
 * Protocol used by the upstream sockets.
 */
typedef enum {
    RpUpstreamProtocol_HTTP,
    RpUpstreamProtocol_TCP,
    RpUpstreamProtocol_UDP
} RpUpstreamProtocol_e;


typedef RpRouteMatchStatus_e (*RpRouteCallback)(RpRouteConfig*, RpRoute*, RpRouteEvalStatus_e);


/**
 * Shared part of the route configuration. This class contains interfaces that needn't depend on
 * router matcher. Then every virtualhost could keep a reference to the CommonConfig. When the
 * entire route config is destroyed, the part of CommonConfig will still live until all
 * virtual hosts are destroyed.
 */
#define RP_TYPE_ROUTE_COMMON_CONFIG rp_route_common_config_get_type()
G_DECLARE_INTERFACE(RpRouteCommonConfig, rp_route_common_config, RP, ROUTE_COMMON_CONFIG, GObject)

struct _RpRouteCommonConfigInterface {
    GTypeInterface parent_iface;

    const char* (*name)(RpRouteCommonConfig*);
    guint32 (*max_direct_response_body_size_bytes)(RpRouteCommonConfig*);
};

static inline const char*
rp_route_common_config_name(RpRouteCommonConfig* self)
{
    return RP_IS_ROUTE_COMMON_CONFIG(self) ?
        RP_ROUTE_COMMON_CONFIG_GET_IFACE(self)->name(self) : NULL;
}
static inline guint32
rp_route_common_config_max_direct_response_body_size_bytes(RpRouteCommonConfig* self)
{
    return RP_IS_ROUTE_COMMON_CONFIG(self) ?
        RP_ROUTE_COMMON_CONFIG_GET_IFACE(self)->max_direct_response_body_size_bytes(self) :
        0;
}


/**
 * The router configuration.
 */
#define RP_TYPE_ROUTE_CONFIG rp_route_config_get_type()
G_DECLARE_INTERFACE(RpRouteConfig, rp_route_config, RP, ROUTE_CONFIG, RpRouteCommonConfig)

struct _RpRouteConfigInterface {
    RpRouteCommonConfigInterface parent_iface;

    RpRoute* (*route)(RpRouteConfig*, RpRouteCallback, evhtp_headers_t*, RpStreamInfo*, guint64);
};

static inline RpRoute*
rp_route_config_route(RpRouteConfig* self, RpRouteCallback cb, evhtp_headers_t* request_headers, RpStreamInfo* stream_info, guint64 random_value)
{
    return RP_IS_ROUTE_CONFIG(self) ?
        RP_ROUTE_CONFIG_GET_IFACE(self)->route(self, cb, request_headers, stream_info, random_value) :
        NULL;
}


/**
 * Functionality common among routing primitives, such as DirectResponseEntry and RouteEntry.
 */
#define RP_TYPE_RESPONSE_ENTRY rp_response_entry_get_type()
G_DECLARE_INTERFACE(RpResponseEntry, rp_response_entry, RP, RESPONSE_ENTRY, GObject)

struct _RpResponseEntryInterface {
    GTypeInterface parent_iface;

    void (*finalize_response_headers)(RpResponseEntry*,
                                        evhtp_headers_t*,
                                        RpStreamInfo*);
    //TODO...
};

static inline void
rp_response_entry_finalize_response_headers(RpResponseEntry* self, evhtp_headers_t* response_headers, RpStreamInfo* stream_info)
{
    if (RP_IS_RESPONSE_ENTRY(self)) \
        RP_RESPONSE_ENTRY_GET_IFACE(self)->finalize_response_headers(self, response_headers, stream_info);
}


/**
 * A routing primitive that specifies a direct (non-proxied) HTTP response.
 */
#define RP_TYPE_DIRECT_RESPONSE_ENTRY rp_direct_response_entry_get_type()
G_DECLARE_INTERFACE(RpDirectResponseEntry, rp_direct_response_entry, RP, DIRECT_RESPONSE_ENTRY, RpResponseEntry)

struct _RpDirectResponseEntryInterface {
    RpResponseEntryInterface parent_iface;

    evhtp_res (*response_code)(RpDirectResponseEntry*);
    char* (*new_uri)(RpDirectResponseEntry*, evhtp_headers_t*);
    void (*rewrite_path_header)(RpDirectResponseEntry*, evhtp_headers_t*, bool);
};


/**
 * Virtual host definition.
 */
#define RP_TYPE_VIRTUAL_HOST rp_virtual_host_get_type()
G_DECLARE_INTERFACE(RpVirtualHost, rp_virtual_host, RP, VIRTUAL_HOST, GObject)

struct _RpVirtualHostInterface {
    GTypeInterface parent_iface;

    const char* (*name)(RpVirtualHost*);
    RpRouteCommonConfig* (*route_config)(RpVirtualHost*);
};

static inline const char*
rp_virtual_host_name(RpVirtualHost* self)
{
    return RP_IS_VIRTUAL_HOST(self) ?
        RP_VIRTUAL_HOST_GET_IFACE(self)->name(self) : NULL;
}
static inline RpRouteCommonConfig*
rp_virtual_host_route_config(RpVirtualHost* self)
{
    return RP_IS_VIRTUAL_HOST(self) ?
        RP_VIRTUAL_HOST_GET_IFACE(self)->route_config(self) : NULL;
}


/**
 * An individual resolved route entry.
 */
#define RP_TYPE_ROUTE_ENTRY rp_route_entry_get_type()
G_DECLARE_INTERFACE(RpRouteEntry, rp_route_entry, RP, ROUTE_ENTRY, RpResponseEntry)

struct _RpRouteEntryInterface {
    RpResponseEntryInterface parent_iface;

    const char* (*cluster_name)(RpRouteEntry*);
    char* (*get_request_host_value)(RpRouteEntry*, evhtp_headers_t*);
    //TODO...
    char* (*current_url_path_after_rewrite)(RpRouteEntry*, evhtp_headers_t*);
    void (*finalize_request_headers)(RpRouteEntry*, evhtp_headers_t*, RpStreamInfo*, bool);
    //TODO...
    RpResourcePriority_e (*priority)(RpRouteEntry*);
    //TODO...
    bool (*append_xfh)(RpRouteEntry*);
    //TODO...
};

static inline const char*
rp_route_entry_cluster_name(RpRouteEntry* self)
{
    return RP_IS_ROUTE_ENTRY(self) ?
        RP_ROUTE_ENTRY_GET_IFACE(self)->cluster_name(self) : NULL;
}
static inline char*
rp_route_entry_get_request_host_value(RpRouteEntry* self, evhtp_headers_t* request_headers)
{
    return RP_IS_ROUTE_ENTRY(self) ?
        RP_ROUTE_ENTRY_GET_IFACE(self)->get_request_host_value(self, request_headers) :
        NULL;
}
static inline char*
rp_route_entry_current_url_path_after_rewrite(RpRouteEntry* self, evhtp_headers_t* request_headers)
{
    return RP_IS_ROUTE_ENTRY(self) ?
        RP_ROUTE_ENTRY_GET_IFACE(self)->current_url_path_after_rewrite(self, request_headers) :
        NULL;
}
static inline void
rp_route_entry_finalize_request_headers(RpRouteEntry* self, evhtp_headers_t* request_headers, RpStreamInfo* stream_info, bool insert_original_path)
{
    if (RP_IS_ROUTE_ENTRY(self))
    {
        RP_ROUTE_ENTRY_GET_IFACE(self)->finalize_request_headers(self, request_headers, stream_info, insert_original_path);
    }
}
static inline RpResourcePriority_e
rp_route_entry_priority(RpRouteEntry* self)
{
    return RP_IS_ROUTE_ENTRY(self) ?
        RP_ROUTE_ENTRY_GET_IFACE(self)->priority(self) :
        RpResourcePriority_Default;
}
static inline bool
rp_route_entry_append_xfh(RpRouteEntry* self)
{
    return RP_IS_ROUTE_ENTRY(self) ?
        RP_ROUTE_ENTRY_GET_IFACE(self)->append_xfh(self) : false;
}


/**
 * An interface that holds a DirectResponseEntry or RouteEntry for a request.
 */
#define RP_TYPE_ROUTE rp_route_get_type()
G_DECLARE_INTERFACE(RpRoute, rp_route, RP, ROUTE, GObject)

struct _RpRouteInterface {
    GTypeInterface parent_iface;

    RpDirectResponseEntry* (*direct_response_entry)(RpRoute*);
    RpRouteEntry* (*route_entry)(RpRoute*);
    //TODO...
    const char* (*route_name)(RpRoute*);
    //TODO...
};

typedef SHARED_PTR(RpRoute) RpRouteConstSharedPtr;

static inline RpDirectResponseEntry*
rp_route_direct_response_entry(RpRoute* self)
{
    return RP_IS_ROUTE(self) ?
        RP_ROUTE_GET_IFACE(self)->direct_response_entry(self) : NULL;
}
static inline RpRouteEntry*
rp_route_route_entry(RpRoute* self)
{
    return RP_IS_ROUTE(self) ?
        RP_ROUTE_GET_IFACE(self)->route_entry(self) : NULL;
}
static inline const char*
rp_route_route_name(RpRoute* self)
{
    return RP_IS_ROUTE(self) ?
        RP_ROUTE_GET_IFACE(self)->route_name(self) : NULL;
}

/**
 * An API for sending information to either a TCP, UDP, or HTTP upstream.
 *
 * It is similar logically to RequestEncoder, only without the getStream interface.
 */
#define RP_TYPE_GENERIC_UPSTREAM rp_generic_upstream_get_type()
G_DECLARE_INTERFACE(RpGenericUpstream, rp_generic_upstream, RP, GENERIC_UPSTREAM, GObject)

struct _RpGenericUpstreamInterface {
    GTypeInterface parent_iface;

    void (*encode_data)(RpGenericUpstream*, evbuf_t*, bool);
    //TODO...
    RpStatusCode_e (*encode_headers)(RpGenericUpstream*, evhtp_headers_t*, bool);
    void (*encode_trailers)(RpGenericUpstream*, evhtp_headers_t*);
    void (*enable_tcp_tunneling)(RpGenericUpstream*);
    void (*read_disable)(RpGenericUpstream*, bool);
    void (*reset_stream)(RpGenericUpstream*);
    //TODO...
    //TODO...StreamInfo::BytesMeterSharedPtr& bytesMeter() PURE;
};

static inline void
rp_generic_upstream_encode_data(RpGenericUpstream* self, evbuf_t* data, bool end_stream)
{
    if (RP_IS_GENERIC_UPSTREAM(self)) \
        RP_GENERIC_UPSTREAM_GET_IFACE(self)->encode_data(self, data, end_stream);
}
static inline RpStatusCode_e
rp_generic_upstream_encode_headers(RpGenericUpstream* self, evhtp_headers_t* request_headers, bool end_stream)
{
    return RP_IS_GENERIC_UPSTREAM(self) ?
        RP_GENERIC_UPSTREAM_GET_IFACE(self)->encode_headers(self, request_headers, end_stream) :
        RpStatusCode_Ok;
}
static inline void
rp_generic_upstream_encode_trailers(RpGenericUpstream* self, evhtp_headers_t* trailers)
{
    if (RP_IS_GENERIC_UPSTREAM(self)) \
        RP_GENERIC_UPSTREAM_GET_IFACE(self)->encode_trailers(self, trailers);
}
static inline void
rp_generic_upstream_enable_tcp_tunneling(RpGenericUpstream* self)
{
    if (RP_IS_GENERIC_UPSTREAM(self)) \
        RP_GENERIC_UPSTREAM_GET_IFACE(self)->enable_tcp_tunneling(self);
}
static inline void rp_generic_upstream_read_disable(RpGenericUpstream* self, bool disable)
{
    if (RP_IS_GENERIC_UPSTREAM(self)) \
        RP_GENERIC_UPSTREAM_GET_IFACE(self)->read_disable(self, disable);
}
static inline void rp_generic_upstream_reset_stream(RpGenericUpstream* self)
{
    if (RP_IS_GENERIC_UPSTREAM(self)) \
        RP_GENERIC_UPSTREAM_GET_IFACE(self)->reset_stream(self);
}


/**
 * An API for the interactions the upstream stream needs to have with the downstream stream
 * and/or router components
 */
#define RP_TYPE_UPSTREAM_TO_DOWNSTREAM rp_upstream_to_downstream_get_type()
G_DECLARE_INTERFACE(RpUpstreamToDownstream, rp_upstream_to_downstream, RP, UPSTREAM_TO_DOWNSTREAM, GObject)

struct _RpUpstreamToDownstreamInterface {
    GTypeInterface/*Http::ResponseDecoder, Http::StreamCallbacks*/ parent_iface;

    RpRoute* (*route)(RpUpstreamToDownstream*);
    RpNetworkConnection* (*connection)(RpUpstreamToDownstream*);
    RpHttpConnPoolInstStreamOptions (*upstream_stream_options)(RpUpstreamToDownstream*);
};

static inline RpRoute*
rp_upstream_to_downstream_route(RpUpstreamToDownstream* self)
{
    return RP_IS_UPSTREAM_TO_DOWNSTREAM(self) ?
        RP_UPSTREAM_TO_DOWNSTREAM_GET_IFACE(self)->route(self) : NULL;
}
static inline RpNetworkConnection*
rp_upstream_to_downstream_connection(RpUpstreamToDownstream* self)
{
    return RP_IS_UPSTREAM_TO_DOWNSTREAM(self) ?
        RP_UPSTREAM_TO_DOWNSTREAM_GET_IFACE(self)->connection(self) : NULL;
}
static inline RpHttpConnPoolInstStreamOptions
rp_upstream_to_downstream_upstream_stream_options(RpUpstreamToDownstream* self)
{
    return RP_IS_UPSTREAM_TO_DOWNSTREAM(self) ?
        RP_UPSTREAM_TO_DOWNSTREAM_GET_IFACE(self)->upstream_stream_options(self) :
        rp_http_conn_pool_inst_stream_options_ctor(false, false);
}


/**
 * An API for wrapping callbacks from either an HTTP, TCP, or UDP connection pool.
 *
 * Just like the connection pool callbacks, the GenericConnectionPoolCallbacks
 * will either call onPoolReady when a GenericUpstream is ready, or
 * onPoolFailure if a connection/stream can not be established.
 */
#define RP_TYPE_GENERIC_CONNECTION_POOL_CALLBACKS rp_generic_connection_pool_callbacks_get_type()
G_DECLARE_INTERFACE(RpGenericConnectionPoolCallbacks, rp_generic_connection_pool_callbacks, RP, GENERIC_CONNECTION_POOL_CALLBACKS, GObject)

struct _RpGenericConnectionPoolCallbacksInterface {
    GTypeInterface parent_iface;

    void (*on_pool_failure)(RpGenericConnectionPoolCallbacks*,
                            RpPoolFailureReason_e,
                            const char*,
                            RpHostDescription*);
    void (*on_pool_ready)(RpGenericConnectionPoolCallbacks*,
                            RpGenericUpstream*,
                            RpHostDescription*,
                            RpConnectionInfoProvider*,
                            RpStreamInfo*,
                            evhtp_proto);
    RpUpstreamToDownstream* (*upstream_to_downstream)(RpGenericConnectionPoolCallbacks*);
};

static inline void
rp_generic_connection_pool_callbacks_on_pool_failure(RpGenericConnectionPoolCallbacks* self, RpPoolFailureReason_e reason,
                                                        const char* transport_failure_reason, RpHostDescription* host)
{
    if (RP_IS_GENERIC_CONNECTION_POOL_CALLBACKS(self))
    {
        RP_GENERIC_CONNECTION_POOL_CALLBACKS_GET_IFACE(self)->on_pool_failure(self, reason, transport_failure_reason, host);
    }
}
static inline void
rp_generic_connection_pool_callbacks_on_pool_ready(RpGenericConnectionPoolCallbacks* self, RpGenericUpstream* upstream,
                                                    RpHostDescription* host, RpConnectionInfoProvider* provider,
                                                    RpStreamInfo* info, evhtp_proto protocol)
{
    if (RP_IS_GENERIC_CONNECTION_POOL_CALLBACKS(self))
    {
        RP_GENERIC_CONNECTION_POOL_CALLBACKS_GET_IFACE(self)->on_pool_ready(self, upstream, host, provider, info, protocol);
    }
}
static inline RpUpstreamToDownstream*
rp_generic_connection_pool_callbacks_upstream_to_downstream(RpGenericConnectionPoolCallbacks* self)
{
    return RP_IS_GENERIC_CONNECTION_POOL_CALLBACKS(self) ?
        RP_GENERIC_CONNECTION_POOL_CALLBACKS_GET_IFACE(self)->upstream_to_downstream(self) :
        NULL;
}


/**
 * An API for wrapping either an HTTP, TCP, or UDP connection pool.
 *
 * The GenericConnPool exists to create a GenericUpstream handle via a call to
 * newStream resulting in an eventual call to onPoolReady
 */
#define RP_TYPE_GENERIC_CONN_POOL rp_generic_conn_pool_get_type()
G_DECLARE_INTERFACE(RpGenericConnPool, rp_generic_conn_pool, RP, GENERIC_CONN_POOL, GObject)

struct _RpGenericConnPoolInterface {
    GTypeInterface parent_iface;

    void (*new_stream)(RpGenericConnPool*, RpGenericConnectionPoolCallbacks*);
    bool (*cancel_any_pending_stream)(RpGenericConnPool*);
    RpHostDescription* (*host)(RpGenericConnPool*);
    bool (*valid)(RpGenericConnPool*);
};

static inline void
rp_generic_conn_pool_new_stream(RpGenericConnPool* self, RpGenericConnectionPoolCallbacks* callbacks)
{
    if (RP_IS_GENERIC_CONN_POOL(self))
    {
        RP_GENERIC_CONN_POOL_GET_IFACE(self)->new_stream(self, callbacks);
    }
}
static inline bool
rp_generic_conn_pool_cancel_any_pending_stream(RpGenericConnPool* self)
{
    return RP_IS_GENERIC_CONN_POOL(self) ?
        RP_GENERIC_CONN_POOL_GET_IFACE(self)->cancel_any_pending_stream(self) : false;
}
static inline RpHostDescription*
rp_generic_conn_pool_host(RpGenericConnPool* self)
{
    return RP_IS_GENERIC_CONN_POOL(self) ?
        RP_GENERIC_CONN_POOL_GET_IFACE(self)->host(self) : NULL;
}
static inline bool
rp_generic_conn_pool_valid(RpGenericConnPool* self)
{
    return RP_IS_GENERIC_CONN_POOL(self) ?
        RP_GENERIC_CONN_POOL_GET_IFACE(self)->valid(self) : false;
}


/*
 * A factory for creating generic connection pools.
 */
#define RP_TYPE_GENERIC_CONN_POOL_FACTORY rp_generic_conn_pool_factory_get_type()
G_DECLARE_INTERFACE(RpGenericConnPoolFactory, rp_generic_conn_pool_factory, RP, GENERIC_CONN_POOL_FACTORY, GObject)

struct _RpGenericConnPoolFactoryInterface {
    GTypeInterface parent_iface;

    RpGenericConnPool* (*create_generic_conn_pool)(RpGenericConnPoolFactory*,
                                                    RpHostConstSharedPtr,
                                                    RpThreadLocalCluster*,
                                                    RpUpstreamProtocol_e,
                                                    RpResourcePriority_e,
                                                    evhtp_proto,
                                                    RpLoadBalancerContext*
                                                    /*Protobuf::Message*/);
};

typedef UNIQUE_PTR(RpGenericConnPoolFactory) RpGenericConnPoolFactoryPtr;

static inline RpGenericConnPool*
rp_generic_conn_pool_factory_create_generic_conn_pool(RpGenericConnPoolFactory* self,
                                                        RpHostConstSharedPtr host,
                                                        RpThreadLocalCluster* thread_local_cluster,
                                                        RpUpstreamProtocol_e upstream_protocol,
                                                        RpResourcePriority_e priority,
                                                        evhtp_proto downstream_protocol,
                                                        RpLoadBalancerContext* context)
{
    return RP_IS_GENERIC_CONN_POOL_FACTORY(self) ?
        RP_GENERIC_CONN_POOL_FACTORY_GET_IFACE(self)->create_generic_conn_pool(self, host, thread_local_cluster, upstream_protocol, priority, downstream_protocol, context) :
        NULL;
}

G_END_DECLS
