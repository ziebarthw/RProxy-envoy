/*
 * rp-http-filter.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 * https://github.com/envoyproxy/envoy/blob/main/envoy/http/filter.h
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <evhtp.h>
#include "rp-dispatcher.h"
#include "rp-load-balancer.h"
#include "rp-net-connection.h"
#include "rp-router.h"
#include "rp-stream-reset-handler.h"

G_BEGIN_DECLS

typedef struct _RpRoute RpRoute;
typedef struct _RpStreamInfo RpStreamInfo;
typedef struct _RpFilterManager RpFilterManager;

/**
 * Return codes for encode/decode headers filter invocations. The connection manager bases further
 * filter invocations on the return code of the previous filter.
 */
typedef enum {
    RpFilterHeadersStatus_Continue,
    RpFilterHeadersStatus_StopIteration,
    RpFilterHeadersStatus_ContinueAndDontEndStream,
    RpFilterHeadersStatus_StopAllIterationAndBuffer,
    RpFilterHeadersStatus_StopAllIterationAndWatermark
} RpFilterHeadersStatus_e;

/**
 * Return codes for encode/decode data filter invocations. The connection manager bases further
 * filter invocations on the return code of the previous filter.
 */
typedef enum {
    RpFilterDataStatus_Continue,
    RpFilterDataStatus_StopIterationAndBuffer,
    RpFilterDataStatus_StopIterationAndWatermark,
    RpFilterDataStatus_StopIterationNoBuffer
} RpFilterDataStatus_e;

/**
 * Return codes for encode/decode trailers filter invocations. The connection manager bases further
 * filter invocations on the return code of the previous filter.
 */
typedef enum {
    RpFilterTrailerStatus_Continue,
    RpFilterTrailerStatus_StopIteration
} RpFilterTrailerStatus_e;

/**
 * Return codes for onLocalReply filter invocations.
 */
typedef enum {
    RpLocalErrorStatus_Continue,
    RpLocalErrorStatus_ContinueAndResetStream
} RpLocalErrorStatus_e;

typedef void (*modify_headers_cb)(evhtp_headers_t* response_headers, void* arg);
typedef void (*rewrite_cb)(evhtp_headers_t* response_headers,
                            evhtp_res response_code,
                            evbuf_t* response_body,
                            const char* details,
                            void* arg);
typedef void (*encode_headers_cb)(evhtp_headers_t* response_headers,
                                    bool end_stream,
                                    void* arg);
typedef void (*encode_data_cb)(evbuf_t* data, bool end_stream, void* arg);

// These are events that upstream HTTP filters can register for, via the addUpstreamCallbacks
// function.
#define RP_TYPE_UPSTREAM_CALLBACKS rp_upstream_callbacks_get_type()
G_DECLARE_INTERFACE(RpUpstreamCallbacks, rp_upstream_callbacks, RP, UPSTREAM_CALLBACKS, GObject)

struct _RpUpstreamCallbacksInterface {
    GTypeInterface parent_iface;

    void (*on_upstream_connection_established)(RpUpstreamCallbacks*);
};

static inline void
rp_upstream_callbacks_on_upstream_connection_established(RpUpstreamCallbacks* self)
{
    if (RP_IS_UPSTREAM_CALLBACKS(self)) \
        RP_UPSTREAM_CALLBACKS_GET_IFACE(self)->on_upstream_connection_established(self);
}

// These are filter callbacks specific to upstream HTTP filters, accessible via
// StreamFilterCallbacks::upstreamCallbacks()
#define RP_TYPE_UPSTREAM_STREAM_FILTER_CALLBACKS rp_upstream_stream_filter_callbacks_get_type()
G_DECLARE_INTERFACE(RpUpstreamStreamFilterCallbacks, rp_upstream_stream_filter_callbacks, RP, UPSTREAM_STREAM_FILTER_CALLBACKS, GObject)

struct _RpUpstreamStreamFilterCallbacksInterface {
    GTypeInterface parent_iface;

    RpStreamInfo* (*upstream_stream_info)(RpUpstreamStreamFilterCallbacks*);
    RpGenericUpstream* (*upstream)(RpUpstreamStreamFilterCallbacks*);
    bool (*paused_for_connect)(RpUpstreamStreamFilterCallbacks*);
    void (*set_paused_for_connect)(RpUpstreamStreamFilterCallbacks*, bool);
    bool (*paused_for_web_socket_upgrade)(RpUpstreamStreamFilterCallbacks*);
    void (*set_paused_for_web_socket_upgrade)(RpUpstreamStreamFilterCallbacks*, bool);
    RpHttpConnPoolInstStreamOptions (*upstream_stream_options)(RpUpstreamStreamFilterCallbacks*);
    void (*add_upstream_callbacks)(RpUpstreamStreamFilterCallbacks*, RpUpstreamCallbacks*);
    void (*set_upstream_to_downstream)(RpUpstreamStreamFilterCallbacks*,
                                        RpUpstreamToDownstream*);
};

