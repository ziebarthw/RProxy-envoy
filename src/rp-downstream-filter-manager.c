/*
 * rp-downstream-filter-manager.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_downstream_filter_manager_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_downstream_filter_manager_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-headers.h"
#include "router/rp-router-filter-interface.h"
#include "stream_info/rp-stream-info-impl.h"
#include "rp-downstream-filter-manager.h"

#define FILTER_MANAGER_CALLBACKS(s) \
    rp_filter_manager_filter_manager_callbacks(RP_FILTER_MANAGER(s))

G_DEFINE_INTERFACE(RpLocalReply, rp_local_reply, G_TYPE_OBJECT)

static void
rp_local_reply_default_rewrite(RpLocalReply* self G_GNUC_UNUSED, const evhtp_headers_t* request_headers G_GNUC_UNUSED, evhtp_headers_t* response_headers G_GNUC_UNUSED, evhtp_res code G_GNUC_UNUSED, evbuf_t* body G_GNUC_UNUSED, const char* content_type G_GNUC_UNUSED)
{
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static void
rp_local_reply_default_init(RpLocalReplyInterface* iface)
{
    iface->rewrite = rp_local_reply_default_rewrite;
}

static inline void
rp_local_reply_rewrite(RpLocalReply* self, const evhtp_headers_t* request_headers, evhtp_headers_t* response_headers, evhtp_res code, evbuf_t* body, const char* content_type)
{
    NOISY_MSG_("(%p, %p, %p, %d, %p(%zu), %p(%s))",
        self, request_headers, response_headers, code, body, body ? evbuffer_get_length(body) : 0, content_type, content_type);
    if (RP_IS_LOCAL_REPLY(self))
    {
        RP_LOCAL_REPLY_GET_IFACE(self)->rewrite(self,
                                                request_headers,
                                                response_headers,
                                                code,
                                                body,
                                                content_type);
    }
}

struct RpEncodeFunctions {
    modify_headers_cb m_modify_headers;
    rewrite_cb m_rewrite;
    encode_headers_cb m_encode_headers;
    encode_data_cb m_encode_data;
    void* m_arg;
};
static inline struct RpEncodeFunctions
RpEncodeFunctions_ctor(modify_headers_cb modify_headers, rewrite_cb rewrite,
                        encode_headers_cb encode_headers, encode_data_cb encode_data, void* arg)
{
    struct RpEncodeFunctions self = {
        .m_modify_headers = modify_headers,
        .m_rewrite = rewrite,
        .m_encode_headers = encode_headers,
        .m_encode_data = encode_data,
        .m_arg = arg
    };
    return self;
}

// source/common/http/utility.h
struct RpUtility_LocalReplyData {
    evhtp_res m_response_code;
    evbuf_t* m_body_text;
    bool m_is_head_request;
};
static inline struct RpUtility_LocalReplyData
RpUtility_LocalReplyData_ctor(evhtp_res response_code, evbuf_t* body_text, bool is_head_request)
{
    struct RpUtility_LocalReplyData self = {
        .m_response_code = response_code,
        .m_body_text = body_text,
        .m_is_head_request = is_head_request
    };
    return self;
}

typedef struct RpPreparedLocalReply * RpPreparedLocalReplyPtr;
struct RpPreparedLocalReply {
    bool m_is_head_request;
    evhtp_headers_t* m_response_headers;
    evbuf_t* m_response_body;
    encode_headers_cb m_encode_headers;
    encode_data_cb m_encode_data;
    void* m_arg;
};

static inline struct RpPreparedLocalReply
RpPreparedLocalReply_ctor(bool is_head_request, evhtp_headers_t* response_headers,
                            evbuf_t* response_body, encode_headers_cb encode_headers, encode_data_cb encode_data, void* arg)
{
    struct RpPreparedLocalReply self = {
        .m_is_head_request = is_head_request,
        .m_response_headers = response_headers,
        .m_response_body = response_body,
        .m_encode_headers = encode_headers,
        .m_encode_data = encode_data,
        .m_arg = arg
    };
    return self;
}
static inline RpPreparedLocalReplyPtr
RpPreparedLocalReply_new(bool is_head_request, evhtp_headers_t* response_headers,
                            evbuf_t* response_body, encode_headers_cb encode_headers, encode_data_cb encode_data, void* arg)
{
    NOISY_MSG_("(%u, %p, %p(%zu), %p, %p, %p)",
        is_head_request, response_headers, response_body, response_body ? evbuffer_get_length(response_body) :0,
        encode_headers, encode_data, arg);
    RpPreparedLocalReplyPtr self = g_new(struct RpPreparedLocalReply, 1);
    *self = RpPreparedLocalReply_ctor(is_head_request,
                                        response_headers,
                                        response_body,
                                        encode_headers,
                                        encode_data,
                                        arg);
    return self;
}
static inline void
RpPreparedLocalReply_free(RpPreparedLocalReplyPtr self)
{
    g_clear_pointer(&self, g_free);
}

struct _RpDownstreamFilterManager {
    RpFilterManager parent_instance;

    //...stream_info_;
    RpLocalReply* m_local_reply;
    RpFilterChainFactory* m_filter_chain_factory;
    RpFilterState* m_parent_filter_state;
    RpPreparedLocalReplyPtr m_prepared_local_reply;
    RpStreamInfoImpl* m_stream_info;
    modify_headers_cb m_modify_headers;
    void* m_modify_headers_arg;

    bool m_use_filter_manager_state_for_downstream_end_stream : 1;
};

G_DEFINE_FINAL_TYPE(RpDownstreamFilterManager, rp_downstream_filter_manager, RP_TYPE_FILTER_MANAGER)

static inline RpConnectionInfoProvider*
connection_info_provider(RpDownstreamFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_network_connection_connection_info_provider(
                rp_filter_manager_connection(RP_FILTER_MANAGER(self)));
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpDownstreamFilterManager* self = RP_DOWNSTREAM_FILTER_MANAGER(obj);
NOISY_MSG_("stream info %p(%u)", self->m_stream_info, G_OBJECT(self->m_stream_info)->ref_count);
    g_clear_object(&self->m_stream_info);

    G_OBJECT_CLASS(rp_downstream_filter_manager_parent_class)->dispose(obj);
}

static inline char*
int_to_string(int val, char* buf, size_t buf_size)
{
    snprintf(buf, buf_size, "%d", val);
    return buf;
}

static inline evhtp_header_t*
create_status_header(evhtp_res code)
{
    char buf[256];
    return evhtp_header_new(RpHeaderValues.Status,
                            int_to_string(code, buf, sizeof(buf)),
                            0,/*no copy name*/
                            1/*copy value*/);
}

