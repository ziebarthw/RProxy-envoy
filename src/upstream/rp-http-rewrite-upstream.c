/*
 * rp-http-rewrite-upstream.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_http_rewrite_upstream_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_http_rewrite_upstream_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-headers.h"
#include "rp-http-filter.h"
#include "rp-request-rewrite.h"
#include "rp-state-filter.h"
#include "upstream/rp-http-rewrite-upstream.h"

#define DECODER_FILTER_CALLBACKS(s) \
    RP_HTTP_REWRITE_UPSTREAM(s)->m_callbacks
#define STREAM_FILTER_CALLBACKS(s) \
    RP_STREAM_FILTER_CALLBACKS(DECODER_FILTER_CALLBACKS(s))
#define STREAM_INFO(s) \
    rp_stream_filter_callbacks_stream_info(STREAM_FILTER_CALLBACKS(s))
#define FILTER_STATE(s) rp_stream_info_filter_state(STREAM_INFO(s))
#define UPSTREAM_INFO(s) rp_stream_info_upstream_info(STREAM_INFO(s))
#define UPSTREAM_HOST(s) rp_upstream_info_upstream_host(UPSTREAM_INFO(s))
#define UPSTREAM_SSL_CONNECTION(s) \
    rp_upstream_info_upstream_ssl_connection(UPSTREAM_INFO(s))
#define REWRITE_URLS(s) \
    rp_filter_state_get_data(FILTER_STATE(s), rewrite_urls_key)
#define ORIGINAL_URI(s) \
    rp_filter_state_get_data(FILTER_STATE(s), original_uri_key)
#define PASSTHROUGH(s) \
    rp_filter_state_get_data(FILTER_STATE(s), passthrough_key)

struct _RpHttpRewriteUpstream {
    GObject parent_instance;

    RpRequestRewrite m_request_rewrite;
    RpStreamDecoderFilterCallbacks* m_callbacks;
    evbuf_t* m_output_buffer;
};

static void stream_decoder_filter_iface_init(RpStreamDecoderFilterInterface* iface);
static void stream_filter_base_iface_init(RpStreamFilterBaseInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpHttpRewriteUpstream, rp_http_rewrite_upstream, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_FILTER_BASE, stream_filter_base_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_DECODER_FILTER, stream_decoder_filter_iface_init)
)

static inline evbuf_t*
ensure_output_buffer(RpHttpRewriteUpstream* self)
{
    NOISY_MSG_("(%p)", self);
    if (self->m_output_buffer)
    {
        NOISY_MSG_("pre-allocated output buffer %p", self->m_output_buffer);
        return self->m_output_buffer;
    }
    self->m_output_buffer = evbuffer_new();
    NOISY_MSG_("allocated output buffer %p", self->m_output_buffer);
    return self->m_output_buffer;
}

static void
on_destroy_i(RpStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    g_object_unref(self);
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

    if (PASSTHROUGH(self))
    {
        NOISY_MSG_("passthrough");
        return RpFilterHeadersStatus_Continue;
    }

    RpHttpRewriteUpstream* me = RP_HTTP_REWRITE_UPSTREAM(self);
    me->m_request_rewrite = rp_request_rewrite_ctor(ORIGINAL_URI(self), REWRITE_URLS(self), UPSTREAM_HOST(self), UPSTREAM_SSL_CONNECTION(self));
    if (rp_request_rewrite_active(&me->m_request_rewrite))
    {
        NOISY_MSG_("rewriting headers");
        rp_request_rewrite_process_headers(&me->m_request_rewrite, request_headers);
    }
    return RpFilterHeadersStatus_Continue;
}

static RpFilterDataStatus_e
decode_data_i(RpStreamDecoderFilter* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);

    RpHttpRewriteUpstream* me = RP_HTTP_REWRITE_UPSTREAM(self);
    if (rp_request_rewrite_active(&me->m_request_rewrite))
    {
        if (!end_stream)
        {
            NOISY_MSG_("buffering");
            evbuffer_add_buffer(ensure_output_buffer(me), data);
            return RpFilterDataStatus_StopIterationNoBuffer;
        }
        if (me->m_output_buffer)
        {
            NOISY_MSG_("prepending %zu bytes", evbuffer_get_length(me->m_output_buffer));
            evbuffer_prepend_buffer(data, me->m_output_buffer);
        }
        NOISY_MSG_("rewriting");
        rp_request_rewrite_process_data(&me->m_request_rewrite, data);
    }
    return RpFilterDataStatus_Continue;
}

static void
set_decoder_filter_callbacks_i(RpStreamDecoderFilter* self, RpStreamDecoderFilterCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    RP_HTTP_REWRITE_UPSTREAM(self)->m_callbacks = callbacks;
}

static RpFilterTrailerStatus_e
decode_trailers_i(RpStreamDecoderFilter* self G_GNUC_UNUSED, evhtp_headers_t* trailers G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    return RpFilterTrailerStatus_Continue;
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
    iface->set_decoder_filter_callbacks = set_decoder_filter_callbacks_i;
    iface->decode_trailers = decode_trailers_i;
    iface->decode_complete = decode_complete_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpHttpRewriteUpstream* self = RP_HTTP_REWRITE_UPSTREAM(obj);
    g_clear_pointer(&self->m_output_buffer, evbuffer_free);
    rp_request_rewrite_dtor(&self->m_request_rewrite);

    G_OBJECT_CLASS(rp_http_rewrite_upstream_parent_class)->dispose(obj);
}

static void
rp_http_rewrite_upstream_class_init(RpHttpRewriteUpstreamClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_http_rewrite_upstream_init(RpHttpRewriteUpstream* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpHttpRewriteUpstream*
http_rewrite_upstream_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_HTTP_REWRITE_UPSTREAM, NULL);
}

static void
filter_factory_cb(RpFilterFactoryCb* self, RpFilterChainFactoryCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);

    RpHttpRewriteUpstream* filter = http_rewrite_upstream_new();
    rp_filter_chain_factory_callbacks_add_stream_decoder_filter(callbacks, RP_STREAM_DECODER_FILTER(filter));
}

RpFilterFactoryCb*
rp_http_rewrite_filter_create_filter_factory(RpFactoryContext* context)
{
    LOGD("(%p)", context);
    g_return_val_if_fail(RP_IS_FACTORY_CONTEXT(context), NULL);
    return rp_filter_factory_cb_new(filter_factory_cb, g_free);
}