static inline RpStreamInfo*
rp_upstream_stream_filter_callbacks_upstream_stream_info(RpUpstreamStreamFilterCallbacks* self)
{
    return RP_IS_UPSTREAM_STREAM_FILTER_CALLBACKS(self) ?
        RP_UPSTREAM_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->upstream_stream_info(self) :
        NULL;
}
static inline RpGenericUpstream*
rp_upstream_stream_filter_callbacks_upstream(RpUpstreamStreamFilterCallbacks* self)
{
    return RP_IS_UPSTREAM_STREAM_FILTER_CALLBACKS(self) ?
        RP_UPSTREAM_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->upstream(self) :
        NULL;
}
static inline bool
rp_upstream_stream_filter_callbacks_paused_for_connect(RpUpstreamStreamFilterCallbacks* self)
{
    return RP_IS_UPSTREAM_STREAM_FILTER_CALLBACKS(self) ?
        RP_UPSTREAM_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->paused_for_connect(self) :
        false;
}
static inline void
rp_upstream_stream_filter_callbacks_set_paused_for_connect(RpUpstreamStreamFilterCallbacks* self, bool value)
{
    if (RP_IS_UPSTREAM_STREAM_FILTER_CALLBACKS(self))
    {
        RP_UPSTREAM_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->set_paused_for_connect(self, value);
    }
}
static inline bool
rp_upstream_stream_filter_callbacks_paused_for_web_socket_upgrade(RpUpstreamStreamFilterCallbacks* self)
{
    return RP_IS_UPSTREAM_STREAM_FILTER_CALLBACKS(self) ?
        RP_UPSTREAM_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->paused_for_web_socket_upgrade(self) :
        false;
}
static inline void
rp_upstream_stream_filter_callbacks_set_paused_for_web_socket_upgrade(RpUpstreamStreamFilterCallbacks* self, bool value)
{
    if (RP_IS_UPSTREAM_STREAM_FILTER_CALLBACKS(self))
    {
        RP_UPSTREAM_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->set_paused_for_web_socket_upgrade(self, value);
    }
}
static inline RpHttpConnPoolInstStreamOptions
rp_upstream_stream_filter_callbacks_upstream_stream_options(RpUpstreamStreamFilterCallbacks* self)
{
    return RP_IS_UPSTREAM_STREAM_FILTER_CALLBACKS(self) ?
        RP_UPSTREAM_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->upstream_stream_options(self) :
        rp_http_conn_pool_inst_stream_options_ctor(false, false);
}
static inline void
rp_upstream_stream_filter_callbacks_add_upstream_callbacks(RpUpstreamStreamFilterCallbacks* self, RpUpstreamCallbacks* callbacks)
{
    if (RP_IS_UPSTREAM_STREAM_FILTER_CALLBACKS(self))
    {
        RP_UPSTREAM_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->add_upstream_callbacks(self, callbacks);
    }
}
static inline void
rp_upstream_stream_filter_callbacks_set_upstream_to_downstream(RpUpstreamStreamFilterCallbacks* self,
                                                                RpUpstreamToDownstream* upstream_to_downstream_interface)
{
    if (RP_IS_UPSTREAM_STREAM_FILTER_CALLBACKS(self))
    {
        RP_UPSTREAM_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->set_upstream_to_downstream(self, upstream_to_downstream_interface);
    }
}


/*
 * This class allows entities caching a route (e.g.) ActiveStream to delegate route clearing to xDS
 * components.
 */
#define RP_TYPE_ROUTE_CACHE rp_route_cache_get_type()
G_DECLARE_INTERFACE(RpRouteCache, rp_route_cache, RP, ROUTE_CACHE, GObject)

struct _RpRouteCacheInterface {
    GTypeInterface parent_iface;

    bool (*has_cached_route)(RpRouteCache*);
    void (*refresh_cached_route)(RpRouteCache*);
};

static inline bool
rp_route_cache_has_cached_route(RpRouteCache* self)
{
    return RP_IS_ROUTE_CACHE(self) ?
        RP_ROUTE_CACHE_GET_IFACE(self)->has_cached_route(self) : false;
}
static inline void
rp_route_cache_refresh_cached_route(RpRouteCache* self)
{
    if (RP_IS_ROUTE_CACHE(self)) \
        RP_ROUTE_CACHE_GET_IFACE(self)->refresh_cached_route(self);
}


