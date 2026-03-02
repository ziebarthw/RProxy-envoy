/*
 * rp-stream-info.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <evhtp.h>
#include "rp-filter-state.h"
#include "rp-host-description.h"
#include "rp-net-address.h"
#include "rp-socket.h"
#include "rp-time.h"

G_BEGIN_DECLS

typedef struct _RpRoute RpRoute;
typedef const SHARED_PTR(RpRoute) RpRouteConstSharedPtr;
typedef SHARED_PTR(RpRoute) RpRouteSharedPtr;
typedef const SHARED_PTR(RpClusterInfo) RpClusterInfoConstSharedPtr;
typedef SHARED_PTR(RpClusterInfo) RpClusterInfoSharedPtr;

typedef enum {
    // Local server healthcheck failed.
    RpCoreResponseFlag_FailedLocalHealthCheck = 1 << 0,
    // No healthy upstream.
    RpCoreResponseFlag_NoHealthyUpstream = 1 << 1,
    // Request timeout on upstream.
    RpCoreResponseFlag_UpstreamRequestTimeout = 1 << 2,
    // Local codec level reset was sent on the stream.
    RpCoreResponseFlag_LocalReset = 1 << 3,
    // Remote codec level reset was received on the stream.
    RpCoreResponseFlag_UpstreamRemoteReset = 1 << 4,
    // Local reset by a connection pool due to an initial connection failure.
    RpCoreResponseFlag_UpstreamConnectionFailure = 1 << 5,
    // If the stream was locally reset due to connection termination.
    RpCoreResponseFlag_UpstreamConnectionTermination = 1 << 6,
    // The stream was reset because of a resource overflow.
    RpCoreResponseFlag_UpstreamOverflow = 1 << 7,
    // No route found for a given request.
    RpCoreResponseFlag_NoRouteFound = 1 << 8,
    // Request was delayed before proxying.
    RpCoreResponseFlag_DelayInjected = 1 << 9,
    // Abort with error code was injected.
    RpCoreResponseFlag_FaultInjected = 1 << 10,
    // Request was ratelimited locally by rate limit filter.
    RpCoreResponseFlag_RateLimited = 1 << 11,
    // Request was unauthorized by external authorization service.
    RpCoreResponseFlag_UnauthorizedExternalService = 1 << 12,
    // Unable to call Ratelimit service.
    RpCoreResponseFlag_RateLimitServiceError = 1 << 13,
    // If the stream was reset due to a downstream connection termination.
    RpCoreResponseFlag_DownstreamConnectionTermination = 1 << 14,
    // Exceeded upstream retry limit.
    RpCoreResponseFlag_UpstreamRetryLimitExceeded = 1 << 15,
    // Request hit the stream idle timeout, triggering a 408.
    RpCoreResponseFlag_StreamIdleTimeout = 1 << 16,
    // Request specified x-envoy-* header values that failed strict header checks.
    RpCoreResponseFlag_InvalidEnvoyRequestHeaders = 1 << 17,
    // Downstream request had an HTTP protocol error
    RpCoreResponseFlag_DownstreamProtocolError = 1 << 18,
    // Upstream request reached to user defined max stream duration.
    RpCoreResponseFlag_UpstreamMaxStreamDurationReached = 1 << 19,
    // True if the response was served from an Envoy cache filter.
    RpCoreResponseFlag_ResponseFromCacheFilter = 1 << 20,
    // Filter config was not received within the permitted warming deadline.
    RpCoreResponseFlag_NoFilterConfigFound = 1 << 21,
    // Request or connection exceeded the downstream connection duration.
    RpCoreResponseFlag_DurationTimeout = 1 << 22,
    // Upstream response had an HTTP protocol error
    RpCoreResponseFlag_UpstreamProtocolError = 1 << 23,
    // No cluster found for a given request.
    RpCoreResponseFlag_NoClusterFound = 1 << 24,
    // Overload Manager terminated the stream.
    RpCoreResponseFlag_OverloadManager = 1 << 25,
    // DNS resolution failed.
    RpCoreResponseFlag_DnsResolutionFailed = 1 << 26,
    // Drop certain percentage of overloaded traffic.
    RpCoreResponseFlag_DropOverLoad = 1 << 27,
    // Downstream remote codec level reset was received on the stream.
    RpCoreResponseFlag_DownstreamRemoteReset = 1 << 28,
    // Unconditionally drop all traffic due to drop_overload is set to 100%.
    RpCoreResponseFlag_UnconditionalDropOverload = 1 << 29,
    // ATTENTION: MAKE SURE THIS REMAINS EQUAL TO THE LAST FLAG.
    RpCoreResponseFlag_LastFlag = RpCoreResponseFlag_UnconditionalDropOverload,
} RpCoreResponseFlag_e;


typedef struct _RpUpstreamTiming RpUpstreamTiming;
struct _RpUpstreamTiming {
    RpMonotonicTime m_first_upstream_tx_byte_sent;
    RpMonotonicTime m_last_upstream_tx_byte_sent;
    RpMonotonicTime m_first_upstream_rx_byte_received;
    RpMonotonicTime m_last_upstream_rx_byte_received;
    RpMonotonicTime m_upstream_handshake_complete;
    RpMonotonicTime m_upstream_connect_start;
    RpMonotonicTime m_upstream_connect_complete;
};
static inline RpUpstreamTiming
rp_upstream_timing_ctor(void)
{
    RpUpstreamTiming self = {
        .m_first_upstream_tx_byte_sent = 0,
        .m_last_upstream_tx_byte_sent = 0,
        .m_first_upstream_rx_byte_received = 0,
        .m_last_upstream_rx_byte_received = 0,
        .m_upstream_handshake_complete = 0,
        .m_upstream_connect_start = 0,
        .m_upstream_connect_complete = 0
    };
    return self;
}
static inline RpMonotonicTime
rp_upstream_timing_fist_upstream_rx_byte_received(RpUpstreamTiming* self)
{
    return self ? self->m_first_upstream_rx_byte_received : 0;
}
static inline RpMonotonicTime
rp_upstream_timing_last_upstream_rx_byte_received(RpUpstreamTiming* self)
{
    return self ? self->m_last_upstream_rx_byte_received : 0;
}
static inline RpMonotonicTime
rp_upstream_timing_first_upstream_tx_byte_sent(RpUpstreamTiming* self)
{
    return self ? self->m_last_upstream_tx_byte_sent : 0;
}
static inline RpMonotonicTime
rp_upstream_timing_last_upstream_tx_byte_sent(RpUpstreamTiming* self)
{
    return self ? self->m_last_upstream_tx_byte_sent : 0;
}
static inline RpMonotonicTime
rp_upstream_timing_upstream_handshake_complete(RpUpstreamTiming* self)
{
    return self ? self->m_upstream_handshake_complete : 0;
}
static inline RpMonotonicTime
rp_upstream_timing_upstream_connect_start(RpUpstreamTiming* self)
{
    return self ? self->m_upstream_connect_start : 0;
}
static inline RpMonotonicTime
rp_upstream_timing_upstream_connect_complete(RpUpstreamTiming* self)
{
    return self ? self->m_upstream_connect_complete : 0;
}
static inline void
rp_upstream_timing_on_last_upstream_rx_byte_received(RpUpstreamTiming* self)
{
    if (self) self->m_last_upstream_rx_byte_received = g_get_monotonic_time();
}
static inline void
rp_upstream_timing_on_first_upstream_tx_byte_sent(RpUpstreamTiming* self)
{
    if (self) self->m_first_upstream_tx_byte_sent = g_get_monotonic_time();
}
static inline void
rp_upstream_timing_on_first_upstream_rx_byte_received(RpUpstreamTiming* self)
{
    if (self) self->m_first_upstream_rx_byte_received = g_get_monotonic_time();
}
static inline void
rp_upstream_timing_on_last_upstream_tx_byte_sent(RpUpstreamTiming* self)
{
    if (self) self->m_last_upstream_tx_byte_sent = g_get_monotonic_time();
}
static inline void
rp_upstream_timing_on_upstream_handshake_complete(RpUpstreamTiming* self)
{
    if (self) self->m_upstream_handshake_complete = g_get_monotonic_time();
}
static inline void
rp_upstream_timing_on_upstream_connect_start(RpUpstreamTiming* self)
{
    if (self) self->m_upstream_connect_start = g_get_monotonic_time();
}
static inline void
rp_upstream_timing_on_upstream_connect_complete(RpUpstreamTiming* self)
{
    if (self) self->m_upstream_connect_complete = g_get_monotonic_time();
}


typedef struct _RpDownstreamTiming RpDownstreamTiming;
struct _RpDownstreamTiming {
    RpMonotonicTime m_last_downstream_rx_byte_received;
    RpMonotonicTime m_first_downstream_tx_byte_sent;
    RpMonotonicTime m_last_downstream_tx_byte_sent;
    RpMonotonicTime m_downstream_handshake_complete;
    RpMonotonicTime m_last_downstream_ack_received;
    RpMonotonicTime m_last_downstream_header_rx_byte_received;
};
static inline RpDownstreamTiming
rp_downstream_timing_ctor(void)
{
    RpDownstreamTiming self = {
        .m_last_downstream_rx_byte_received = 0,
        .m_first_downstream_tx_byte_sent = 0,
        .m_last_downstream_tx_byte_sent = 0,
        .m_downstream_handshake_complete = 0,
        .m_last_downstream_ack_received = 0,
        .m_last_downstream_header_rx_byte_received = 0
    };
    return self;
}
static inline RpMonotonicTime
rp_downstream_timing_last_downstream_rx_byte_received(RpDownstreamTiming* self)
{
    return self ? self->m_last_downstream_rx_byte_received : 0;
}
static inline RpMonotonicTime
rp_downstream_timing_first_downstream_tx_byte_sent(RpDownstreamTiming* self)
{
    return self ? self->m_last_downstream_tx_byte_sent : 0;
}
static inline RpMonotonicTime
rp_downstream_timing_last_downstream_tx_byte_sent(RpDownstreamTiming* self)
{
    return self ? self->m_last_downstream_tx_byte_sent : 0;
}
static inline RpMonotonicTime
rp_downstream_timing_downstream_handshake_complete(RpDownstreamTiming* self)
{
    return self ? self->m_downstream_handshake_complete : 0;
}
static inline RpMonotonicTime
rp_downstream_timing_last_downstream_ack_received(RpDownstreamTiming* self)
{
    return self ? self->m_last_downstream_ack_received : 0;
}
static inline RpMonotonicTime
rp_downstream_timing_last_downstream_header_rx_byte_received(RpDownstreamTiming* self)
{
    return self ? self->m_last_downstream_header_rx_byte_received : 0;
}
static inline void
rp_downstream_timing_on_last_downstream_rx_byte_received(RpDownstreamTiming* self)
{
    if (self) self->m_last_downstream_rx_byte_received = g_get_monotonic_time();
}
static inline void
rp_downstream_timing_on_first_downstream_tx_byte_sent(RpDownstreamTiming* self)
{
    if (self) self->m_first_downstream_tx_byte_sent = g_get_monotonic_time();
}
static inline void
rp_downstream_timing_on_last_downstream_tx_byte_sent(RpDownstreamTiming* self)
{
    if (self) self->m_last_downstream_tx_byte_sent = g_get_monotonic_time();
}
static inline void
rp_downstream_timing_on_downstream_handshake_complete(RpDownstreamTiming* self)
{
    if (self) self->m_downstream_handshake_complete = g_get_monotonic_time();
}
static inline void
rp_downstream_timing_on_last_downstream_ack_received(RpDownstreamTiming* self)
{
    if (self) self->m_last_downstream_ack_received = g_get_monotonic_time();
}
static inline void
rp_downstream_timing_on_last_downstream_header_rx_byte_received(RpDownstreamTiming* self)
{
    if (self) self->m_last_downstream_header_rx_byte_received = g_get_monotonic_time();
}


#define RP_TYPE_UPSTREAM_INFO rp_upstream_info_get_type()
G_DECLARE_INTERFACE(RpUpstreamInfo, rp_upstream_info, RP, UPSTREAM_INFO, GObject)

struct _RpUpstreamInfoInterface {
    GTypeInterface parent_iface;

    void (*set_upstream_connection_id)(RpUpstreamInfo*, guint64);
    guint64 (*upstream_connection_id)(const RpUpstreamInfo*);
    void (*set_upstream_interface_name)(RpUpstreamInfo*, const char*);
    const char* (*upstream_interface_name)(const RpUpstreamInfo*);
    void (*set_upstream_ssl_connection)(RpUpstreamInfo*, RpSslConnectionInfo*);
    RpSslConnectionInfo* (*upstream_ssl_connection)(const RpUpstreamInfo*);
    RpUpstreamTiming* (*upstream_timing)(const RpUpstreamInfo*);
    void (*set_upstream_local_address)(RpUpstreamInfo*, RpNetworkAddressInstanceConstSharedPtr);
    RpNetworkAddressInstanceConstSharedPtr (*upstream_local_address)(const RpUpstreamInfo*);
    void (*set_upstream_remote_address)(RpUpstreamInfo*, RpNetworkAddressInstanceConstSharedPtr);
    RpNetworkAddressInstanceConstSharedPtr (*upstream_remote_address)(const RpUpstreamInfo*);
    void (*set_upstream_transport_failure_reason)(RpUpstreamInfo*, const char*);
    const char* (*upstream_transport_failure_reason)(const RpUpstreamInfo*);
    void (*set_upstream_host)(RpUpstreamInfo*, RpHostDescriptionConstSharedPtr);
    RpHostDescriptionConstSharedPtr (*upstream_host)(const RpUpstreamInfo*);
    RpFilterState* (*upstream_filter_state)(const RpUpstreamInfo*);
    void (*set_upstream_filter_state)(RpUpstreamInfo*, RpFilterState*);
    void (*set_upstream_num_streams)(RpUpstreamInfo*, gint64);
    guint64 (*upstream_num_streams)(const RpUpstreamInfo*);
    void (*set_upstream_protocol)(RpUpstreamInfo*, evhtp_proto);
    evhtp_proto (*upstream_protocol)(const RpUpstreamInfo*);
};

static inline gboolean
rp_upstream_info_is_a(const RpUpstreamInfo* self)
{
    g_return_val_if_fail(self != NULL, FALSE);
    return RP_IS_UPSTREAM_INFO((GObject*)self);
}
static inline RpUpstreamInfoInterface*
rp_upstream_info_iface(const RpUpstreamInfo* self)
{
    g_return_val_if_fail(rp_upstream_info_is_a(self), NULL);
    return RP_UPSTREAM_INFO_GET_IFACE((GObject*)self);
}
static inline void
rp_upstream_info_set_object(RpUpstreamInfo** dst, const RpUpstreamInfo* src)
{
    g_return_if_fail(dst != NULL);
    g_set_object((GObject**)dst, (GObject*)src);
}
static inline void
rp_upstream_info_set_upstream_connection_id(RpUpstreamInfo* self, guint64 id)
{
    g_return_if_fail(rp_upstream_info_is_a(self));
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    if (iface && iface->set_upstream_connection_id) \
        iface->set_upstream_connection_id(self, id);
}
static inline guint64
rp_upstream_info_upstream_connection_id(const RpUpstreamInfo* self)
{
    g_return_val_if_fail(rp_upstream_info_is_a(self), 0);
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    return iface && iface->upstream_connection_id ?
        iface->upstream_connection_id(self) : 0;
}
static inline void
rp_upstream_info_set_upstream_interface_name(RpUpstreamInfo* self, const char* interface_name)
{
    g_return_if_fail(rp_upstream_info_is_a(self));
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    if (iface && iface->set_upstream_interface_name) \
        iface->set_upstream_interface_name(self, interface_name);
}
static inline void
rp_upstream_info_set_upstream_ssl_connection(RpUpstreamInfo* self, RpSslConnectionInfo* ssl_connection_info)
{
    g_return_if_fail(rp_upstream_info_is_a(self));
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    if (iface && iface->set_upstream_ssl_connection) \
        iface->set_upstream_ssl_connection(self, ssl_connection_info);
}
static inline RpUpstreamTiming*
rp_upstream_info_upstream_timing(const RpUpstreamInfo* self)
{
    g_return_val_if_fail(rp_upstream_info_is_a(self), NULL);
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    return iface && iface->upstream_timing ?
        iface->upstream_timing(self) : NULL;
}
static inline RpSslConnectionInfo*
rp_upstream_info_upstream_ssl_connection(const RpUpstreamInfo* self)
{
    g_return_val_if_fail(rp_upstream_info_is_a(self), NULL);
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    return iface && iface->upstream_ssl_connection ?
        iface->upstream_ssl_connection(self) : NULL;
}
static inline const char*
rp_upstream_info_upstream_interface_name(const RpUpstreamInfo* self)
{
    g_return_val_if_fail(rp_upstream_info_is_a(self), NULL);
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    return iface && iface->upstream_interface_name ?
        iface->upstream_interface_name(self) : NULL;
}
static inline void
rp_upstream_info_set_upstream_local_address(RpUpstreamInfo* self, RpNetworkAddressInstanceConstSharedPtr upstream_local_address)
{
    g_return_if_fail(rp_upstream_info_is_a(self));
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    if (iface && iface->set_upstream_local_address) \
        iface->set_upstream_local_address(self, upstream_local_address);
}
static inline RpNetworkAddressInstanceConstSharedPtr
rp_upstream_info_upstream_local_address(const RpUpstreamInfo* self)
{
    g_return_val_if_fail(rp_upstream_info_is_a(self), NULL);
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    return iface && iface->upstream_local_address ?
        iface->upstream_local_address(self) : NULL;
}
static inline void
rp_upstream_info_set_upstream_remote_address(RpUpstreamInfo* self, RpNetworkAddressInstanceConstSharedPtr upstream_remote_address)
{
    g_return_if_fail(rp_upstream_info_is_a(self));
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    if (iface && iface->set_upstream_remote_address) \
        iface->set_upstream_remote_address(self, upstream_remote_address);
}
static inline RpNetworkAddressInstanceConstSharedPtr
rp_upstream_info_upstream_remote_address(const RpUpstreamInfo* self)
{
    g_return_val_if_fail(rp_upstream_info_is_a(self), NULL);
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    return iface && iface->upstream_remote_address ?
        iface->upstream_remote_address(self) : NULL;
}
static inline void
rp_upstream_info_set_upstream_transport_failure_reason(RpUpstreamInfo* self, const char* failure_reason)
{
    g_return_if_fail(rp_upstream_info_is_a(self));
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    if (iface && iface->set_upstream_transport_failure_reason) \
        iface->set_upstream_transport_failure_reason(self, failure_reason);
}
static inline const char*
rp_upstream_info_upstream_transport_failure_reason(const RpUpstreamInfo* self)
{
    g_return_val_if_fail(rp_upstream_info_is_a(self), NULL);
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    return iface && iface->upstream_transport_failure_reason ?
        iface->upstream_transport_failure_reason(self) : NULL;
}
static inline void
rp_upstream_info_set_upstream_host(RpUpstreamInfo* self, RpHostDescriptionConstSharedPtr host)
{
    g_return_if_fail(rp_upstream_info_is_a(self));
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    if (iface && iface->set_upstream_host) \
        iface->set_upstream_host(self, host);
}
static inline RpHostDescriptionConstSharedPtr
rp_upstream_info_upstream_host(const RpUpstreamInfo* self)
{
    g_return_val_if_fail(rp_upstream_info_is_a(self), NULL);
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    return iface && iface->upstream_host ?
        iface->upstream_host(self) : NULL;
}
static inline RpFilterState*
rp_upstream_info_upstream_filter_state(const RpUpstreamInfo* self)
{
    g_return_val_if_fail(rp_upstream_info_is_a(self), NULL);
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    return iface && iface->upstream_filter_state ?
        iface->upstream_filter_state(self) : NULL;
}
static inline void
rp_upstream_info_set_upstream_filter_state(RpUpstreamInfo* self, RpFilterState* filter_state)
{
    g_return_if_fail(rp_upstream_info_is_a(self));
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    if (iface && iface->set_upstream_filter_state) \
        iface->set_upstream_filter_state(self, filter_state);
}
static inline void
rp_upstream_info_set_upstream_num_streams(RpUpstreamInfo* self, gint64 num_streams)
{
    g_return_if_fail(rp_upstream_info_is_a(self));
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    if (iface && iface->set_upstream_num_streams) \
        iface->set_upstream_num_streams(self, num_streams);
}
static inline guint64
rp_upstream_info_upstream_num_streams(const RpUpstreamInfo* self)
{
    g_return_val_if_fail(rp_upstream_info_is_a(self), 0);
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    return iface && iface->upstream_connection_id ?
        iface->upstream_num_streams(self) : 0;
}
static inline void
rp_upstream_info_set_upstream_protocol(RpUpstreamInfo* self, evhtp_proto protocol)
{
    g_return_if_fail(rp_upstream_info_is_a(self));
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    if (iface && iface->set_upstream_protocol) \
        iface->set_upstream_protocol(self, protocol);
}
static inline evhtp_proto
rp_upstream_info_upstream_protocol(const RpUpstreamInfo* self)
{
    g_return_val_if_fail(rp_upstream_info_is_a(self), EVHTP_PROTO_INVALID);
    RpUpstreamInfoInterface* iface = rp_upstream_info_iface(self);
    return iface && iface->upstream_protocol?
        iface->upstream_protocol(self) : EVHTP_PROTO_INVALID;
}


/**
 * Additional information about a completed request for logging.
 */
