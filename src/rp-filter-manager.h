/*
 * rp-filter-manager.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <evhtp.h>
#include "rp-dispatcher.h"
#include "rp-http-filter.h"
#include "rp-filter-factory.h"
#include "rp-active-stream-decoder-filter.h"
#include "rp-active-stream-encoder-filter.h"
#include "rp-net-connection.h"
#include "rp-stream-reset-handler.h"
#include "lzq.h"

G_BEGIN_DECLS

typedef enum {
    RpFilterCallState_DecodeHeaders = 0x01,
    RpFilterCallState_DecodeData = 0x02,
    RpFilterCallState_DecodeMetadata = 0x04,
    RpFilterCallState_DecodeTrailers = 0x08,
    RpFilterCallState_EncodeHeaders = 0x10,
    RpFilterCallState_EncodeData = 0x20,
    RpFilterCallState_EncodeMetadata = 0x40,
    RpFilterCallState_EncodeTrailers = 0x80,
    RpFilterCallState_Encode1xxHeaders = 0x100,
    RpFilterCallState_EndOfStream = 0x200,
    RpFilterCallState_IsDecodingMask = RpFilterCallState_DecodeHeaders | RpFilterCallState_DecodeData | RpFilterCallState_DecodeMetadata | RpFilterCallState_DecodeTrailers,
    RpFilterCallState_IsEncodingMask = RpFilterCallState_EncodeHeaders | RpFilterCallState_EncodeData | RpFilterCallState_EncodeMetadata | RpFilterCallState_EncodeTrailers
} RpFilterCallState_e;

/**
 * Callbacks invoked by the FilterManager to pass filter data/events back to the caller.
 */
#define RP_TYPE_FILTER_MANAGER_CALLBACKS rp_filter_manager_callbacks_get_type()
G_DECLARE_INTERFACE(RpFilterManagerCallbacks, rp_filter_manager_callbacks, RP, FILTER_MANAGER_CALLBACKS, GObject)

struct _RpFilterManagerCallbacksInterface {
    GTypeInterface parent_iface;

    void (*encode_headers)(RpFilterManagerCallbacks*, evhtp_headers_t*, bool);
    void (*encode_1xx_headers)(RpFilterManagerCallbacks*, evhtp_headers_t*);
    void (*encode_data)(RpFilterManagerCallbacks*, evbuf_t*, bool);
    void (*encode_trailers)(RpFilterManagerCallbacks*, evhtp_headers_t*);
    void (*set_request_trailers)(RpFilterManagerCallbacks*, evhtp_headers_t*);
    void (*set_response_headers)(RpFilterManagerCallbacks*, evhtp_headers_t*);
    void (*set_response_trailers)(RpFilterManagerCallbacks*, evhtp_headers_t*);
    void (*charge_stats)(RpFilterManagerCallbacks*, evhtp_headers_t*);
    evhtp_headers_t* (*request_headers)(RpFilterManagerCallbacks*);
    evhtp_headers_t* (*request_trailers)(RpFilterManagerCallbacks*);
    evhtp_headers_t* (*response_headers)(RpFilterManagerCallbacks*);
    evhtp_headers_t* (*response_trailers)(RpFilterManagerCallbacks*);
    void (*end_stream)(RpFilterManagerCallbacks*);
    void (*send_go_away_and_close)(RpFilterManagerCallbacks*);
    void (*on_decoder_filter_below_write_buffer_low_water_mark)(RpFilterManagerCallbacks*);
    void (*on_decoder_filter_above_write_buffer_high_water_mark)(RpFilterManagerCallbacks*);
    void (*disarm_request_timeout)(RpFilterManagerCallbacks*);
    void (*reset_idle_timer)(RpFilterManagerCallbacks*);
    void (*reset_stream)(RpFilterManagerCallbacks*,
                            RpStreamResetReason_e,
                            const char*);
    RpClusterInfoConstSharedPtr (*cluster_info)(RpFilterManagerCallbacks*);
    RpUpstreamStreamFilterCallbacks* (*upstream_callbacks)(RpFilterManagerCallbacks*);
    RpDownstreamStreamFilterCallbacks* (*downstream_callbacks)(RpFilterManagerCallbacks*);
    void (*on_local_reply)(RpFilterManagerCallbacks*, evhtp_res);
    bool (*is_half_close_enabled)(RpFilterManagerCallbacks*);
    lztq* (*rules)(RpFilterManagerCallbacks*);
};