static inline const char*
get_content_type(evhtp_headers_t* headers)
{
    NOISY_MSG_("(%p)", headers);
    return evhtp_kv_find(headers, RpHeaderValues.ContentType);
}

static inline void
remove_content_type(evhtp_headers_t* headers)
{
    NOISY_MSG_("(%p)", headers);
    evhtp_header_rm_and_free(headers,
        evhtp_headers_find_header(headers, RpHeaderValues.ContentType));
}

static inline evhtp_header_t*
create_reference_content_type_header(const char* content_type)
{
    NOISY_MSG_("(%p(%s))", content_type, content_type);
    return evhtp_header_new(RpHeaderValues.ContentType,
                            content_type,
                            0,/*no copy name*/
                            0/*no copy value*/);
}

static inline void
set_reference_content_type(evhtp_headers_t* headers, const char* content_type)
{
    NOISY_MSG_("(%p, %p(%s))", headers, content_type, content_type);
    evhtp_header_rm_and_free(headers,
        evhtp_headers_find_header(headers, RpHeaderValues.ContentType));
    evhtp_headers_add_header(headers,
        create_reference_content_type_header(content_type));
}

static inline char*
size_to_string(size_t val, char* buf, size_t buf_size)
{
    snprintf(buf, buf_size, "%zu", val);
    return buf;
}