#define RP_TYPE_STREAM_INFO rp_stream_info_get_type()
G_DECLARE_INTERFACE(RpStreamInfo, rp_stream_info, RP, STREAM_INFO, GObject)

struct _RpStreamInfoInterface {
    GTypeInterface parent_iface;

    void (*set_response_flag)(RpStreamInfo*, RpCoreResponseFlag_e);
    void (*set_response_code)(RpStreamInfo*, evhtp_res);
    void (*set_response_code_details)(RpStreamInfo*, const char*);
    void (*set_connection_termination_details)(RpStreamInfo*, const char*);
    const char* (*get_route_name)(const RpStreamInfo*);
    void (*add_bytes_received)(RpStreamInfo*, guint64);
    guint64 (*bytes_received)(const RpStreamInfo*);
    void (*add_bytes_retransmitted)(RpStreamInfo*, guint64);
    guint64 (*bytes_retransmitted)(const RpStreamInfo*);
    void (*add_packets_retransmitted)(RpStreamInfo*, guint64);
    guint64 (*packets_retransmitted)(const RpStreamInfo*);
    evhtp_proto (*protocol)(const RpStreamInfo*);
    void (*set_protocol)(RpStreamInfo*, evhtp_proto);
    evhtp_res (*response_code)(const RpStreamInfo*);
    const char* (*response_code_details)(const RpStreamInfo*);
    const char* (*connection_termination_details)(const RpStreamInfo*);
    RpSystemTime (*start_time)(const RpStreamInfo*);
    RpMonotonicTime (*start_time_monotonic)(RpStreamInfo*);
    //TODO...
    void (*set_upstream_info)(RpStreamInfo*, SHARED_PTR(RpUpstreamInfo));
    SHARED_PTR(RpUpstreamInfo) (*upstream_info)(const RpStreamInfo*);
    gint64 (*request_complete)(const RpStreamInfo*);
    void (*on_request_complete)(RpStreamInfo*);
    RpDownstreamTiming* (*downstream_timing)(const RpStreamInfo*);
    void (*add_bytes_sent)(RpStreamInfo*, guint64);
    guint64 (*bytes_sent)(const RpStreamInfo*);
    bool (*has_response_flag)(const RpStreamInfo*, RpCoreResponseFlag_e);
    bool (*has_any_response_flags)(const RpStreamInfo*);
    guint64 (*legacy_response_flags)(const RpStreamInfo*);
    //TODO...
    RpConnectionInfoProviderSharedPtr (*downstream_address_provider)(const RpStreamInfo*);
    RpRouteSharedPtr (*route)(const RpStreamInfo*);
    GHashTable* (*dynamic_metadata)(const RpStreamInfo*);
    void (*set_dynamic_metadata)(RpStreamInfo*, const char*, void*);
    RpFilterState* (*filter_state)(const RpStreamInfo*);
    void (*set_request_headers)(RpStreamInfo*, evhtp_headers_t*);
    evhtp_headers_t* (*get_request_headers)(const RpStreamInfo*);
    void (*set_upstream_cluster_info)(RpStreamInfo*, RpClusterInfoConstSharedPtr);
    RpClusterInfoSharedPtr (*upstream_cluster_info)(const RpStreamInfo*);
    //TODO...
    void (*set_attempt_count)(RpStreamInfo*, guint32);
    guint32 (*attempt_count)(const RpStreamInfo*);
    //TODO...BytesMeter* (*bytes_meter)(RpStreamInfo*);
    //TODO...
    const char* (*downstream_transport_failure_reason)(const RpStreamInfo*);
    void (*set_downstream_transport_failure_reason)(RpStreamInfo*, const char*);
    bool (*should_scheme_match_upstream)(const RpStreamInfo*);
    void (*set_should_scheme_match_upstream)(RpStreamInfo*, bool);
    bool (*should_drain_connection_upon_completion)(const RpStreamInfo*);
    void (*set_should_drain_connection_upon_completion)(RpStreamInfo*, bool);
    void (*set_parent_stream_info)(RpStreamInfo*, RpStreamInfo*);
    RpStreamInfo* (*parent_stream_info)(const RpStreamInfo*);
    void (*clear_parent_stream_info)(RpStreamInfo*);
};