#define RP_TYPE_DOWNSTREAM_STREAM_FILTER_CALLBACKS rp_downstream_stream_filter_callbacks_get_type()
G_DECLARE_INTERFACE(RpDownstreamStreamFilterCallbacks, rp_downstream_stream_filter_callbacks, RP, DOWNSTREAM_STREAM_FILTER_CALLBACKS, GObject)

struct _RpDownstreamStreamFilterCallbacksInterface {
    GTypeInterface parent_iface;

    void (*set_route)(RpDownstreamStreamFilterCallbacks*, RpRoute*);
    RpRoute* (*route)(RpDownstreamStreamFilterCallbacks*, RpRouteCallback);
    void (*clear_route_cache)(RpDownstreamStreamFilterCallbacks*);
};

static inline void
rp_downstream_stream_filter_callbacks_set_route(RpDownstreamStreamFilterCallbacks* self, RpRoute* route)
{
    if (RP_IS_DOWNSTREAM_STREAM_FILTER_CALLBACKS(self)) \
        RP_DOWNSTREAM_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->set_route(self, route);
}
static inline RpRoute*
rp_downstream_stream_filter_callbacks_route(RpDownstreamStreamFilterCallbacks* self, RpRouteCallback cb)
{
    return RP_IS_DOWNSTREAM_STREAM_FILTER_CALLBACKS(self) ?
        RP_DOWNSTREAM_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->route(self, cb) :
        NULL;
}
static inline void
rp_downstream_stream_filter_callbacks_clear_route_cache(RpDownstreamStreamFilterCallbacks* self)
{
    if (RP_IS_DOWNSTREAM_STREAM_FILTER_CALLBACKS(self)) \
        RP_DOWNSTREAM_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->clear_route_cache(self);
}


/**
 * The stream filter callbacks are passed to all filters to use for writing response data and
 * interacting with the underlying stream in general.
 */
#define RP_TYPE_STREAM_FILTER_CALLBACKS rp_stream_filter_callbacks_get_type()
G_DECLARE_INTERFACE(RpStreamFilterCallbacks, rp_stream_filter_callbacks, RP, STREAM_FILTER_CALLBACKS, GObject)

struct _RpStreamFilterCallbacksInterface {
    GTypeInterface parent_iface;

    RpNetworkConnection* (*connection)(RpStreamFilterCallbacks*);
    RpDispatcher* (*dispatcher)(RpStreamFilterCallbacks*);
    void (*reset_stream)(RpStreamFilterCallbacks*,
                            RpStreamResetReason_e,
                            const char*);
    RpRoute* (*route)(RpStreamFilterCallbacks*);
    RpClusterInfoConstSharedPtr (*cluster_info)(RpStreamFilterCallbacks*);
    guint64 (*stream_id)(RpStreamFilterCallbacks*);
    RpStreamInfo* (*stream_info)(RpStreamFilterCallbacks*);
    void (*reset_idle_timer)(RpStreamFilterCallbacks*);
    RpUpstreamStreamFilterCallbacks* (*upstream_callbacks)(RpStreamFilterCallbacks*);
    RpDownstreamStreamFilterCallbacks* (*downstream_callbacks)(RpStreamFilterCallbacks*);
    const char* (*filter_config_name)(RpStreamFilterCallbacks*);
    evhtp_headers_t* (*request_headers)(RpStreamFilterCallbacks*);
    evhtp_headers_t* (*request_trailers)(RpStreamFilterCallbacks*);
    evhtp_headers_t* (*response_headers)(RpStreamFilterCallbacks*);
    evhtp_headers_t* (*response_trailers)(RpStreamFilterCallbacks*);
};

static inline RpNetworkConnection*
rp_stream_filter_callbacks_connection(RpStreamFilterCallbacks* self)
{
    return RP_IS_STREAM_FILTER_CALLBACKS(self) ?
        RP_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->connection(self) : NULL;
}

