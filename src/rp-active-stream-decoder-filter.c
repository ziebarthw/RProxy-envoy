/*
 * rp-active-stream-decoder-filter.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_active_stream_decoder_filter_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_active_stream_decoder_filter_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-codec.h"
#include "rp-filter-manager.h"
#include "rp-active-stream-filter-base.h"
#include "rp-active-stream-decoder-filter.h"

struct _RpActiveStreamDecoderFilter {
    RpActiveStreamFilterBase parent_instance;

    SHARED_PTR(RpFilterManager) m_parent;
    SHARED_PTR(RpStreamDecoderFilter) m_handle;
    GList* m_entry;
};

static void stream_decoder_filter_callbacks_iface_init(RpStreamDecoderFilterCallbacksInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpActiveStreamDecoderFilter, rp_active_stream_decoder_filter, RP_TYPE_ACTIVE_STREAM_FILTER_BASE,
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_DECODER_FILTER_CALLBACKS, stream_decoder_filter_callbacks_iface_init)
)

static void
add_decoded_data_i(RpStreamDecoderFilterCallbacks* self, evbuf_t* data, bool streaming)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuffer_get_length(data), streaming);
    RpActiveStreamDecoderFilter* me = RP_ACTIVE_STREAM_DECODER_FILTER(self);
    rp_filter_manager_add_decoded_data(me->m_parent, me, data, streaming);
}

static void
continue_decoding_i(RpStreamDecoderFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    rp_active_stream_filter_base_common_continue(RP_ACTIVE_STREAM_FILTER_BASE(self));
}

static const evbuf_t*
decoding_buffer_i(RpStreamDecoderFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_filter_manager_get_buffered_request_data(RP_ACTIVE_STREAM_DECODER_FILTER(self)->m_parent);
}

static void
inject_decoded_data_to_filter_chain_i(RpStreamDecoderFilterCallbacks* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuffer_get_length(data), end_stream);
    RpActiveStreamFilterBase* fb = RP_ACTIVE_STREAM_FILTER_BASE(self);
    if (!rp_active_stream_filter_base_get_headers_continued(fb))
    {
        rp_active_stream_filter_base_set_headers_continued(fb);
        rp_active_stream_filter_base_do_headers(fb, false);
    }
    RpActiveStreamDecoderFilter* me = RP_ACTIVE_STREAM_DECODER_FILTER(self);
    rp_filter_manager_decode_data_(me->m_parent, me, data, end_stream, RpFilterIterationStartState_CanStartFromCurrent);
}

static void
stop_decoding_if_non_terminal_filter_encoded_end_stream(RpActiveStreamDecoderFilter* self, bool encoded_end_stream)
{
    NOISY_MSG_("(%p, %u)", self, encoded_end_stream);
    RpActiveStreamDecoderFilter* me = RP_ACTIVE_STREAM_DECODER_FILTER(self);
    if (encoded_end_stream &&
        !rp_filter_manager_is_terminal_decoder_filter(me->m_parent, self) &&
        !rp_filter_manager_get_decoder_filter_chain_complete(me->m_parent))
    {
        rp_filter_manager_set_decoder_filter_chain_aborted(me->m_parent);
    }
}

static void
encode_headers_i(RpStreamDecoderFilterCallbacks* self, evhtp_headers_t* response_headers, bool end_stream, const char* details)
{
    NOISY_MSG_("(%p, %p, %u, %p)", self, response_headers, end_stream, details);

    RpActiveStreamDecoderFilter* me = RP_ACTIVE_STREAM_DECODER_FILTER(self);
    stop_decoding_if_non_terminal_filter_encoded_end_stream(me, end_stream);
    //TODO: parent_.streamInfo().setResponseCodeDetails(details);
    RpFilterManagerCallbacks* callbacks = rp_filter_manager_filter_manager_callbacks(me->m_parent);
    rp_filter_manager_callbacks_set_response_headers(callbacks, response_headers);
    rp_filter_manager_encode_headers_(me->m_parent, NULL, rp_filter_manager_callbacks_response_headers(callbacks), end_stream);
}

static void
encode_data_i(RpStreamDecoderFilterCallbacks* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuffer_get_length(data), end_stream);

    RpActiveStreamDecoderFilter* me = RP_ACTIVE_STREAM_DECODER_FILTER(self);
    stop_decoding_if_non_terminal_filter_encoded_end_stream(me, end_stream);
    rp_filter_manager_encode_data_(me->m_parent, NULL, data, end_stream, RpFilterIterationStartState_CanStartFromCurrent);
}

static void
encode_trailers_i(RpStreamDecoderFilterCallbacks* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);

    RpActiveStreamDecoderFilter* me = RP_ACTIVE_STREAM_DECODER_FILTER(self);
    stop_decoding_if_non_terminal_filter_encoded_end_stream(me, true);
    RpFilterManagerCallbacks* callbacks = rp_filter_manager_filter_manager_callbacks(me->m_parent);
    rp_filter_manager_callbacks_set_response_trailers(callbacks, trailers);
    rp_filter_manager_encode_trailers_(me->m_parent, NULL, trailers);
}

static void
send_local_reply_i(RpStreamDecoderFilterCallbacks* self, evhtp_res code, evbuf_t* body, modify_headers_cb modify_headers, const char* details, void* arg)
{
    NOISY_MSG_("(%p, %d, %p(%zu), %p, %p(%s), %p)",
        self, code, body, body ? evbuffer_get_length(body) : 0, modify_headers, details, details, arg);
    rp_active_stream_filter_base_send_local_reply(RP_ACTIVE_STREAM_FILTER_BASE(self), code, body, modify_headers, details, arg);
}

static guint32
decoder_buffer_limit_i(RpStreamDecoderFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_filter_manager_buffer_limit_(RP_ACTIVE_STREAM_DECODER_FILTER(self)->m_parent);
}

static void
add_downstream_watermark_callbacks_i(RpStreamDecoderFilterCallbacks* self, RpDownstreamWatermarkCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    RpActiveStreamDecoderFilter* me = RP_ACTIVE_STREAM_DECODER_FILTER(self);
    GSList** watermark_callbacks_ = rp_filter_manager_watermark_callbacks_(me->m_parent);
    guint32 high_watermark_count_ = rp_filter_manager_high_watermark_count_(me->m_parent);
    *watermark_callbacks_ = g_slist_append(*watermark_callbacks_, callbacks);
    for (guint32 i = 0; i < high_watermark_count_; ++i)
    {
        rp_downstream_watermark_callbacks_on_above_write_buffer_high_watermark(callbacks);
    }
}

static void
remove_downstream_watermark_callbacks_i(RpStreamDecoderFilterCallbacks* self, RpDownstreamWatermarkCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    RpActiveStreamDecoderFilter* me = RP_ACTIVE_STREAM_DECODER_FILTER(self);
    GSList** watermark_callbacks_ = rp_filter_manager_watermark_callbacks_(me->m_parent);
    *watermark_callbacks_ = g_slist_remove(*watermark_callbacks_, callbacks);
}

static RpOverrideHost
upstream_override_host_i(RpStreamDecoderFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    RpActiveStreamDecoderFilter* me = RP_ACTIVE_STREAM_DECODER_FILTER(self);
    return rp_filter_manager_upstream_override_host_(me->m_parent);
}

static void
stream_decoder_filter_callbacks_iface_init(RpStreamDecoderFilterCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->add_decoded_data = add_decoded_data_i;
    iface->continue_decoding = continue_decoding_i;
    iface->decoding_buffer = decoding_buffer_i;
    iface->inject_decoded_data_to_filter_chain = inject_decoded_data_to_filter_chain_i;
    iface->encode_headers = encode_headers_i;
    iface->encode_data = encode_data_i;
    iface->encode_trailers = encode_trailers_i;
    iface->send_local_reply = send_local_reply_i;
    iface->decoder_buffer_limit = decoder_buffer_limit_i;
    iface->add_downstream_watermark_callbacks = add_downstream_watermark_callbacks_i;
    iface->remove_downstream_watermark_callbacks = remove_downstream_watermark_callbacks_i;
    iface->upstream_override_host = upstream_override_host_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_active_stream_decoder_filter_parent_class)->dispose(obj);
}

OVERRIDE void
do_headers(RpActiveStreamFilterBase* self, bool end_stream)
{
    NOISY_MSG_("(%p, %u)", self, end_stream);
    RpActiveStreamDecoderFilter* me = RP_ACTIVE_STREAM_DECODER_FILTER(self);
    RpFilterManagerCallbacks* callbacks = rp_filter_manager_filter_manager_callbacks(me->m_parent);
    evhtp_headers_t* request_headers = rp_filter_manager_callbacks_request_headers(callbacks);
    rp_filter_manager_decode_headers_(me->m_parent, me, request_headers, end_stream);
}

OVERRIDE void
do_data(RpActiveStreamFilterBase* self, bool end_stream)
{
    NOISY_MSG_("(%p, %u)", self, end_stream);
    RpActiveStreamDecoderFilter* me = RP_ACTIVE_STREAM_DECODER_FILTER(self);
    evbuf_t* data = rp_filter_manager_get_buffered_request_data(me->m_parent);
    rp_filter_manager_decode_data_(me->m_parent, me, data, end_stream, RpFilterIterationStartState_CanStartFromCurrent);
}

OVERRIDE void
do_trailers(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    RpActiveStreamDecoderFilter* me = RP_ACTIVE_STREAM_DECODER_FILTER(self);
    RpFilterManagerCallbacks* callbacks = rp_filter_manager_filter_manager_callbacks(me->m_parent);
    evhtp_headers_t* trailers = rp_filter_manager_callbacks_request_trailers(callbacks);
    rp_filter_manager_decode_trailers_(me->m_parent, me, trailers);
}

OVERRIDE bool
has_trailers(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    RpActiveStreamDecoderFilter* me = RP_ACTIVE_STREAM_DECODER_FILTER(self);
    RpFilterManagerCallbacks* callbacks = rp_filter_manager_filter_manager_callbacks(me->m_parent);
    return rp_filter_manager_callbacks_request_trailers(callbacks) != NULL;
}

OVERRIDE evbuf_t*
buffered_data(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_filter_manager_get_buffered_request_data(RP_ACTIVE_STREAM_DECODER_FILTER(self)->m_parent);
}

OVERRIDE evbuf_t*
create_buffer(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    evbuf_t* buffer = evbuffer_new();
    rp_filter_manager_set_buffered_request_data(RP_ACTIVE_STREAM_DECODER_FILTER(self)->m_parent, buffer);
    return buffer;
}

OVERRIDE bool
observed_end_stream(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_filter_manager_decoder_observed_end_stream(RP_ACTIVE_STREAM_DECODER_FILTER(self)->m_parent);
}

OVERRIDE bool
can_continue(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    return !rp_filter_manager_stop_decoder_filter_chain(RP_ACTIVE_STREAM_DECODER_FILTER(self)->m_parent);
}

static void
active_stream_filter_base_class_init(RpActiveStreamFilterBaseClass* klass)
{
    LOGD("(%p)", klass);
    klass->buffered_data = buffered_data;
    klass->create_buffer = create_buffer;
    klass->do_headers = do_headers;
    klass->do_data = do_data;
    klass->do_trailers = do_trailers;
    klass->has_trailers = has_trailers;
    klass->observed_end_stream = observed_end_stream;
    klass->can_continue = can_continue;
}

static void
rp_active_stream_decoder_filter_class_init(RpActiveStreamDecoderFilterClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    active_stream_filter_base_class_init(RP_ACTIVE_STREAM_FILTER_BASE_CLASS(klass));
}

static void
rp_active_stream_decoder_filter_init(RpActiveStreamDecoderFilter* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpActiveStreamDecoderFilter*
constructed(RpActiveStreamDecoderFilter* self)
{
    NOISY_MSG_("(%p)", self);
    rp_stream_decoder_filter_set_decoder_filter_callbacks(self->m_handle,
                                    RP_STREAM_DECODER_FILTER_CALLBACKS(self));
    return self;
}

RpActiveStreamDecoderFilter*
rp_active_stream_decoder_filter_new(RpFilterManager* parent, RpStreamDecoderFilter* filter, const struct RpFilterContext* filter_context)
{
    LOGD("(%p, %p, %p)", parent, filter, filter_context);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(parent), NULL);
    g_return_val_if_fail(RP_IS_STREAM_DECODER_FILTER(filter), NULL);
    RpActiveStreamDecoderFilter* self = g_object_new(RP_TYPE_ACTIVE_STREAM_DECODER_FILTER,
                        "parent", RP_FILTER_MANAGER(parent),
                        "filter-context", filter_context,
                        NULL);
    self->m_handle = filter;
    self->m_parent = parent;
    return constructed(self);
}

GList*
rp_active_stream_decoder_get_entry(RpActiveStreamDecoderFilter* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_ACTIVE_STREAM_DECODER_FILTER(self), NULL);
    return self->m_entry;
}

void
rp_active_stream_decoder_set_entry(RpActiveStreamDecoderFilter* self, GList* entry)
{
    NOISY_MSG_("(%p, %p)", self, entry);
    g_return_if_fail(RP_IS_ACTIVE_STREAM_DECODER_FILTER(self));
    self->m_entry = entry;
}

RpFilterHeadersStatus_e
rp_active_stream_decoder_decode_headers(RpActiveStreamDecoderFilter* self,
                                                evhtp_headers_t* request_headers,
                                                bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, request_headers, end_stream);
    g_return_val_if_fail(RP_IS_ACTIVE_STREAM_DECODER_FILTER(self), RpFilterHeadersStatus_Continue);
    g_return_val_if_fail(request_headers != NULL, RpFilterHeadersStatus_Continue);
    return rp_stream_decoder_filter_decode_headers(self->m_handle,
                                                    request_headers,
                                                    end_stream);
}

RpFilterDataStatus_e
rp_active_stream_decoder_decode_data(RpActiveStreamDecoderFilter* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    g_return_val_if_fail(RP_IS_STREAM_DECODER_FILTER(self), RpFilterDataStatus_Continue);
    return rp_stream_decoder_filter_decode_data(self->m_handle, data, end_stream);
}

RpStreamDecoderFilter*
rp_active_stream_decoder_handle(RpActiveStreamDecoderFilter* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_ACTIVE_STREAM_DECODER_FILTER(self), NULL);
    return self->m_handle;
}