void rp_filter_manager_callbacks_encode_headers(RpFilterManagerCallbacks* self,
                                                evhtp_headers_t* response_headers,
                                                bool end_stream);
static inline void
rp_filter_manager_callbacks_encode_1xx_headers(RpFilterManagerCallbacks* self, evhtp_headers_t* response_headers)
{
    if (RP_IS_FILTER_MANAGER_CALLBACKS(self)) \
        RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->encode_1xx_headers(self, response_headers);
}
void rp_filter_manager_callbacks_encode_data(RpFilterManagerCallbacks* self,
                                                evbuf_t* data,
                                                bool end_stream);
static inline void
rp_filter_manager_callbacks_encode_trailers(RpFilterManagerCallbacks* self, evhtp_headers_t* trailers)
{
    if (RP_IS_FILTER_MANAGER_CALLBACKS(self)) \
        RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->encode_trailers(self, trailers);
}
static inline void
rp_filter_manager_callbacks_set_request_trailers(RpFilterManagerCallbacks* self,
                                            evhtp_headers_t* request_trailers)
{
    if (RP_IS_FILTER_MANAGER_CALLBACKS(self)) \
        RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->set_request_trailers(self, request_trailers);
}
void rp_filter_manager_callbacks_set_response_headers(RpFilterManagerCallbacks* self,
                                            evhtp_headers_t* response_headers);
static inline void
rp_filter_manager_callbacks_set_response_trailers(RpFilterManagerCallbacks* self,
                                            evhtp_headers_t* response_trailers)
{
    if (RP_IS_FILTER_MANAGER_CALLBACKS(self))
    {
        RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->set_response_trailers(self, response_trailers);
    }
}
static inline void
rp_filter_manager_callbacks_charge_stats(RpFilterManagerCallbacks* self,
                                            evhtp_headers_t* response_headers)
{
    if (RP_IS_FILTER_MANAGER_CALLBACKS(self)) \
        RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->charge_stats(self, response_headers);
}
evhtp_headers_t* rp_filter_manager_callbacks_request_headers(RpFilterManagerCallbacks* self);
static inline evhtp_headers_t*
rp_filter_manager_callbacks_request_trailers(RpFilterManagerCallbacks* self)
{
    return RP_IS_FILTER_MANAGER_CALLBACKS(self) ?
        RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->request_trailers(self) : NULL;
}
evhtp_headers_t* rp_filter_manager_callbacks_response_headers(RpFilterManagerCallbacks* self);
static inline evhtp_headers_t*
rp_filter_manager_callbacks_response_trailers(RpFilterManagerCallbacks* self)
{
    return RP_IS_FILTER_MANAGER_CALLBACKS(self) ?
        RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->response_trailers(self) : NULL;
}
void rp_filter_manager_callbacks_end_stream(RpFilterManagerCallbacks* self);
static inline void
rp_filter_manager_callbacks_send_go_away_and_close(RpFilterManagerCallbacks* self)
{
    if (RP_IS_FILTER_MANAGER_CALLBACKS(self)) \
        RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->send_go_away_and_close(self);
}
static inline void
rp_filter_manager_callbacks_on_decoder_filter_below_write_buffer_low_water_mark(RpFilterManagerCallbacks* self)
{
    if (RP_IS_FILTER_MANAGER_CALLBACKS(self)) \
        RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->on_decoder_filter_below_write_buffer_low_water_mark(self);
}
static inline void
rp_filter_manager_callbacks_on_decoder_filter_above_write_buffer_high_water_mark(RpFilterManagerCallbacks* self)
{
    if (RP_IS_FILTER_MANAGER_CALLBACKS(self)) \
        RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->on_decoder_filter_above_write_buffer_high_water_mark(self);
}
void rp_filter_manager_callbacks_disarm_request_timeout(RpFilterManagerCallbacks* self);
void rp_filter_manager_callbacks_reset_idle_timer(RpFilterManagerCallbacks* self);
static inline void
rp_filter_manager_callbacks_reset_stream(RpFilterManagerCallbacks* self, RpStreamResetReason_e reason, const char* transport_failure_reason)
{
    if (RP_IS_FILTER_MANAGER_CALLBACKS(self)) \
        RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->reset_stream(self,
                                                                    reason,
                                                                    transport_failure_reason);
}
static inline RpClusterInfoConstSharedPtr
rp_filter_manager_callbacks_cluster_info(RpFilterManagerCallbacks* self)
{
    return RP_IS_FILTER_MANAGER_CALLBACKS(self) ?
        RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->cluster_info(self) : NULL;
}
void rp_filter_manager_callbacks_on_local_reply(RpFilterManagerCallbacks* self,
                                                evhtp_res code);