static inline RpDispatcher*
rp_stream_filter_callbacks_dispatcher(RpStreamFilterCallbacks* self)
{
    return RP_IS_STREAM_FILTER_CALLBACKS(self) ?
        RP_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->dispatcher(self) : NULL;
}
static inline void
rp_stream_filter_callbacks_reset_stream(RpStreamFilterCallbacks* self, RpStreamResetReason_e reset_reason, const char* transport_failure_reason)
{
    if (RP_IS_STREAM_FILTER_CALLBACKS(self)) \
        RP_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->reset_stream(self, reset_reason, transport_failure_reason);
}
static inline RpRoute*
rp_stream_filter_callbacks_route(RpStreamFilterCallbacks* self)
{
    return RP_IS_STREAM_FILTER_CALLBACKS(self) ?
        RP_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->route(self) : NULL;
}
static inline RpClusterInfoConstSharedPtr
rp_stream_filter_callbacks_cluster_info(RpStreamFilterCallbacks* self)
{
    return RP_IS_STREAM_FILTER_CALLBACKS(self) ?
        RP_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->cluster_info(self) : NULL;
}
static inline guint64
rp_stream_filter_callbacks_stream_id(RpStreamFilterCallbacks* self)
{
    return RP_IS_STREAM_FILTER_CALLBACKS(self) ?
        RP_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->stream_id(self) : 0;
}
static inline RpStreamInfo*
rp_stream_filter_callbacks_stream_info(RpStreamFilterCallbacks* self)
{
    return RP_IS_STREAM_FILTER_CALLBACKS(self) ?
        RP_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->stream_info(self) : NULL;
}
const char* rp_stream_filter_callbacks_filter_config_name(RpStreamFilterCallbacks* self);
evhtp_headers_t* rp_stream_filter_callbacks_request_headers(RpStreamFilterCallbacks* self);
static inline evhtp_headers_t*
rp_stream_filter_callbacks_request_trailers(RpStreamFilterCallbacks* self)
{
    return RP_IS_STREAM_FILTER_CALLBACKS(self) ?
        RP_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->request_trailers(self) : NULL;
}
evhtp_headers_t* rp_stream_filter_callbacks_response_headers(RpStreamFilterCallbacks* self);
static inline evhtp_headers_t*
rp_stream_filter_callbacks_response_trailers(RpStreamFilterCallbacks* self)
{
    return RP_IS_STREAM_FILTER_CALLBACKS(self) ?
        RP_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->response_trailers(self) : NULL;
}
void rp_stream_filter_callbacks_reset_idle_timer(RpStreamFilterCallbacks* self);
static inline RpUpstreamStreamFilterCallbacks*
rp_stream_filter_callbacks_upstream_callbacks(RpStreamFilterCallbacks* self)
{
    return RP_IS_STREAM_FILTER_CALLBACKS(self) ?
        RP_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->upstream_callbacks(self) : NULL;
}
static inline RpDownstreamStreamFilterCallbacks*
rp_stream_filter_callbacks_downstream_callbacks(RpStreamFilterCallbacks* self)
{
    return RP_IS_STREAM_FILTER_CALLBACKS(self) ?
        RP_STREAM_FILTER_CALLBACKS_GET_IFACE(self)->downstream_callbacks(self) : NULL;
}


/**
 * Stream decoder filter callbacks add additional callbacks that allow a
 * decoding filter to restart decoding if they decide to hold data (e.g. for
 * buffering or rate limiting).
 */
#define RP_TYPE_STREAM_DECODER_FILTER_CALLBACKS rp_stream_decoder_filter_callbacks_get_type()
G_DECLARE_INTERFACE(RpStreamDecoderFilterCallbacks, rp_stream_decoder_filter_callbacks, RP, STREAM_DECODER_FILTER_CALLBACKS, RpStreamFilterCallbacks)

struct _RpStreamDecoderFilterCallbacksInterface {
    RpStreamFilterCallbacksInterface parent_iface;

    void (*continue_decoding)(RpStreamDecoderFilterCallbacks*);
    const evbuf_t* (*decoding_buffer)(RpStreamDecoderFilterCallbacks*);
    void (*modify_decoding_buffer)(RpStreamDecoderFilterCallbacks*,
                                    void (*callback)(evbuf_t*));
    void (*add_decoded_data)(RpStreamDecoderFilterCallbacks*, evbuf_t*, bool);
    void (*inject_decoded_data_to_filter_chain)(RpStreamDecoderFilterCallbacks*,
                                                evbuf_t*,
                                                bool);
    evhtp_headers_t* (*add_decoded_trailers)(RpStreamDecoderFilterCallbacks*);
    void (*send_local_reply)(RpStreamDecoderFilterCallbacks*,
                                evhtp_res,
                                evbuf_t*,
                                modify_headers_cb,
                                const char*,
                                void*);
    void (*encode_1xx_headers)(RpStreamDecoderFilterCallbacks*,
                                evhtp_headers_t*);
    void (*encode_headers)(RpStreamDecoderFilterCallbacks*,
                            evhtp_headers_t*,
                            bool,
                            const char*);
    void (*encode_data)(RpStreamDecoderFilterCallbacks*, evbuf_t*, bool);
    void (*encode_trailers)(RpStreamDecoderFilterCallbacks*, evhtp_headers_t*);
    void (*on_decoder_filter_above_write_buffer_high_water_mark)(RpStreamDecoderFilterCallbacks*);
    void (*on_decoder_filter_below_write_buffer_low_water_mark)(RpStreamDecoderFilterCallbacks*);
    void (*add_downstream_watermark_callbacks)(RpStreamDecoderFilterCallbacks*, RpDownstreamWatermarkCallbacks*);
    void (*remove_downstream_watermark_callbacks)(RpStreamDecoderFilterCallbacks*, RpDownstreamWatermarkCallbacks*);
    void (*set_decoder_buffer_limit)(RpStreamDecoderFilterCallbacks*, guint32);
    guint32 (*decoder_buffer_limit)(RpStreamDecoderFilterCallbacks*);
    //TODO...
    RpOverrideHost (*upstream_override_host)(RpStreamDecoderFilterCallbacks*);
};