static inline evhtp_header_t*
create_content_length_header(size_t content_length)
{
    char buf[256];
    return evhtp_header_new(RpHeaderValues.ContentLength,
                            size_to_string(content_length, buf, sizeof(buf)),
                            0,/*no copy name*/
                            1/*copy value*/);
}

static inline void
set_content_length(evhtp_headers_t* headers, size_t content_length)
{
    evhtp_header_rm_and_free(headers,
        evhtp_headers_find_header(headers, RpHeaderValues.ContentLength));
    evhtp_headers_add_header(headers,
        create_content_length_header(content_length));
}

static inline void
remove_content_length(evhtp_headers_t* headers)
{
    evhtp_header_rm_and_free(headers,
        evhtp_headers_find_header(headers, RpHeaderValues.ContentLength));
}

static RpPreparedLocalReplyPtr
utility_prepare_local_reply(const struct RpEncodeFunctions* encode_functions,
                            const struct RpUtility_LocalReplyData* local_reply_data)
{
    NOISY_MSG_("(%p, %p)", encode_functions, local_reply_data);

    evhtp_res response_code = local_reply_data->m_response_code;
    evbuf_t* body_text = local_reply_data->m_body_text;
    const char* content_type = RpHeaderValues.ContentTypeValues.Text;

    evhtp_headers_t* response_headers = evhtp_headers_new();
    evhtp_headers_add_header(response_headers, create_status_header(response_code));

    if (encode_functions->m_modify_headers)
    {
        NOISY_MSG_("calling modify_headers(%p, %p)", response_headers, encode_functions->m_arg);
        encode_functions->m_modify_headers(response_headers, encode_functions->m_arg);
    }
    bool has_custom_content_type = false;
    if (encode_functions->m_rewrite)
    {
        const char* content_type_value = get_content_type(response_headers);
        NOISY_MSG_("calling rewrite(%p, %d, %p(%zu), %p(%s), %p)",
            response_headers, response_code, body_text, body_text ? evbuffer_get_length(body_text) : 0,
            content_type, content_type, encode_functions->m_arg);
        encode_functions->m_rewrite(response_headers, response_code, body_text, content_type, encode_functions->m_arg);
        const char* rev_content_type_value = get_content_type(response_headers);
        has_custom_content_type = (content_type_value && rev_content_type_value) &&
                                    (content_type_value != rev_content_type_value);
    }

    if (body_text && evbuffer_get_length(body_text) > 0)
    {
        set_content_length(response_headers, evbuffer_get_length(body_text));
        if (!evhtp_kv_find(response_headers, RpHeaderValues.ContentType) ||
            (body_text != local_reply_data->m_body_text && !has_custom_content_type))
        {
            set_reference_content_type(response_headers, content_type);
        }
    }
    else
    {
        remove_content_length(response_headers);
        remove_content_type(response_headers);
        set_content_length(response_headers, 0);
    }

    return RpPreparedLocalReply_new(local_reply_data->m_is_head_request,
                                    response_headers,
                                    body_text,
                                    encode_functions->m_encode_headers,
                                    encode_functions->m_encode_data,
                                    encode_functions->m_arg);
}

static void
encode_local_reply(bool is_reset, RpPreparedLocalReplyPtr prepared_local_reply)
{
    NOISY_MSG_("(%u, %p)", is_reset, prepared_local_reply);
    evhtp_headers_t* response_headers = prepared_local_reply->m_response_headers;
    void* arg = prepared_local_reply->m_arg;

    if (prepared_local_reply->m_is_head_request)
    {
        prepared_local_reply->m_encode_headers(response_headers, true, arg);
        return;
    }

    bool bodyless_response = !prepared_local_reply->m_response_body || evbuffer_get_length(prepared_local_reply->m_response_body) == 0;
    prepared_local_reply->m_encode_headers(response_headers, bodyless_response, arg);
    if (!bodyless_response && !is_reset)
    {
        evbuf_t* buffer = prepared_local_reply->m_response_body;
        prepared_local_reply->m_encode_data(buffer, true, arg);
    }
}