RpUpstreamStreamFilterCallbacks* rp_filter_manager_callbacks_upstream_callbacks(RpFilterManagerCallbacks* self);
RpDownstreamStreamFilterCallbacks* rp_filter_manager_callbacks_downstream_callbacks(RpFilterManagerCallbacks* self);
static inline bool
rp_filter_manager_callbacks_is_half_close_enabled(RpFilterManagerCallbacks* self)
{
    return RP_IS_FILTER_MANAGER_CALLBACKS(self) ?
        RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->is_half_close_enabled(self) :
        false;
}
static inline lztq*
rp_filter_manager_callbacks_rules(RpFilterManagerCallbacks* self)
{
    return RP_IS_FILTER_MANAGER_CALLBACKS(self) ?
        RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->rules(self) : NULL;
}


/**
 * FilterManager manages decoding a request through a series of decoding filter and the encoding
 * of the resulting response.
 */
#define RP_TYPE_FILTER_MANAGER rp_filter_manager_get_type()
G_DECLARE_DERIVABLE_TYPE(RpFilterManager, rp_filter_manager, RP, FILTER_MANAGER, GObject)

typedef enum {
    RpFilterIterationStartState_AlwaysStartFromNext,
    RpFilterIterationStartState_CanStartFromCurrent
} RpFilterIterationStartState_e;

typedef enum {
    RpUpgradeResult_UpgradeUnneeded,
    RpUpgradeResult_UpgradeAccepted,
    RpUpgradeResult_UpgradeRejected
} RpUpgradeResult_e;

/**
 * Filter chain creation result.
 */
typedef struct RpCreateChainResult * RpCreateChainResult;
struct RpCreateChainResult {
    bool m_created;
    RpUpgradeResult_e m_upgrade;
};

static inline struct RpCreateChainResult
RpCreateChainResult_ctor(void)
{
    struct RpCreateChainResult self = {
        .m_created = false,
        .m_upgrade = RpUpgradeResult_UpgradeUnneeded
    };
    return self;
}
static inline struct RpCreateChainResult
RpCreateChainResult_ctor_(bool created, RpUpgradeResult_e upgrade)
{
    struct RpCreateChainResult self = RpCreateChainResult_ctor();
    self.m_created = created;
    self.m_upgrade = upgrade;
    return self;
}
static inline bool
RpCreateChainResult_created(RpCreateChainResult self)
{
    return self ? self->m_created : false;
}
static inline bool
RpCreateChainResult_upgrade_accepted(RpCreateChainResult self)
{
    return self ? self->m_upgrade == RpUpgradeResult_UpgradeAccepted : false;
}
static inline bool
RpCreateChainResult_upgrade_rejected(RpCreateChainResult self)
{
    return self ? self->m_upgrade == RpUpgradeResult_UpgradeRejected : false;
}