void rp_stream_decoder_filter_callbacks_continue_decoding(RpStreamDecoderFilterCallbacks* self);
const evbuf_t* rp_stream_decoder_filter_callbacks_decoding_buffer(RpStreamDecoderFilterCallbacks* self);
void rp_stream_decoder_filter_callbacks_modify_decoding_buffer(RpStreamDecoderFilterCallbacks* self, void (*callback)(evbuf_t *));
void rp_stream_decoder_filter_callbacks_add_decoded_data(RpStreamDecoderFilterCallbacks* self, evbuf_t* data, bool streaming_filter);
void rp_stream_decoder_filter_callbacks_inject_decoded_data_to_filter_chain(RpStreamDecoderFilterCallbacks* self, evbuf_t* data, bool end_stream);
static inline evhtp_headers_t*
rp_stream_decoder_filter_callbacks_add_decoded_trailers(RpStreamDecoderFilterCallbacks* self)
{
    return RP_IS_STREAM_DECODER_FILTER_CALLBACKS(self) ?
        RP_STREAM_DECODER_FILTER_CALLBACKS_GET_IFACE(self)->add_decoded_trailers(self) : NULL;
}
void rp_stream_decoder_filter_callbacks_send_local_reply(RpStreamDecoderFilterCallbacks* self,
                                                            evhtp_res response_code,
                                                            evbuf_t* body_text,
                                                            modify_headers_cb modify_headers,
                                                            const char* details,
                                                            void* arg);
static inline void
rp_stream_decoder_filter_callbacks_encode_1xx_headers(RpStreamDecoderFilterCallbacks* self, evhtp_headers_t* response_headers)
{
    if (RP_IS_STREAM_DECODER_FILTER_CALLBACKS(self)) \
        RP_STREAM_DECODER_FILTER_CALLBACKS_GET_IFACE(self)->encode_1xx_headers(self, response_headers);
}
static inline void
rp_stream_decoder_filter_callbacks_encode_headers(RpStreamDecoderFilterCallbacks* self, evhtp_headers_t* response_headers, bool end_stream, const char* details)
{
    if (RP_IS_STREAM_DECODER_FILTER_CALLBACKS(self))
    {
        RP_STREAM_DECODER_FILTER_CALLBACKS_GET_IFACE(self)->encode_headers(self, response_headers, end_stream, details);
    }
}
static inline void
rp_stream_decoder_filter_callbacks_encode_data(RpStreamDecoderFilterCallbacks* self, evbuf_t* data, bool end_stream)
{
    if (RP_IS_STREAM_DECODER_FILTER_CALLBACKS(self))
    {
        RP_STREAM_DECODER_FILTER_CALLBACKS_GET_IFACE(self)->encode_data(self, data, end_stream);
    }
}
static inline void
rp_stream_decoder_filter_callbacks_encode_trailers(RpStreamDecoderFilterCallbacks* self, evhtp_headers_t* trailers)
{
    if (RP_IS_STREAM_DECODER_FILTER_CALLBACKS(self))
    {
        RP_STREAM_DECODER_FILTER_CALLBACKS_GET_IFACE(self)->encode_trailers(self, trailers);
    }
}
static inline void
rp_stream_decoder_filter_callbacks_on_decoder_filter_above_write_buffer_high_water_mark(RpStreamDecoderFilterCallbacks* self)
{
    if (RP_IS_STREAM_DECODER_FILTER_CALLBACKS(self))
    {
        RP_STREAM_DECODER_FILTER_CALLBACKS_GET_IFACE(self)->on_decoder_filter_above_write_buffer_high_water_mark(self);
    }
}
static inline void
rp_stream_decoder_filter_callbacks_on_decoder_filter_below_write_buffer_low_watermark(RpStreamDecoderFilterCallbacks* self)
{
    if (RP_IS_STREAM_DECODER_FILTER_CALLBACKS(self))
    {
        RP_STREAM_DECODER_FILTER_CALLBACKS_GET_IFACE(self)->on_decoder_filter_below_write_buffer_low_water_mark(self);
    }
}
static inline void
rp_stream_decoder_filter_callbacks_add_downstream_watermark_callbacks(RpStreamDecoderFilterCallbacks* self, RpDownstreamWatermarkCallbacks* callbacks)
{
    if (RP_IS_STREAM_DECODER_FILTER_CALLBACKS(self)) \
        RP_STREAM_DECODER_FILTER_CALLBACKS_GET_IFACE(self)->add_downstream_watermark_callbacks(self, callbacks);
}
static inline void
rp_stream_decoder_filter_callbacks_remove_downstream_watermark_callbacks(RpStreamDecoderFilterCallbacks* self, RpDownstreamWatermarkCallbacks* callbacks)
{
    if (RP_IS_STREAM_DECODER_FILTER_CALLBACKS(self)) \
        RP_STREAM_DECODER_FILTER_CALLBACKS_GET_IFACE(self)->remove_downstream_watermark_callbacks(self, callbacks);
}
static inline void
rp_stream_decoder_filter_callbacks_set_decoder_buffer_limit(RpStreamDecoderFilterCallbacks* self, guint32 limit)
{
    if (RP_IS_STREAM_DECODER_FILTER_CALLBACKS(self))
    {
        RP_STREAM_DECODER_FILTER_CALLBACKS_GET_IFACE(self)->set_decoder_buffer_limit(self, limit);
    }
}
static inline guint32
rp_stream_decoder_filter_callbacks_decoder_buffer_limit(RpStreamDecoderFilterCallbacks* self)
{
    return RP_IS_STREAM_DECODER_FILTER_CALLBACKS(self) ?
        RP_STREAM_DECODER_FILTER_CALLBACKS_GET_IFACE(self)->decoder_buffer_limit(self) :
        0;
}
static inline RpOverrideHost
rp_stream_decoder_filter_callbacks_upstream_override_host(RpStreamDecoderFilterCallbacks* self)
{
    return RP_IS_STREAM_DECODER_FILTER_CALLBACKS(self) ?
        RP_STREAM_DECODER_FILTER_CALLBACKS_GET_IFACE(self)->upstream_override_host(self) :
        RpOverrideHost_make(NULL, false);
}