static void
send_local_reply_(bool is_reset, const struct RpEncodeFunctions* encode_functions,
                    const struct RpUtility_LocalReplyData* local_reply_data)
{
    NOISY_MSG_("(%u, %p, %p)", is_reset, encode_functions, local_reply_data);
    RpPreparedLocalReplyPtr prepared_local_reply = utility_prepare_local_reply(encode_functions, local_reply_data);
    encode_local_reply(is_reset, prepared_local_reply);
    RpPreparedLocalReply_free(prepared_local_reply);
}

static void
modify_headers_(evhtp_headers_t* response_headers, void* arg)
{
    NOISY_MSG_("(%p, %p)", response_headers, arg);
    RpDownstreamFilterManager* self = arg;
    //TODO:...
    if (self->m_modify_headers)
    {
        NOISY_MSG_("calling modify_headers(%p, %p)", response_headers, self->m_modify_headers_arg);
        self->m_modify_headers(response_headers, self->m_modify_headers_arg);
    }
}

static void
rewrite_(evhtp_headers_t* response_headers, evhtp_res response_code, evbuf_t* body_text, const char* content_type, void* arg)
{
    NOISY_MSG_("(%p, %d, %p(%zu), %p(%s), %p)",
        response_headers, response_code, body_text, body_text ? evbuffer_get_length(body_text) : 0, content_type, content_type, arg);
    RpDownstreamFilterManager* self = arg;
    if (self->m_local_reply)
    {
        evhtp_headers_t* request_headers = rp_filter_manager_callbacks_request_headers(FILTER_MANAGER_CALLBACKS(self));
NOISY_MSG_("calling rp_local_reply_rewrite(%p, %p, %p, %d, %p(%zu), %p(%s))",
    self->m_local_reply, request_headers, response_headers, response_code, body_text, body_text ? evbuffer_get_length(body_text) : 0, content_type, content_type);
        rp_local_reply_rewrite(self->m_local_reply,
                                request_headers,
                                response_headers,
                                response_code,
                                body_text,
                                content_type);
    }
}

static void
encode_headers_(evhtp_headers_t* response_headers, bool end_stream, void* arg)
{
    NOISY_MSG_("(%p, %u, %p)", response_headers, end_stream, arg);
    RpDownstreamFilterManager* self = arg;
    rp_filter_manager_callbacks_set_response_headers(FILTER_MANAGER_CALLBACKS(self), response_headers);
    rp_filter_manager_encode_headers_(RP_FILTER_MANAGER(self), NULL, response_headers, end_stream);
}

static void
encode_data_(evbuf_t* data, bool end_stream, void* arg)
{
    NOISY_MSG_("(%p(%zu), %u, %p)", data, evbuffer_get_length(data), end_stream, arg);
    RpDownstreamFilterManager* self = arg;
    rp_filter_manager_encode_data_(RP_FILTER_MANAGER(self), NULL, data, end_stream, RpFilterIterationStartState_CanStartFromCurrent);
}

static void
prepare_local_reply_via_filter_chain(RpDownstreamFilterManager* self, evhtp_res code, evbuf_t* body,
                                        modify_headers_cb modify_headers, bool is_head_request,
                                        const char* details, void* arg)
{
    NOISY_MSG_("(%p, %d, %p(%zu), %p, %u, %p, %p)",
        self, code, body, body ? evbuffer_get_length(body) : 0, modify_headers, is_head_request, details, arg);

    rp_downstream_filter_manager_create_downstream_filter_chain(self);

    if (self->m_prepared_local_reply)
    {
        return;
    }

    self->m_modify_headers = modify_headers;
    self->m_modify_headers_arg = arg;

    struct RpEncodeFunctions encode_functions = RpEncodeFunctions_ctor(modify_headers_,
                                                                        rewrite_,
                                                                        encode_headers_,
                                                                        encode_data_,
                                                                        self);
    struct RpUtility_LocalReplyData local_reply_data = RpUtility_LocalReplyData_ctor(code, body, is_head_request);
    self->m_prepared_local_reply = utility_prepare_local_reply(&encode_functions,
                                                                &local_reply_data);
}

