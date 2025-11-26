/*
 * rp-active-stream-encoder-filter.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_active_stream_encoder_filter_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_active_stream_encoder_filter_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-codec.h"
#include "rp-filter-manager.h"
#include "rp-active-stream-encoder-filter.h"

struct _RpActiveStreamEncoderFilter {
    RpActiveStreamFilterBase parent_instance;

    RpFilterManager* m_parent;
    RpStreamEncoderFilter* m_handle;
    GList* m_entry;
};

static void stream_encoder_filter_callbacks_iface_init(RpStreamEncoderFilterCallbacksInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpActiveStreamEncoderFilter, rp_active_stream_encoder_filter, RP_TYPE_ACTIVE_STREAM_FILTER_BASE,
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_ENCODER_FILTER_CALLBACKS, stream_encoder_filter_callbacks_iface_init)
)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_active_stream_encoder_filter_parent_class)->dispose(obj);
}

static void
add_encoded_data_i(RpStreamEncoderFilterCallbacks* self, evbuf_t* data, bool streaming)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuffer_get_length(data), streaming);
    RpActiveStreamEncoderFilter* me = RP_ACTIVE_STREAM_ENCODER_FILTER(self);
    rp_filter_manager_add_encoded_data(me->m_parent, me, data, streaming);
}

static void
continue_encoding_i(RpStreamEncoderFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    rp_active_stream_filter_base_common_continue(RP_ACTIVE_STREAM_FILTER_BASE(self));
}

static const evbuf_t*
encoding_buffer_i(RpStreamEncoderFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_filter_manager_get_buffered_response_data(RP_ACTIVE_STREAM_ENCODER_FILTER(self)->m_parent);
}

static void
inject_encoded_data_to_filter_chain_i(RpStreamEncoderFilterCallbacks* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuffer_get_length(data), end_stream);
    RpActiveStreamFilterBase* fb = RP_ACTIVE_STREAM_FILTER_BASE(self);
    if (!rp_active_stream_filter_base_get_headers_continued(fb))
    {
        rp_active_stream_filter_base_set_headers_continued(fb);
        rp_active_stream_filter_base_do_headers(fb, false);
    }
    RpActiveStreamEncoderFilter* me = RP_ACTIVE_STREAM_ENCODER_FILTER(self);
    rp_filter_manager_encode_data_(me->m_parent, me, data, end_stream, RpFilterIterationStartState_CanStartFromCurrent);
}

static void
stream_encoder_filter_callbacks_iface_init(RpStreamEncoderFilterCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->add_encoded_data = add_encoded_data_i;
    iface->continue_encoding = continue_encoding_i;
    iface->encoding_buffer = encoding_buffer_i;
    iface->inject_encoded_data_to_filter_chain = inject_encoded_data_to_filter_chain_i;
}

OVERRIDE void
do_headers(RpActiveStreamFilterBase* self, bool end_stream)
{
    NOISY_MSG_("(%p, %u)", self, end_stream);
    RpActiveStreamEncoderFilter* me = RP_ACTIVE_STREAM_ENCODER_FILTER(self);
    RpFilterManagerCallbacks* callbacks = rp_filter_manager_filter_manager_callbacks(me->m_parent);
    evhtp_headers_t* response_headers = rp_filter_manager_callbacks_response_headers(callbacks);
    rp_filter_manager_encode_headers_(me->m_parent, me, response_headers, end_stream);
}

OVERRIDE void
do_data(RpActiveStreamFilterBase* self, bool end_stream)
{
    NOISY_MSG_("(%p, %u)", self, end_stream);
    RpActiveStreamEncoderFilter* me = RP_ACTIVE_STREAM_ENCODER_FILTER(self);
    evbuf_t* data = rp_filter_manager_get_buffered_response_data(me->m_parent);
    rp_filter_manager_encode_data_(me->m_parent, me, data, end_stream, RpFilterIterationStartState_CanStartFromCurrent);
}

OVERRIDE evbuf_t*
buffered_data(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_filter_manager_get_buffered_response_data(RP_ACTIVE_STREAM_ENCODER_FILTER(self)->m_parent);
}

OVERRIDE bool
observed_end_stream(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_filter_manager_get_observed_encode_end_stream(RP_ACTIVE_STREAM_ENCODER_FILTER(self)->m_parent);
}

OVERRIDE bool
has_trailers(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    RpActiveStreamEncoderFilter* me = RP_ACTIVE_STREAM_ENCODER_FILTER(self);
    RpFilterManagerCallbacks* callbacks = rp_filter_manager_filter_manager_callbacks(me->m_parent);
    return rp_filter_manager_callbacks_request_trailers(callbacks) != NULL;
}

OVERRIDE evbuf_t*
create_buffer(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    evbuf_t* buffer = evbuffer_new();
    rp_filter_manager_set_buffered_response_data(RP_ACTIVE_STREAM_ENCODER_FILTER(self)->m_parent, buffer);
    return buffer;
}

OVERRIDE bool
can_continue(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    RpActiveStreamEncoderFilter* me = RP_ACTIVE_STREAM_ENCODER_FILTER(self);
    return !rp_filter_manager_encoder_filter_chain_complete(me->m_parent) &&
            (/*TODO...filter_chain_aborted_can_not_continue*/!false ||
            !rp_filter_manager_stop_encoder_filter_chain(me->m_parent));
}