/**
 * Common base class for both decoder and encoder filters. Functions here are related to the
 * lifecycle of a filter. Currently the life cycle is as follows:
 * - All filters receive onStreamComplete()
 * - All log handlers receive final log()
 * - All filters receive onDestroy()
 *
 * This means:
 * - onStreamComplete can be used to make state changes that are intended to appear in the final
 * access logs (like streamInfo().dynamicMetadata() or streamInfo().filterState()).
 * - onDestroy is used to cleanup all pending filter resources like pending http requests and
 * timers.
 */
#define RP_TYPE_STREAM_FILTER_BASE rp_stream_filter_base_get_type()
G_DECLARE_INTERFACE(RpStreamFilterBase, rp_stream_filter_base, RP, STREAM_FILTER_BASE, GObject)

struct RpStreamFilterBase_LocalReplyData {
    evhtp_res m_code;
    const char* m_details;
    bool m_reset_imminent;
};

static inline struct RpStreamFilterBase_LocalReplyData
RpStreamFilterBase_LocalReplyData_ctor(evhtp_res code, const char* details, bool reset_imminent)
{
    struct RpStreamFilterBase_LocalReplyData self = {
        .m_code = code,
        .m_details = details,
        .m_reset_imminent = reset_imminent
    };
    return self;
}

struct _RpStreamFilterBaseInterface {
    GTypeInterface parent_iface;

    void (*on_stream_complete)(RpStreamFilterBase*);
    void (*on_destroy)(RpStreamFilterBase*);
    RpLocalErrorStatus_e (*on_local_reply)(RpStreamFilterBase*,
                                            const struct RpStreamFilterBase_LocalReplyData*);
};

void rp_stream_filter_base_on_stream_complete(RpStreamFilterBase* self);
void rp_stream_filter_base_on_destroy(RpStreamFilterBase* self);
static inline RpLocalErrorStatus_e
rp_stream_filter_base_on_local_reply(RpStreamFilterBase* self, const struct RpStreamFilterBase_LocalReplyData* data)
{
    return RP_IS_STREAM_FILTER_BASE(self) ?
        RP_STREAM_FILTER_BASE_GET_IFACE(self)->on_local_reply(self, data) :
        RpLocalErrorStatus_Continue;
}


/**
 * Stream decoder filter interface.
 */