static void
send_local_reply_via_filter_chain(RpDownstreamFilterManager* self, evhtp_res code, evbuf_t* body,
                                    modify_headers_cb modify_headers, bool is_head_request, const char* details, void* arg)
{
    NOISY_MSG_("(%p, %d, %p(%zu), %p, %u, %p, %p)",
        self, code, body, body ? evbuffer_get_length(body) : 0, modify_headers, is_head_request, details, arg);

    rp_downstream_filter_manager_create_downstream_filter_chain(self);

    self->m_modify_headers = modify_headers;
    self->m_modify_headers_arg = arg;

    bool is_reset = rp_filter_manager_destroyed(RP_FILTER_MANAGER(self));
    struct RpEncodeFunctions encode_functions = RpEncodeFunctions_ctor(modify_headers_,
                                                                        rewrite_,
                                                                        encode_headers_,
                                                                        encode_data_,
                                                                        self);
    struct RpUtility_LocalReplyData local_reply_data = RpUtility_LocalReplyData_ctor(code, body, is_head_request);
    send_local_reply_(is_reset, &encode_functions, &local_reply_data);
}

static void
encode_headers__(evhtp_headers_t* response_headers, bool end_stream, void* arg)
{
    NOISY_MSG_("(%p, %u, %p)", response_headers, end_stream, arg);

    RpFilterManager* self = arg;
    RpFilterManagerCallbacks* callbacks = FILTER_MANAGER_CALLBACKS(self);
    rp_filter_manager_callbacks_set_response_headers(callbacks, response_headers);

    rp_filter_manager_set_non_100_response_headers_encoded(self, true);
    rp_filter_manager_callbacks_encode_headers(callbacks, response_headers, end_stream);
    if (rp_filter_manager_saw_downstream_reset(self))
    {
        return;
    }
    rp_filter_manager_maybe_end_encode_(self, end_stream);
}

static void
encode_data__(evbuf_t* data, bool end_stream, void* arg)
{
    NOISY_MSG_("(%p(%zu), %u, %p)", data, evbuffer_get_length(data), end_stream, arg);
    RpFilterManager* self = arg;
    rp_filter_manager_callbacks_encode_data(FILTER_MANAGER_CALLBACKS(self), data, end_stream);
    if (rp_filter_manager_saw_downstream_reset(self))
    {
        return;
    }
    rp_filter_manager_maybe_end_encode_(self, end_stream);
}

static void
send_direct_local_reply(RpDownstreamFilterManager* self, evhtp_res code, evbuf_t* body, modify_headers_cb modify_headers, bool is_head_request, void* arg)
{
    NOISY_MSG_("(%p, %d, %p(%zu), %p, %u, %p)",
        self, code, body, body ? evbuffer_get_length(body) : 0, modify_headers, is_head_request, arg);

    self->m_modify_headers = modify_headers;
    self->m_modify_headers_arg = arg;

    rp_filter_manager_set_encoder_filters_streaming(RP_FILTER_MANAGER(self), true);
    bool is_reset = rp_filter_manager_destroyed(RP_FILTER_MANAGER(self));
    struct RpEncodeFunctions encode_functions = RpEncodeFunctions_ctor(modify_headers_,
                                                                        rewrite_,
                                                                        encode_headers__,
                                                                        encode_data__,
                                                                        self);
    struct RpUtility_LocalReplyData local_reply_data = RpUtility_LocalReplyData_ctor(code, body, is_head_request);
    send_local_reply_(is_reset, &encode_functions, &local_reply_data);
}