static inline void
active_stream_filter_base_class_init(RpActiveStreamFilterBaseClass* klass)
{
    LOGD("(%p)", klass);
    klass->buffered_data = buffered_data;
    klass->do_headers = do_headers;
    klass->do_data = do_data;
    klass->observed_end_stream = observed_end_stream;
    klass->has_trailers = has_trailers;
    klass->create_buffer = create_buffer;
    klass->can_continue = can_continue;
}

static void
rp_active_stream_encoder_filter_class_init(RpActiveStreamEncoderFilterClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    active_stream_filter_base_class_init(RP_ACTIVE_STREAM_FILTER_BASE_CLASS(klass));
}

static void
rp_active_stream_encoder_filter_init(RpActiveStreamEncoderFilter* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpActiveStreamEncoderFilter*
constructed(RpActiveStreamEncoderFilter* self)
{
    NOISY_MSG_("(%p)", self);
    rp_stream_encoder_filter_set_encoder_filter_callbacks(self->m_handle,
                                    RP_STREAM_ENCODER_FILTER_CALLBACKS(self));
    return self;
}

RpActiveStreamEncoderFilter*
rp_active_stream_encoder_filter_new(RpFilterManager* parent, RpStreamEncoderFilter* filter, const struct RpFilterContext* filter_context)
{
    LOGD("(%p, %p, %p)", parent, filter, filter_context);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(parent), NULL);
    g_return_val_if_fail(RP_IS_STREAM_ENCODER_FILTER(filter), NULL);
    RpActiveStreamEncoderFilter* self = g_object_new(RP_TYPE_ACTIVE_STREAM_ENCODER_FILTER,
                                                        "parent", parent,
                                                        "filter-context", filter_context,
                                                        NULL);
    self->m_handle = filter;
    self->m_parent = parent;
    return constructed(self);
}

GList*
rp_active_stream_encoder_get_entry(RpActiveStreamEncoderFilter* self)
{
    g_return_val_if_fail(RP_IS_ACTIVE_STREAM_ENCODER_FILTER(self), NULL);
    return self->m_entry;
}

void
rp_active_stream_encoder_set_entry(RpActiveStreamEncoderFilter* self, GList* entry)
{
    g_return_if_fail(RP_IS_ACTIVE_STREAM_ENCODER_FILTER(self));
    self->m_entry = entry;
}

RpStreamEncoderFilter*
rp_active_stream_encoder_handle(RpActiveStreamEncoderFilter* self)
{
    g_return_val_if_fail(RP_IS_ACTIVE_STREAM_ENCODER_FILTER(self), NULL);
    return self->m_handle;
}