struct _RpFilterManagerClass {
    GObjectClass parent_class;

    void (*send_local_reply)(RpFilterManager*,
                                evhtp_res,
                                evbuf_t*,
                                modify_headers_cb,
                                const char*,
                                void*);
    void (*execute_local_reply_if_prepared)(RpFilterManager*);
    bool (*decoder_observed_end_stream)(RpFilterManager*);
    RpStreamInfo* (*stream_info)(RpFilterManager*);
    RpFilterManagerCallbacks* (*filter_manager_callbacks)(RpFilterManager*);
};

struct RpCreateChainResult rp_filter_manager_create_filter_chain(RpFilterManager* self,
                                    RpFilterChainFactory* filter_chain_factory);
GList** rp_filter_manager_get_decoder_filters(RpFilterManager* self);
GList** rp_filter_manager_get_encoder_filters(RpFilterManager* self);
GList** rp_filter_manager_get_filters(RpFilterManager* self);
void rp_filter_manager_on_stream_complete(RpFilterManager* self);
void rp_filter_manager_destroy_filters(RpFilterManager* self);
void rp_filter_manager_decode_headers(RpFilterManager* self,
                                        evhtp_headers_t* request_headers,
                                        bool end_stream);
void rp_filter_manager_decode_headers_(RpFilterManager* self,
                                        RpActiveStreamDecoderFilter* filter,
                                        evhtp_headers_t* request_headers,
                                        bool end_stream);
void rp_filter_manager_encode_headers_(RpFilterManager* self,
                                        RpActiveStreamEncoderFilter* filter,
                                        evhtp_headers_t* response_headers,
                                        bool end_stream);
void rp_filter_manager_decode_data(RpFilterManager* self,
                                    evbuf_t* data,
                                    bool end_stream);
void rp_filter_manager_decode_data_(RpFilterManager* self,
                                    RpActiveStreamDecoderFilter* filter,
                                    evbuf_t* data,
                                    bool end_stream,
                                    RpFilterIterationStartState_e filter_iteration_start_state);
void rp_filter_manager_decode_trailers(RpFilterManager* self,
                                        evhtp_headers_t* trailers);
void rp_filter_manager_decode_trailers_(RpFilterManager* self,
                                        RpActiveStreamDecoderFilter* filter,
                                        evhtp_headers_t* trailers);
void rp_filter_manager_encode_headers(RpFilterManager* self,
                                        evhtp_headers_t* response_headers,
                                        bool end_stream);
void rp_filter_manager_encode_data(RpFilterManager* self,
                                    evbuf_t* data,
                                    bool end_stream);
void rp_filter_manager_encode_data_(RpFilterManager* self,
                                    RpActiveStreamEncoderFilter* filter,
                                    evbuf_t* data,
                                    bool end_stream,
                                    RpFilterIterationStartState_e filter_iteration_start_state);
void rp_filter_manager_encode_trailers_(RpFilterManager* self,
                                        RpActiveStreamEncoderFilter* filter,
                                        evhtp_headers_t* trailers);
void rp_filter_manager_add_decoded_data(RpFilterManager* self,
                                        RpActiveStreamDecoderFilter* filter,
                                        evbuf_t* data,
                                        bool streaming);
void rp_filter_manager_add_encoded_data(RpFilterManager* self,
                                        RpActiveStreamEncoderFilter* filter,
                                        evbuf_t* data,
                                        bool streaming);
bool rp_filter_manager_stop_decoder_filter_chain(RpFilterManager* self);
bool rp_filter_manager_stop_encoder_filter_chain(RpFilterManager* self);
bool rp_filter_manager_is_terminal_decoder_filter(RpFilterManager* self,
                                            RpActiveStreamDecoderFilter* filter);