static void
on_local_reply(RpFilterManager* self, struct RpStreamFilterBase_LocalReplyData* data)
{
    NOISY_MSG_("(%p, %p)", self, data);

    rp_filter_manager_set_under_on_local_reply(self, true);
    rp_filter_manager_callbacks_on_local_reply(FILTER_MANAGER_CALLBACKS(self), data->m_code);

    GList** filters = rp_filter_manager_get_filters(self);
NOISY_MSG_("filters %p(%p)", filters, filters ? *filters : NULL);
    for (GList* entry = *filters; entry; entry = entry->next)
    {
        RpStreamFilterBase* filter = RP_STREAM_FILTER_BASE(entry->data);
        NOISY_MSG_("filter %p", filter);
        if (rp_stream_filter_base_on_local_reply(filter, data) == RpLocalErrorStatus_ContinueAndResetStream)
        {
            data->m_reset_imminent = true;
        }
    }
    rp_filter_manager_set_under_on_local_reply(self, false);
}

OVERRIDE void
send_local_reply(RpFilterManager* self, evhtp_res code, evbuf_t* body, modify_headers_cb modify_headers, const char* details, void* arg)
{
    NOISY_MSG_("(%p, %d, %p(%zu), %p, %p, %p)",
        self, code, body, body ? evbuffer_get_length(body) : 0, modify_headers, details, arg);

    bool is_head_request = rp_filter_manager_get_is_head_request(self);
    guint32 filter_call_state = rp_filter_manager_get_filter_call_state(self);
    if (filter_call_state & RpFilterCallState_IsDecodingMask)
    {
        rp_filter_manager_set_decoder_filter_chain_aborted(self);
    }
    else if (filter_call_state & RpFilterCallState_IsEncodingMask)
    {
        rp_filter_manager_set_encoder_filter_chain_aborted(self);
    }

    if (rp_filter_manager_callbacks_is_half_close_enabled(FILTER_MANAGER_CALLBACKS(self)))
    {
        rp_filter_manager_set_decoder_filter_chain_aborted(self);
    }

    //TODO:streamInfo().setResponseCodeDetails(details);
    struct RpStreamFilterBase_LocalReplyData data = RpStreamFilterBase_LocalReplyData_ctor(code, details, false);
    on_local_reply(self, &data);
    if (data.m_reset_imminent)
    {
        NOISY_MSG_("reset imminent");
        rp_filter_manager_callbacks_reset_stream(FILTER_MANAGER_CALLBACKS(self), RpStreamResetReason_LocalReset, "");
        return;
    }

    if (!rp_filter_manager_callbacks_request_headers(FILTER_MANAGER_CALLBACKS(self))
        /*TODO: || ...*/)
    {
        if (filter_call_state & RpFilterCallState_IsDecodingMask)
        {
            prepare_local_reply_via_filter_chain(RP_DOWNSTREAM_FILTER_MANAGER(self),
                                                    code,
                                                    body,
                                                    modify_headers,
                                                    is_head_request,
                                                    details,
                                                    arg);
        }
        else
        {
            send_local_reply_via_filter_chain(RP_DOWNSTREAM_FILTER_MANAGER(self),
                                                code,
                                                body,
                                                modify_headers,
                                                is_head_request,
                                                details,
                                                arg);
        }
    }
    else if (!rp_filter_manager_get_non_100_response_headers_encoded(self))
    {
        send_direct_local_reply(RP_DOWNSTREAM_FILTER_MANAGER(self),
                                code,
                                body,
                                modify_headers,
                                is_head_request,
                                arg);
    }
    else
    {
        rp_filter_manager_callbacks_reset_stream(FILTER_MANAGER_CALLBACKS(self), RpStreamResetReason_LocalReset, "");
    }
}

OVERRIDE RpStreamInfo*
stream_info(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_STREAM_INFO(RP_DOWNSTREAM_FILTER_MANAGER(self)->m_stream_info);
}

