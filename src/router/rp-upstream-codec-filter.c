/*
 * rp-upstream-codec-filter.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_upstream_codec_filter_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_upstream_codec_filter_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "router/rp-codec-bridge.h"
#include "router/rp-upstream-codec-filter.h"

#define CALLBACKS(s) RP_UPSTREAM_CODEC_FILTER(s)->m_callbacks
#define STREAM_FILTER_CALLBACKS(s) RP_STREAM_FILTER_CALLBACKS(CALLBACKS(s))
#define UPSTREAM_STREAM_FILTER_CALLBACKS(s) \
    rp_stream_filter_callbacks_upstream_callbacks(STREAM_FILTER_CALLBACKS(s))
#define GENERIC_UPSTREAM(s) \
    rp_upstream_stream_filter_callbacks_upstream(UPSTREAM_STREAM_FILTER_CALLBACKS(s))
#define STREAM_INFO(s) \
    rp_stream_filter_callbacks_stream_info(STREAM_FILTER_CALLBACKS(s))

struct _RpUpstreamCodecFilter {
    GObject parent_instance;

    RpStreamDecoderFilterCallbacks* m_callbacks;
    RpCodecBridge* m_bridge;
    evhtp_headers_t* m_latched_headers;
    bool* m_latched_end_stream_;
    RpStatusCode_e m_deferred_reset_status;
    bool m_calling_encode_headers;
    bool m_latched_end_stream;
};

static void stream_filter_base_iface_init(RpStreamFilterBaseInterface* iface);
static void stream_decoder_filter_iface_init(RpStreamDecoderFilterInterface* iface);
static void downstream_watermark_callbacks_iface_init(RpDownstreamWatermarkCallbacksInterface* iface);
static void upstream_callbacks_iface_init(RpUpstreamCallbacksInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpUpstreamCodecFilter, rp_upstream_codec_filter, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_FILTER_BASE, stream_filter_base_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_DECODER_FILTER, stream_decoder_filter_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_DOWNSTREAM_WATERMARK_CALLBACKS, downstream_watermark_callbacks_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_UPSTREAM_CALLBACKS, upstream_callbacks_iface_init)
)

static void
on_destroy_i(RpStreamFilterBase* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    rp_stream_decoder_filter_callbacks_remove_downstream_watermark_callbacks(RP_UPSTREAM_CODEC_FILTER(self)->m_callbacks,
                                                                                RP_DOWNSTREAM_WATERMARK_CALLBACKS(self));
}

static void
stream_filter_base_iface_init(RpStreamFilterBaseInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_destroy = on_destroy_i;
}

static RpFilterHeadersStatus_e
decode_headers_i(RpStreamDecoderFilter* self, evhtp_headers_t* request_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, request_headers, end_stream);
    RpUpstreamCodecFilter* me = RP_UPSTREAM_CODEC_FILTER(self);
    if (!rp_upstream_stream_filter_callbacks_upstream(UPSTREAM_STREAM_FILTER_CALLBACKS(self)))
    {
        me->m_latched_headers = request_headers;
        me->m_latched_end_stream = end_stream;
        me->m_latched_end_stream_ = &me->m_latched_end_stream;
        return RpFilterHeadersStatus_StopAllIterationAndWatermark;
    }

    me->m_calling_encode_headers = true;
    RpStatusCode_e status = rp_generic_upstream_encode_headers(GENERIC_UPSTREAM(self),
                                                                request_headers,
                                                                end_stream);

    me->m_calling_encode_headers = false;
    if ((status != RpStatusCode_Ok) || (me->m_deferred_reset_status != RpStatusCode_Ok))
    {
        rp_stream_info_set_response_flag(STREAM_INFO(self), RpCoreResponseFlag_DownstreamProtocolError);
        //TODO...
        me->m_deferred_reset_status = RpStatusCode_Ok;
        rp_stream_decoder_filter_callbacks_send_local_reply(me->m_callbacks, EVHTP_RES_SERVUNAVAIL, NULL, NULL, "", self);
        return RpFilterHeadersStatus_StopIteration;
    }
    RpUpstreamTiming* timing = rp_upstream_codec_filter_upstream_timing(me);
    rp_upstream_timing_on_first_upstream_tx_byte_sent(timing);

    if (end_stream)
    {
        rp_upstream_timing_on_last_upstream_tx_byte_sent(timing);
    }
    RpUpstreamStreamFilterCallbacks* upstream_stream_filter_callbacks = UPSTREAM_STREAM_FILTER_CALLBACKS(self);
    if (rp_upstream_stream_filter_callbacks_paused_for_connect(upstream_stream_filter_callbacks))
    {
        return RpFilterHeadersStatus_StopAllIterationAndWatermark;
    }
    else if (rp_upstream_stream_filter_callbacks_paused_for_web_socket_upgrade(upstream_stream_filter_callbacks))
    {
        return RpFilterHeadersStatus_StopAllIterationAndWatermark;
    }
    return RpFilterHeadersStatus_Continue;
}

static RpFilterDataStatus_e
decode_data_i(RpStreamDecoderFilter* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuf_length(data), end_stream);
    //TODO...
    RpUpstreamCodecFilter* me = RP_UPSTREAM_CODEC_FILTER(self);
    rp_generic_upstream_encode_data(GENERIC_UPSTREAM(self), data, end_stream);
    if (end_stream)
    {
        rp_upstream_timing_on_last_upstream_tx_byte_sent(
            rp_upstream_codec_filter_upstream_timing(me));
    }
    return RpFilterDataStatus_Continue;
}

static RpFilterTrailerStatus_e
decode_trailers_i(RpStreamDecoderFilter* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    RpUpstreamCodecFilter* me = RP_UPSTREAM_CODEC_FILTER(self);
    rp_generic_upstream_encode_trailers(GENERIC_UPSTREAM(self), trailers);
    rp_upstream_timing_on_last_upstream_tx_byte_sent(
        rp_upstream_codec_filter_upstream_timing(me));
    return RpFilterTrailerStatus_Continue;
}

static void
set_decoder_filter_callbacks_i(RpStreamDecoderFilter* self, RpStreamDecoderFilterCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    RpUpstreamCodecFilter* me = RP_UPSTREAM_CODEC_FILTER(self);
    me->m_callbacks = callbacks;
    rp_stream_decoder_filter_callbacks_add_downstream_watermark_callbacks(me->m_callbacks,
                                                                            RP_DOWNSTREAM_WATERMARK_CALLBACKS(me));
    RpUpstreamStreamFilterCallbacks* upstream_stream_filter_callbacks = UPSTREAM_STREAM_FILTER_CALLBACKS(self);
    rp_upstream_stream_filter_callbacks_add_upstream_callbacks(upstream_stream_filter_callbacks, RP_UPSTREAM_CALLBACKS(self));
    rp_upstream_stream_filter_callbacks_set_upstream_to_downstream(upstream_stream_filter_callbacks, RP_UPSTREAM_TO_DOWNSTREAM(me->m_bridge));
}

static void
decode_complete_i(RpStreamDecoderFilter* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static void
stream_decoder_filter_iface_init(RpStreamDecoderFilterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->decode_headers = decode_headers_i;
    iface->decode_data = decode_data_i;
    iface->decode_trailers = decode_trailers_i;
    iface->set_decoder_filter_callbacks = set_decoder_filter_callbacks_i;
    iface->decode_complete = decode_complete_i;
}

static void
on_above_write_buffer_high_watermark_i(RpDownstreamWatermarkCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    //TODO...
    rp_generic_upstream_read_disable(GENERIC_UPSTREAM(self), false);
}

static void
on_below_write_buffer_low_watermark_i(RpDownstreamWatermarkCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    //TODO...
    rp_generic_upstream_read_disable(GENERIC_UPSTREAM(self), true);
}

static void
downstream_watermark_callbacks_iface_init(RpDownstreamWatermarkCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_above_write_buffer_high_watermark = on_above_write_buffer_high_watermark_i;
    iface->on_below_write_buffer_low_watermark = on_below_write_buffer_low_watermark_i;
}

static void
on_upstream_connection_established_i(RpUpstreamCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpUpstreamCodecFilter* me = RP_UPSTREAM_CODEC_FILTER(self);
    if (me->m_latched_end_stream_)
    {
        bool end_stream = *me->m_latched_end_stream_;
        me->m_latched_end_stream_ = NULL;
        RpFilterHeadersStatus_e status = decode_headers_i(RP_STREAM_DECODER_FILTER(self), me->m_latched_headers, end_stream);
        if (status == RpFilterHeadersStatus_Continue)
        {
            rp_stream_decoder_filter_callbacks_continue_decoding(me->m_callbacks);
        }
    }
}

static void
upstream_callbacks_iface_init(RpUpstreamCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_upstream_connection_established = on_upstream_connection_established_i;
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_upstream_codec_filter_parent_class)->constructed(obj);

    RpUpstreamCodecFilter* self = RP_UPSTREAM_CODEC_FILTER(obj);
    self->m_bridge = rp_codec_bridge_new(self);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpUpstreamCodecFilter* self = RP_UPSTREAM_CODEC_FILTER(obj);
    g_clear_object(&self->m_bridge);

    G_OBJECT_CLASS(rp_upstream_codec_filter_parent_class)->dispose(obj);
}

static void
rp_upstream_codec_filter_class_init(RpUpstreamCodecFilterClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = constructed;
    object_class->dispose = dispose;
}

static void
rp_upstream_codec_filter_init(RpUpstreamCodecFilter* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_deferred_reset_status = RpStatusCode_Ok;
    self->m_calling_encode_headers = false;
    self->m_latched_end_stream_ = NULL;
}

static inline RpUpstreamCodecFilter*
upstream_codec_filter_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_UPSTREAM_CODEC_FILTER, NULL);
}

static void
filter_factory_cb(RpFilterFactoryCb* self, RpFilterChainFactoryCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);

    RpUpstreamCodecFilter* filter = upstream_codec_filter_new();
    rp_filter_chain_factory_callbacks_add_stream_decoder_filter(callbacks, RP_STREAM_DECODER_FILTER(filter));
}

RpFilterFactoryCb*
rp_upstream_codec_filter_create_filter_factory(RpFactoryContext* context)
{
    LOGD("(%p)", context);
    g_return_val_if_fail(RP_IS_FACTORY_CONTEXT(context), NULL);
    return rp_filter_factory_cb_new(filter_factory_cb, g_free);
}

RpUpstreamTiming*
rp_upstream_codec_filter_upstream_timing(RpUpstreamCodecFilter* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_UPSTREAM_CODEC_FILTER(self), NULL);

    return rp_upstream_info_upstream_timing(
                rp_stream_info_upstream_info(STREAM_INFO(self)));
}

RpStreamDecoderFilterCallbacks*
rp_upstream_codec_filter_callbacks_(RpUpstreamCodecFilter* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_UPSTREAM_CODEC_FILTER(self), NULL);
    return self->m_callbacks;
}

bool
rp_upstream_codec_filter_calling_encode_headers_(RpUpstreamCodecFilter* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_UPSTREAM_CODEC_FILTER(self), false);
    return self->m_calling_encode_headers;
}