evbuf_t* rp_filter_manager_get_buffered_request_data(RpFilterManager* self);
void rp_filter_manager_set_buffered_request_data(RpFilterManager* self, evbuf_t* buffer);
evbuf_t* rp_filter_manager_get_buffered_response_data(RpFilterManager* self);
void rp_filter_manager_set_buffered_response_data(RpFilterManager* self, evbuf_t* buffer);
bool rp_filter_manager_destroyed(RpFilterManager* self);
bool rp_filter_manager_get_decoder_filter_chain_complete(RpFilterManager* self);
void rp_filter_manager_set_decoder_filter_chain_aborted(RpFilterManager* self);
void rp_filter_manager_set_encoder_filter_chain_aborted(RpFilterManager* self);
void rp_filter_manager_set_encoder_filter_chain_complete(RpFilterManager* self);
bool rp_filter_manager_encoder_filter_chain_complete(RpFilterManager* self);
bool rp_filter_manager_get_is_head_request(RpFilterManager* self);
bool rp_filter_manager_get_non_100_response_headers_encoded(RpFilterManager* self);
void rp_filter_manager_set_non_100_response_headers_encoded(RpFilterManager* self, bool val);
guint32 rp_filter_manager_get_filter_call_state(RpFilterManager* self);
void rp_filter_manager_set_under_on_local_reply(RpFilterManager* self, bool val);
void rp_filter_manager_set_encoder_filters_streaming(RpFilterManager* self, bool val);
void rp_filter_manager_maybe_end_encode_(RpFilterManager* self, bool end_stream);
static inline void
rp_filter_manager_send_local_reply(RpFilterManager* self, evhtp_res code, evbuf_t* body, modify_headers_cb modify_headers, const char* details, void* arg)
{
    if (RP_IS_FILTER_MANAGER(self))
    {
        RP_FILTER_MANAGER_GET_CLASS(self)->send_local_reply(self,
                                                            code,
                                                            body,
                                                            modify_headers,
                                                            details,
                                                            arg);
    }
}
static inline RpStreamInfo*
rp_filter_manager_stream_info(RpFilterManager* self)
{
    return RP_IS_FILTER_MANAGER(self) ?
        RP_FILTER_MANAGER_GET_CLASS(self)->stream_info(self) : NULL;
}
void rp_filter_manager_request_headers_initialized(RpFilterManager* self);
void rp_filter_manager_set_local_complete(RpFilterManager* self);
void rp_filter_manager_check_and_close_stream_if_fully_closed(RpFilterManager* self);
void rp_filter_manager_on_downstream_reset(RpFilterManager* self);
bool rp_filter_manager_saw_downstream_reset(RpFilterManager* self);
void rp_filter_manager_send_go_away_and_close(RpFilterManager* self);
void rp_filter_manager_set_observed_encode_end_stream(RpFilterManager* self);
bool rp_filter_manager_get_observed_encode_end_stream(RpFilterManager* self);
RpNetworkConnection* rp_filter_manager_connection(RpFilterManager* self);
bool rp_filter_manager_decoder_observed_end_stream(RpFilterManager* self);
RpDispatcher* rp_filter_manager_dispatcher(RpFilterManager* self);
guint32 rp_filter_manager_buffer_limit_(RpFilterManager* self);
guint32 rp_filter_manager_high_watermark_count_(RpFilterManager* self);
GSList** rp_filter_manager_watermark_callbacks_(RpFilterManager* self);
void rp_filter_manager_call_high_watermark_callbacks(RpFilterManager* self);
void rp_filter_manager_call_low_watermark_callbacks(RpFilterManager* self);
void rp_filter_manager_reset_stream(RpFilterManager* self,
                                    RpStreamResetReason_e reason,
                                    const char* transport_failure_reason);
static inline RpFilterManagerCallbacks*
rp_filter_manager_filter_manager_callbacks(RpFilterManager* self)
{
    return RP_IS_FILTER_MANAGER(self) ?
        RP_FILTER_MANAGER_GET_CLASS(self)->filter_manager_callbacks(self) :
        NULL;
}
RpOverrideHost rp_filter_manager_upstream_override_host_(RpFilterManager* self);

G_END_DECLS