OVERRIDE bool
decoder_observed_end_stream(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    RpDownstreamFilterManager* me = RP_DOWNSTREAM_FILTER_MANAGER(self);
    if (me->m_use_filter_manager_state_for_downstream_end_stream)
    {
        return rp_filter_manager_decoder_observed_end_stream(self);
    }
    return rp_downstream_filter_manager_has_last_downstream_byte_received(me);
}

static void
filter_manager_class_init(RpFilterManagerClass* klass)
{
    LOGD("(%p)", klass);
    klass->send_local_reply = send_local_reply;
    klass->stream_info = stream_info;
    klass->decoder_observed_end_stream = decoder_observed_end_stream;
}

static void
rp_downstream_filter_manager_class_init(RpDownstreamFilterManagerClass* klass)
{
    LOGD("(%p)]", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    filter_manager_class_init(RP_FILTER_MANAGER_CLASS(klass));
}

static void
rp_downstream_filter_manager_init(RpDownstreamFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_prepared_local_reply = NULL;
    self->m_use_filter_manager_state_for_downstream_end_stream = true;//TODO...runtime feature setting...
}

static inline RpDownstreamFilterManager*
constructed(RpDownstreamFilterManager* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_stream_info = rp_stream_info_impl_new(EVHTP_PROTO_INVALID,
                                                    connection_info_provider(self),
                                                    RpFilterStateLifeSpan_FilterChain,
                                                    g_steal_pointer(&self->m_parent_filter_state));
    return self;
}

RpDownstreamFilterManager*
rp_downstream_filter_manager_new(RpFilterManagerCallbacks* filter_manager_callbacks,
                                    RpDispatcher* dispatcher, RpNetworkConnection* connection,
                                    guint64 stream_id, bool proxy_100_continue, guint32 buffer_limit,
                                    RpFilterChainFactory* filter_chain_factory,
                                    RpLocalReply* local_reply, evhtp_proto protocol, RpFilterState* parent_filter_state)
{
    NOISY_MSG_("(%p, %p, %zu, %u, %u, %p, %p, %d, %p)",
        filter_manager_callbacks, connection, stream_id, proxy_100_continue, buffer_limit, filter_chain_factory, local_reply, protocol, parent_filter_state);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER_CALLBACKS(filter_manager_callbacks), NULL);
    g_return_val_if_fail(dispatcher != NULL, NULL);
    g_return_val_if_fail(RP_IS_NETWORK_CONNECTION(connection), NULL);
    g_return_val_if_fail(RP_IS_FILTER_CHAIN_FACTORY(filter_chain_factory), NULL);
    RpDownstreamFilterManager* self = g_object_new(RP_TYPE_DOWNSTREAM_FILTER_MANAGER,
                        "dispatcher", dispatcher,
                        "filter-manager-callbacks", filter_manager_callbacks,
                        "connection", connection,
                        "proxy-100-continue", proxy_100_continue,
                        "buffer-limit", buffer_limit,
                        NULL);
    self->m_filter_chain_factory = filter_chain_factory;
    self->m_local_reply = local_reply;
    self->m_parent_filter_state = parent_filter_state;
    return constructed(self);
}

struct RpCreateChainResult
rp_downstream_filter_manager_create_downstream_filter_chain(RpDownstreamFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_filter_manager_create_filter_chain(RP_FILTER_MANAGER(self),
                                                    self->m_filter_chain_factory);
}

bool
rp_downstream_filter_manager_has_last_downstream_byte_received(RpDownstreamFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_DOWNSTREAM_FILTER_MANAGER(self), false);
    RpStreamInfo* stream_info = rp_filter_manager_stream_info(RP_FILTER_MANAGER(self));
    RpDownstreamTiming* downstream_timing = rp_stream_info_downstream_timing(stream_info);
    return downstream_timing &&
        rp_downstream_timing_last_downstream_rx_byte_received(downstream_timing) > 0;
}