#define RP_TYPE_STREAM_DECODER_FILTER rp_stream_decoder_filter_get_type()
G_DECLARE_INTERFACE(RpStreamDecoderFilter, rp_stream_decoder_filter, RP, STREAM_DECODER_FILTER, /*RpStreamFilterBase*/GObject)

struct _RpStreamDecoderFilterInterface {
    GTypeInterface/*virtual RpStreamFilterBaseInterface*/ parent_iface;

    RpFilterHeadersStatus_e (*decode_headers)(RpStreamDecoderFilter*,
                                                evhtp_headers_t*, bool);
    RpFilterDataStatus_e (*decode_data)(RpStreamDecoderFilter*, evbuf_t*, bool);
    RpFilterTrailerStatus_e (*decode_trailers)(RpStreamDecoderFilter*,
                                                evhtp_headers_t*);
    void (*set_decoder_filter_callbacks)(RpStreamDecoderFilter*,
                                            RpStreamDecoderFilterCallbacks*);
    void (*decode_complete)(RpStreamDecoderFilter*);
};

RpFilterHeadersStatus_e rp_stream_decoder_filter_decode_headers(RpStreamDecoderFilter* self, evhtp_headers_t* request_headers, bool end_stream);
RpFilterDataStatus_e rp_stream_decoder_filter_decode_data(RpStreamDecoderFilter* self, evbuf_t* data, bool end_stream);
static inline RpFilterTrailerStatus_e
rp_stream_decoder_filter_decode_trailers(RpStreamDecoderFilter* self, evhtp_headers_t* trailers)
{
    return RP_IS_STREAM_DECODER_FILTER(self) ?
        RP_STREAM_DECODER_FILTER_GET_IFACE(self)->decode_trailers(self, trailers) :
        RpFilterTrailerStatus_Continue;
}
void rp_stream_decoder_filter_set_decoder_filter_callbacks(RpStreamDecoderFilter* self, RpStreamDecoderFilterCallbacks* callbacks);
void rp_stream_decoder_filter_decode_complete(RpStreamDecoderFilter* self);

/**
 * Stream encoder filter callbacks add additional callbacks that allow a encoding filter to restart
 * encoding if they decide to hold data (e.g. for buffering or rate limiting).
 */
#define RP_TYPE_STREAM_ENCODER_FILTER_CALLBACKS rp_stream_encoder_filter_callbacks_get_type()
G_DECLARE_INTERFACE(RpStreamEncoderFilterCallbacks, rp_stream_encoder_filter_callbacks, RP, STREAM_ENCODER_FILTER_CALLBACKS, RpStreamFilterCallbacks)

struct _RpStreamEncoderFilterCallbacksInterface {
    RpStreamFilterCallbacksInterface parent_iface;

    void (*continue_encoding)(RpStreamEncoderFilterCallbacks*);
    const evbuf_t* (*encoding_buffer)(RpStreamEncoderFilterCallbacks*);
    void (*modify_encoding_buffer)(RpStreamEncoderFilterCallbacks*,
                                    void (*callback)(evbuf_t*));
    void (*add_encoded_data)(RpStreamEncoderFilterCallbacks*, evbuf_t*, bool);
    void (*inject_encoded_data_to_filter_chain)(RpStreamEncoderFilterCallbacks*,
                                                evbuf_t*,
                                                bool);
    void (*send_local_reply)(RpStreamEncoderFilterCallbacks*,
                                evhtp_res,
                                evbuf_t*,
                                void (*modify_headers)(evhtp_headers_t*),
                                const char*);
    //TODO: others from envoy proxy?????
};

void rp_stream_encoder_filter_callbacks_continue_encoding(RpStreamEncoderFilterCallbacks* self);
const evbuf_t* rp_stream_encoder_filter_callbacks_encoding_buffer(RpStreamEncoderFilterCallbacks* self);
void rp_stream_encoder_filter_callbacks_modify_decoding_buffer(RpStreamEncoderFilterCallbacks* self, void (*callback)(evbuf_t *));
void rp_stream_encoder_filter_callbacks_add_decoded_data(RpStreamEncoderFilterCallbacks* self, evbuf_t* data, bool streaming_filter);
void rp_stream_encoder_filter_callbacks_inject_decoded_data_to_filter_chain(RpStreamEncoderFilterCallbacks* self, evbuf_t* data, bool end_stream);
void rp_stream_encoder_filter_callbacks_send_local_reply(RpStreamEncoderFilterCallbacks* self, evhtp_res response_code, evbuf_t* body_text, void (*modify_headers)(evhtp_headers_t *), const char* details);