static inline gboolean
rp_stream_info_is_a(const RpStreamInfo* self)
{
    g_return_val_if_fail(self != NULL, FALSE);
    return RP_IS_STREAM_INFO((GObject*)self);
}
static inline RpStreamInfoInterface*
rp_stream_info_iface(const RpStreamInfo* self)
{
    g_return_val_if_fail(rp_stream_info_is_a(self), NULL);
    return RP_STREAM_INFO_GET_IFACE((GObject*)self);
}
static inline void
rp_stream_info_set_object(RpStreamInfo** dst, const RpStreamInfo* src)
{
    g_return_if_fail(dst != NULL);
    g_set_object((GObject**)dst, (GObject*)src);
}
static inline void
rp_stream_info_set_response_flag(RpStreamInfo* self, RpCoreResponseFlag_e response_flag)
{
    g_return_if_fail(rp_stream_info_is_a(self));
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    if (iface && iface->set_response_flag) \
        iface->set_response_flag(self, response_flag);
}
static inline void
rp_stream_info_set_response_code(RpStreamInfo* self, evhtp_res response_code)
{
    g_return_if_fail(rp_stream_info_is_a(self));
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    if (iface && iface->set_response_code) \
        iface->set_response_code(self, response_code);
}
static inline void
rp_stream_info_set_upstream_info(RpStreamInfo* self, SHARED_PTR(RpUpstreamInfo) upstream_info)
{
    g_return_if_fail(rp_stream_info_is_a(self));
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    if (iface && iface->set_upstream_info) \
        iface->set_upstream_info(self, upstream_info);
}
static inline RpUpstreamInfo*
rp_stream_info_upstream_info(const RpStreamInfo* self)
{
    g_return_val_if_fail(rp_stream_info_is_a(self), NULL);
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    return (iface && iface->upstream_info) ?
        iface->upstream_info(self) : NULL;
}
static inline GHashTable*
rp_stream_info_dynamic_metadata(const RpStreamInfo* self)
{
    g_return_val_if_fail(rp_stream_info_is_a(self), NULL);
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    return (iface && iface->dynamic_metadata) ?
        iface->dynamic_metadata(self) : NULL;
}
static inline void
rp_stream_info_set_dynamic_metadata(RpStreamInfo* self, const char* name, void* value)
{
    g_return_if_fail(rp_stream_info_is_a(self));
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    if (iface && iface->set_dynamic_metadata) \
        iface->set_dynamic_metadata(self, name, value);
}
static inline RpFilterState*
rp_stream_info_filter_state(const RpStreamInfo* self)
{
    g_return_val_if_fail(rp_stream_info_is_a(self), NULL);
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    return (iface && iface->filter_state) ? iface->filter_state(self) : NULL;
}
static inline void
rp_stream_info_set_request_headers(RpStreamInfo* self, evhtp_headers_t* request_headers)
{
    g_return_if_fail(rp_stream_info_is_a(self));
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    if (iface && iface->set_request_headers) \
        iface->set_request_headers(self, request_headers);
}
static inline evhtp_headers_t*
rp_stream_info_get_request_headers(const RpStreamInfo* self)
{
    g_return_val_if_fail(rp_stream_info_is_a(self), NULL);
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    return (iface && iface->get_request_headers) ?
        iface->get_request_headers(self) : NULL;
}
static inline void
rp_stream_info_set_upstream_cluster_info(RpStreamInfo* self, RpClusterInfoConstSharedPtr upstream_cluster_info)
{
    g_return_if_fail(rp_stream_info_is_a(self));
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    if (iface && iface->set_upstream_cluster_info) \
        iface->set_upstream_cluster_info(self, upstream_cluster_info);
}
static inline RpClusterInfoSharedPtr
rp_stream_info_upstream_cluster_info(const RpStreamInfo* self)
{
    g_return_val_if_fail(rp_stream_info_is_a(self), NULL);
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    return (iface && iface->upstream_cluster_info) ?
        iface->upstream_cluster_info(self) : NULL;
}
static inline RpConnectionInfoProviderSharedPtr
rp_stream_info_downstream_address_provider(const RpStreamInfo* self)
{
    g_return_val_if_fail(rp_stream_info_is_a(self), NULL);
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    return (iface && iface->downstream_address_provider) ?
        iface->downstream_address_provider(self) : NULL;
}
static inline RpRouteSharedPtr
rp_stream_info_route(const RpStreamInfo* self)
{
    g_return_val_if_fail(rp_stream_info_is_a(self), NULL);
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    return (iface && iface->route) ? iface->route(self) : NULL;
}
static inline evhtp_proto
rp_stream_info_protocol(const RpStreamInfo* self)
{
    g_return_val_if_fail(rp_stream_info_is_a(self), EVHTP_PROTO_INVALID);
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    return (iface && iface->protocol) ?
        iface->protocol(self) : EVHTP_PROTO_INVALID;
}
static inline void
rp_stream_info_set_protocol(RpStreamInfo* self, evhtp_proto protocol)
{
    g_return_if_fail(rp_stream_info_is_a(self));
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    if (iface && iface->set_protocol) \
        iface->set_protocol(self, protocol);
}
static inline evhtp_res
rp_stream_info_response_code(const RpStreamInfo* self)
{
    g_return_val_if_fail(rp_stream_info_is_a(self), (evhtp_res)0);
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    return (iface && iface->response_code) ?
        iface->response_code(self) : (evhtp_res)0;
}
static inline bool
rp_stream_info_should_drain_connection_upon_completion(const RpStreamInfo* self)
{
    g_return_val_if_fail(rp_stream_info_is_a(self), false);
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    return (iface && iface->should_drain_connection_upon_completion) ?
        iface->should_drain_connection_upon_completion(self) : false;
}
static inline void
rp_stream_info_set_should_drain_connection_upon_completion(RpStreamInfo* self, bool should_drain)
{
    g_return_if_fail(rp_stream_info_is_a(self));
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    if (iface && iface->set_should_drain_connection_upon_completion) \
        iface->set_should_drain_connection_upon_completion(self, should_drain);
}
static inline RpDownstreamTiming*
rp_stream_info_downstream_timing(const RpStreamInfo* self)
{
    g_return_val_if_fail(rp_stream_info_is_a(self), NULL);
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    return (iface && iface->downstream_timing) ?
        iface->downstream_timing(self) : NULL;
}
static inline void
rp_stream_info_on_request_complete(RpStreamInfo* self)
{
    g_return_if_fail(rp_stream_info_is_a(self));
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    if (iface && iface->set_upstream_info) \
        iface->on_request_complete(self);
}
static inline bool
rp_stream_info_has_response_flag(const RpStreamInfo* self, RpCoreResponseFlag_e flag)
{
    g_return_val_if_fail(rp_stream_info_is_a(self), false);
    RpStreamInfoInterface* iface = rp_stream_info_iface(self);
    return (iface && iface->has_response_flag) ?
        iface->has_response_flag(self, flag) : false;
}

G_END_DECLS
