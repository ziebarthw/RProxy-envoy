/*
 * rp-pass-through-filter.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_rewrite_urls_filter_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_rewrite_urls_filter_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-pass-through-filter.h"

#ifndef OVERRIDE
#define OVERRIDE static
#endif

typedef struct _RpPassThroughFilterPrivate RpPassThroughFilterPrivate;
struct _RpPassThroughFilterPrivate {
    RpStreamDecoderFilterCallbacks* m_decoder_callbacks;
    RpStreamEncoderFilterCallbacks* m_encoder_callbacks;
};

enum
{
    PROP_0, // Reserved.
    PROP_DECODER_CALLBACKS,
    PROP_ENCODER_CALLBACKS,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void stream_filter_base_iface_init(RpStreamFilterBaseInterface* iface);
static void stream_decoder_filter_iface_init(RpStreamDecoderFilterInterface* iface);
static void stream_encoder_filter_iface_init(RpStreamEncoderFilterInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpPassThroughFilter, rp_pass_through_filter, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpPassThroughFilter)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_FILTER_BASE, stream_filter_base_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_DECODER_FILTER, stream_decoder_filter_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_ENCODER_FILTER, stream_encoder_filter_iface_init)
)

#define PRIV(obj) \
    ((RpPassThroughFilterPrivate*) rp_pass_through_filter_get_instance_private(RP_PASS_THROUGH_FILTER(obj)))

static void
on_stream_complete_i(RpStreamFilterBase* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
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
    iface->on_stream_complete = on_stream_complete_i;
    iface->on_destroy = on_destroy_i;
}

static RpFilterHeadersStatus_e
decode_headers_i(RpStreamDecoderFilter* self G_GNUC_UNUSED, evhtp_headers_t* request_headers G_GNUC_UNUSED, bool end_stream G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p, %u)", self, request_headers, end_stream);
    return RpFilterHeadersStatus_Continue;
}

static RpFilterDataStatus_e
decode_data_i(RpStreamDecoderFilter* self G_GNUC_UNUSED, evbuf_t* data G_GNUC_UNUSED, bool end_stream G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuffer_get_length(data), end_stream);
    return RpFilterDataStatus_Continue;
}

static void
set_decoder_filter_callbacks_i(RpStreamDecoderFilter* self, RpStreamDecoderFilterCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    PRIV(self)->m_decoder_callbacks = callbacks;
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
    iface->decode_complete = decode_complete_i;
}

static RpFilterHeadersStatus_e
encode_headers_i(RpStreamEncoderFilter* self G_GNUC_UNUSED, evhtp_headers_t* response_headers G_GNUC_UNUSED, bool end_stream G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p, %u)", self, response_headers, end_stream);
    return RpFilterHeadersStatus_Continue;
}

static RpFilterDataStatus_e
encode_data_i(RpStreamEncoderFilter* self G_GNUC_UNUSED, evbuf_t* data G_GNUC_UNUSED, bool end_stream G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    return RpFilterDataStatus_Continue;
}

static void
set_encoder_filter_callbacks_i(RpStreamEncoderFilter* self, RpStreamEncoderFilterCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    PRIV(self)->m_encoder_callbacks = callbacks;
}

static void
encode_complete_i(RpStreamEncoderFilter* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static void
stream_encoder_filter_iface_init(RpStreamEncoderFilterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->encode_headers = encode_headers_i;
    iface->encode_data = encode_data_i;
    iface->set_encoder_filter_callbacks = set_encoder_filter_callbacks_i;
    iface->encode_complete = encode_complete_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_DECODER_CALLBACKS:
            g_value_set_object(value, PRIV(obj)->m_decoder_callbacks);
            break;
        case PROP_ENCODER_CALLBACKS:
            g_value_set_object(value, PRIV(obj)->m_encoder_callbacks);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_pass_through_filter_parent_class)->dispose(obj);
}

static void
rp_pass_through_filter_class_init(RpPassThroughFilterClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->dispose = dispose;

    obj_properties[PROP_DECODER_CALLBACKS] = g_param_spec_object("decoder-callbacks",
                                                    "Decoder callbacks",
                                                    "Stream Decoder Filter Callbacks",
                                                    RP_TYPE_STREAM_DECODER_FILTER_CALLBACKS,
                                                    G_PARAM_READABLE|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_ENCODER_CALLBACKS] = g_param_spec_object("encoder-callbacks",
                                                    "Encoder callbacks",
                                                    "Stream Encoder Filter Callbacks",
                                                    RP_TYPE_STREAM_ENCODER_FILTER_CALLBACKS,
                                                    G_PARAM_READABLE|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_pass_through_filter_init(RpPassThroughFilter* self)
{
    NOISY_MSG_("(%p)", self);
    RpPassThroughFilterPrivate* me = PRIV(self);
    me->m_decoder_callbacks = NULL;
    me->m_encoder_callbacks = NULL;
}

RpStreamEncoderFilterCallbacks*
rp_pass_through_filter_encoder_callbacks_(RpPassThroughFilter* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_PASS_THROUGH_FILTER(self), NULL);
    return PRIV(self)->m_encoder_callbacks;
}

RpStreamDecoderFilterCallbacks*
rp_pass_through_filter_decoder_callbacks_(RpPassThroughFilter* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_PASS_THROUGH_FILTER(self), NULL);
    return PRIV(self)->m_decoder_callbacks;
}