#define RP_TYPE_STREAM_ENCODER_FILTER rp_stream_encoder_filter_get_type()
G_DECLARE_INTERFACE(RpStreamEncoderFilter, rp_stream_encoder_filter, RP, STREAM_ENCODER_FILTER, GObject)

struct _RpStreamEncoderFilterInterface {
    GTypeInterface/*virtual RpStreamFilterBaseInterface*/ parent_iface;

    RpFilterHeadersStatus_e (*encode_headers)(RpStreamEncoderFilter*,
                                                evhtp_headers_t*,
                                                bool);
    RpFilterDataStatus_e (*encode_data)(RpStreamEncoderFilter*, evbuf_t*, bool);
    RpFilterTrailerStatus_e (*encode_trailers)(RpStreamEncoderFilter*,
                                                evhtp_headers_t*);
    void (*set_encoder_filter_callbacks)(RpStreamEncoderFilter*,
                                            RpStreamEncoderFilterCallbacks*);
    void (*encode_complete)(RpStreamEncoderFilter*);
};

RpFilterHeadersStatus_e rp_stream_encoder_filter_encode_headers(RpStreamEncoderFilter* self, evhtp_headers_t* response_headers, bool end_stream);
RpFilterDataStatus_e rp_stream_encoder_filter_encode_data(RpStreamEncoderFilter* self, evbuf_t* data, bool end_stream);
static inline RpFilterTrailerStatus_e
rp_stream_encoder_filter_encode_trailers(RpStreamEncoderFilter* self, evhtp_headers_t* trailers)
{
    return RP_IS_STREAM_ENCODER_FILTER(self) ?
        RP_STREAM_ENCODER_FILTER_GET_IFACE(self)->encode_trailers(self, trailers) :
        RpFilterTrailerStatus_Continue;
}
void rp_stream_encoder_filter_set_encoder_filter_callbacks(RpStreamEncoderFilter* self, RpStreamEncoderFilterCallbacks* callbacks);
void rp_stream_encoder_filter_encode_complete(RpStreamEncoderFilter* self);

#define RP_TYPE_STREAM_FILTER rp_stream_filter_get_type()
G_DECLARE_INTERFACE(RpStreamFilter, rp_stream_filter, RP, STREAM_FILTER, RpStreamFilterBase)

struct _RpStreamFilterInterface {
    RpStreamFilterBaseInterface parent_iface;

};

/**
 * These callbacks are provided by the connection manager to the factory so that the factory can
 * build the filter chain in an application specific way.
 */
#define RP_TYPE_FILTER_CHAIN_FACTORY_CALLBACKS rp_filter_chain_factory_callbacks_get_type()
G_DECLARE_INTERFACE(RpFilterChainFactoryCallbacks, rp_filter_chain_factory_callbacks, RP, FILTER_CHAIN_FACTORY_CALLBACKS, GObject)

struct _RpFilterChainFactoryCallbacksInterface {
    GTypeInterface parent_iface;

    void (*add_stream_decoder_filter)(RpFilterChainFactoryCallbacks*,
                                        RpStreamDecoderFilter*);
    void (*add_stream_encoder_filter)(RpFilterChainFactoryCallbacks*,
                                        RpStreamEncoderFilter*);
    void (*add_stream_filter)(RpFilterChainFactoryCallbacks*, RpStreamFilter*);
    RpDispatcher* (*dispatcher)(RpFilterChainFactoryCallbacks*);
    RpFilterManager* (*filter_manager)(RpFilterChainFactoryCallbacks*);
};

void rp_filter_chain_factory_callbacks_add_stream_encoder_filter(RpFilterChainFactoryCallbacks* self, RpStreamEncoderFilter* filter);
void rp_filter_chain_factory_callbacks_add_stream_decoder_filter(RpFilterChainFactoryCallbacks* self, RpStreamDecoderFilter* filter);
void rp_filter_chain_factory_callbacks_add_stream_filter(RpFilterChainFactoryCallbacks* self, RpStreamFilter* filter);
static inline RpDispatcher*
rp_filter_chain_factory_callbacks_dispatcher(RpFilterChainFactoryCallbacks* self)
{
    return RP_IS_FILTER_CHAIN_FACTORY_CALLBACKS(self) ?
        RP_FILTER_CHAIN_FACTORY_CALLBACKS_GET_IFACE(self)->dispatcher(self) :
        NULL;
}
static inline RpFilterManager*
rp_filter_chain_factory_callbacks_filter_manager(RpFilterChainFactoryCallbacks* self)
{
    return RP_IS_FILTER_CHAIN_FACTORY_CALLBACKS(self) ?
        RP_FILTER_CHAIN_FACTORY_CALLBACKS_GET_IFACE(self)->filter_manager(self) :
        NULL;
}

G_END_DECLS
