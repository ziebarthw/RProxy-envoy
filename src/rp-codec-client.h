/*
 * rp-codec-client.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-codec.h"
#include "rp-net-connection.h"
#include "rp-stream-reset-handler.h"

G_BEGIN_DECLS

typedef struct _RpCodecClientActiveRequest RpCodecClientActiveRequest;

/**
 * Callbacks specific to a codec client.
 */
#define RP_TYPE_CODEC_CLIENT_CALLBACKS rp_codec_client_callbacks_get_type()
G_DECLARE_INTERFACE(RpCodecClientCallbacks, rp_codec_client_callbacks, RP, CODEC_CLIENT_CALLBACKS, GObject)

struct _RpCodecClientCallbacksInterface {
    GTypeInterface parent_iface;

    void (*on_stream_pre_decode_complete)(RpCodecClientCallbacks*);
    void (*on_stream_destory)(RpCodecClientCallbacks*);
    void (*on_stream_reset)(RpCodecClientCallbacks*, RpStreamResetReason_e);
};

static inline void
rp_codec_client_callbacks_on_stream_pre_decode_complete(RpCodecClientCallbacks* self)
{
    if (RP_IS_CODEC_CLIENT_CALLBACKS(self))
    {
        RP_CODEC_CLIENT_CALLBACKS_GET_IFACE(self)->on_stream_pre_decode_complete(self);
    }
}
static inline void
rp_codec_client_callbacks_on_stream_destroy(RpCodecClientCallbacks* self)
{
    if (RP_IS_CODEC_CLIENT_CALLBACKS(self))
    {
        RP_CODEC_CLIENT_CALLBACKS_GET_IFACE(self)->on_stream_destory(self);
    }
}
static inline void
rp_codec_client_callbacks_on_stream_reset(RpCodecClientCallbacks* self, RpStreamResetReason_e reason)
{
    if (RP_IS_CODEC_CLIENT_CALLBACKS(self))
    {
        RP_CODEC_CLIENT_CALLBACKS_GET_IFACE(self)->on_stream_reset(self, reason);
    }
}

/**
 * This is an HTTP client that multiple stream management and underlying connection management
 * across multiple HTTP codec types.
 */
#define RP_TYPE_CODEC_CLIENT rp_codec_client_get_type()
G_DECLARE_DERIVABLE_TYPE(RpCodecClient, rp_codec_client, RP, CODEC_CLIENT, GObject)

struct _RpCodecClientClass {
    GObjectClass parent_class;

};

void rp_codec_client_set_codec_connection_callbacks(RpCodecClient* self,
                                            RpHttpConnectionCallbacks* callbacks);
void rp_codec_client_go_away(RpCodecClient* self);
evhtp_proto rp_codec_client_protocol(RpCodecClient* self);
RpRequestEncoder* rp_codec_client_new_stream(RpCodecClient* self,
                                                RpResponseDecoder* response_decoder);
void rp_codec_client_response_pre_decode_complete(RpCodecClient* self,
                                                    RpCodecClientActiveRequest* request);
void rp_codec_client_request_encode_complete(RpCodecClient* self,
                                                RpCodecClientActiveRequest* request);
gsize rp_codec_client_num_active_requests(RpCodecClient* self);
void rp_codec_client_on_reset(RpCodecClient* self,
                                RpCodecClientActiveRequest* request,
                                RpStreamResetReason_e reason);
void rp_codec_client_on_data(RpCodecClient* self, evbuf_t* data);
bool rp_codec_client_is_half_close_enabled(RpCodecClient* self);
void rp_codec_client_close(RpCodecClient* self, RpNetworkConnectionCloseType_e type);
void rp_codec_client_close_(RpCodecClient* self);
void rp_codec_client_add_connection_callbacks(RpCodecClient* self,
                                                RpNetworkConnectionCallbacks* callbacks);
void rp_codec_client_initialize_read_filters(RpCodecClient* self);
guint64 rp_codec_client_id(RpCodecClient* self);
const char* rp_codec_client_connection_failure_reason(RpCodecClient* self);
bool rp_codec_client_remote_closed(RpCodecClient* self);
void rp_codec_client_connect(RpCodecClient* self);
RpStreamInfo* rp_codec_client_stream_info(RpCodecClient* self);
void rp_codec_client_set_requested_server_name(RpCodecClient* self,
                                                const char* requested_server_name);
RpCodecType_e rp_codec_client_type_(RpCodecClient* self);
void rp_codec_client_set_codec_(RpCodecClient* self,
                                RpHttpClientConnection* codec);

G_END_DECLS
