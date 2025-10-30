/*
 * rp-test-filter.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_test_filter_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_test_filter_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-test-filter.h"
#include "rproxy.h"

struct _RpTestFilter {
    RpPassThroughFilter parent_instance;

    request_t* m_request;
};

G_DEFINE_TYPE(RpTestFilter, rp_test_filter, RP_TYPE_PASS_THROUGH_FILTER)

enum
{
    PROP_0, // Reserved.
    PROP_REQUEST,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void
get_property_i(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p)", obj, prop_id, value, pspec);
    switch (prop_id)
    {
        case PROP_REQUEST:
            g_value_set_pointer(value, RP_TEST_FILTER(obj)->m_request);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
set_property_i(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p)", obj, prop_id, value, pspec);
    switch (prop_id)
    {
        case PROP_REQUEST:
            RP_TEST_FILTER(obj)->m_request = g_value_get_pointer(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
dispose_i(GObject* object)
{
    NOISY_MSG_("(%p)", object);
    G_OBJECT_CLASS(rp_test_filter_parent_class)->dispose(object);
}

static void
on_stream_complete_i(RpPassThroughFilter* self)
{
    LOGD("(%p)", self);
}

static void
on_destroy_i(RpPassThroughFilter* self)
{
    LOGD("(%p)", self);
    g_object_unref(self);
}

static RpFilterHeadersStatus_e
decode_headers_i(RpPassThroughFilter* self, evhtp_headers_t* request_headers, bool end_stream)
{
    LOGD("(%p, %p, %u)", self, request_headers, end_stream);
    return RP_FHS_CONTINUE;
}

static RpFilterDataStatus_e
decode_data_i(RpPassThroughFilter* self, evbuf_t* data, bool end_stream)
{
    LOGD("(%p, %p(%zu), %u)", self, data, evbuffer_get_length(data), end_stream);
    return RP_FDS_CONTINUE;
}

static void
decode_complete_i(RpPassThroughFilter* self)
{
    LOGD("(%p)", self);
}

static RpFilterHeadersStatus_e
encode_headers_i(RpPassThroughFilter* self, evhtp_headers_t* response_headers, bool end_stream)
{
    LOGD("(%p, %p, %u)", self, response_headers, end_stream);
    return RP_FHS_CONTINUE;
}

static RpFilterDataStatus_e
encode_data_i(RpPassThroughFilter* self, evbuf_t* data, bool end_stream)
{
    LOGD("(%p, %p(%zu), %u)", self, data, evbuffer_get_length(data), end_stream);
    return RP_FDS_CONTINUE;
}

static void
encode_complete_i(RpPassThroughFilter* self)
{
    LOGD("(%p)", self);
}

static void
rp_test_filter_class_init(RpTestFilterClass* klass)
{
    g_debug("%s:%s(%p) [%d]", __FILE__, __func__, klass, __LINE__);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property_i;
    object_class->set_property = set_property_i;
    object_class->dispose = dispose_i;

    RpPassThroughFilterClass* pass_through_filter_class = RP_PASS_THROUGH_FILTER_CLASS(klass);
    pass_through_filter_class->on_stream_complete = on_stream_complete_i;
    pass_through_filter_class->on_destroy = on_destroy_i;
    pass_through_filter_class->decode_headers = decode_headers_i;
    pass_through_filter_class->decode_data = decode_data_i;
    pass_through_filter_class->decode_complete = decode_complete_i;
    pass_through_filter_class->encode_headers = encode_headers_i;
    pass_through_filter_class->encode_data = encode_data_i;
    pass_through_filter_class->encode_complete = encode_complete_i;

    obj_properties[PROP_REQUEST] = g_param_spec_pointer("request",
                                                    "Request",
                                                    "RProxy request",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_test_filter_init(RpTestFilter* self)
{
    g_debug("%s:%s(%p) [%d]", __FILE__, __func__, self, __LINE__);
}

RpTestFilter*
rp_test_filter_new(request_t* request)
{
    g_debug("%s:%s(%p) [%d]", __FILE__, __func__, request, __LINE__);
    return g_object_new(RP_TYPE_TEST_FILTER, "request", request, NULL);
}
