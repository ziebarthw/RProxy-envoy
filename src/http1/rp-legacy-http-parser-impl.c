/*
 * rp-legacy-http-parser-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_legacy_http_parser_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_legacy_http_parser_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "evhtp/parser.h"
#include "rproxy.h"
#include "rp-legacy-http-parser-impl.h"

struct _RpLegacyHttpParserImpl {
    GObject parent_instance;

    htparser* m_parser;
    RpMessageType_e m_type;
    RpParserCallbacks* m_callbacks;
    GString* m_request_uri;
    bool m_paused;
};

static void parser_iface_init(RpParserInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpLegacyHttpParserImpl, rp_legacy_http_parser_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_PARSER, parser_iface_init)
)

static inline void
g_string_free_and_clear(GString** strp)
{
    if (strp && *strp)
    {
        g_string_free(*strp, TRUE);
        *strp = NULL;
    }
}

static guint64
content_length_i(RpParser* self)
{
    NOISY_MSG_("(%p)", self);
    return htparser_get_content_length(RP_LEGACY_HTTP_PARSER_IMPL(self)->m_parser);
}

static const char*
error_message_i(RpParser* self)
{
    NOISY_MSG_("(%p)", self);
    return htparser_get_strerror(RP_LEGACY_HTTP_PARSER_IMPL(self)->m_parser);
}

static int
on_msg_begin(htparser* self)
{
    NOISY_MSG_("(%p)", self);
    RpLegacyHttpParserImpl* me = htparser_get_userdata(self);
    if (me->m_request_uri) g_string_truncate(me->m_request_uri, 0);
    return rp_parser_callbacks_on_message_begin(me->m_callbacks);
}

static inline GString*
ensure_request_uri(RpLegacyHttpParserImpl* self)
{
    NOISY_MSG_("(%p)", self);
    if (self->m_request_uri)
    {
        NOISY_MSG_("pre-allocated request uri %p", self->m_request_uri);
        return self->m_request_uri;
    }
    self->m_request_uri = g_string_sized_new(64/*REVISIT-arbitrary*/);
    NOISY_MSG_("allocated request uri %p", self->m_request_uri);
    return self->m_request_uri;
}

static inline int
append_request_uri(htparser* self, const char* data, size_t length, bool insert_port_sep, bool insert_query_sep)
{
    NOISY_MSG_("(%p, %p(%.*s), %zu, %u, %u)", self, data, (int)length, data, length, insert_port_sep, insert_query_sep);
    GString* request_uri = ensure_request_uri(RP_LEGACY_HTTP_PARSER_IMPL(htparser_get_userdata(self)));
    if (insert_port_sep) g_string_append_c(request_uri, ':');
    if (insert_query_sep) g_string_append_c(request_uri, '?');
//if (length != 1 || data[0] != '/' || request_uri->len != 1 || request_uri->str[0] != '/')
    g_string_append_len(request_uri, data, length);
NOISY_MSG_("request uri \"%s\"", request_uri->str);
    return RpCallbackResult_Success;
}

static int
scheme_cb(htparser* self, const char* data, size_t length)
{
    NOISY_MSG_("(%p, %p(%.*s), %zu)", self, data, (int)length, data, length);
    int rval = append_request_uri(self, data, length, false, false);
    GString* request_uri = ensure_request_uri(RP_LEGACY_HTTP_PARSER_IMPL(htparser_get_userdata(self)));
    g_string_append_len(request_uri, "://", 3);
    return rval;
}

static int
host_cb(htparser* self, const char* data, size_t length)
{
    NOISY_MSG_("(%p, %p(%.*s), %zu)", self, data, (int)length, data, length);
    return append_request_uri(self, data, length, false, false);
}

static int
port_cb(htparser* self, const char* data, size_t length)
{
    NOISY_MSG_("(%p, %p(%.*s), %zu)", self, data, (int)length, data, length);
    return append_request_uri(self, data, length, true, false);
}

static int
path_cb(htparser* self, const char* data, size_t length)
{
    NOISY_MSG_("(%p, %p(%.*s), %zu)", self, data, (int)length, data, length);
    return htparser_get_method(self) != htp_method_CONNECT ?
            append_request_uri(self, data, length, false, false) :
            RpCallbackResult_Success;
}

static int
args_cb(htparser* self, const char* data, size_t length)
{
    NOISY_MSG_("(%p, %p(%.*s), %zu)", self, data, (int)length, data, length);
    return append_request_uri(self, data, length, false, true);
}

static int
on_hdrs_begin(htparser* self)
{
    NOISY_MSG_("(%p)", self);
    RpLegacyHttpParserImpl* me = htparser_get_userdata(self);
    return me->m_type == RpMessageType_Request ?
            rp_parser_callbacks_on_url(me->m_callbacks, me->m_request_uri->str, me->m_request_uri->len) :
            RpCallbackResult_Success;
}

static int
hdr_key_cb(htparser* self, const char* data, size_t length)
{
    NOISY_MSG_("(%p, %p(%.*s), %zu)", self, data, (int)length, data, length);
    RpLegacyHttpParserImpl* me = htparser_get_userdata(self);
    return rp_parser_callbacks_on_header_field(me->m_callbacks, data, length);
}

static int
hdr_val_cb(htparser* self, const char* data, size_t length)
{
    NOISY_MSG_("(%p, %p(%.*s), %zu)", self, data, (int)length, data, length);
    RpLegacyHttpParserImpl* me = htparser_get_userdata(self);
    return rp_parser_callbacks_on_header_value(me->m_callbacks, data, length);
}

static int
on_hdrs_complete(htparser* self)
{
    NOISY_MSG_("(%p)", self);
    RpLegacyHttpParserImpl* me = htparser_get_userdata(self);
    return rp_parser_callbacks_on_headers_complete(me->m_callbacks);
}

static int
on_new_chunk(htparser* self)
{
    NOISY_MSG_("(%p)", self);
    RpLegacyHttpParserImpl* me = htparser_get_userdata(self);
    rp_parser_callbacks_on_chunk_header(me->m_callbacks, false);
    return 0;
}

static int
on_chunk_complete(htparser* self)
{
    NOISY_MSG_("(%p)", self);
    NOISY_MSG_("%zu bytes", htparser_get_content_length(self));
    return 0;
}

static int
on_chunks_complete(htparser* self)
{
    NOISY_MSG_("(%p)", self);
    RpLegacyHttpParserImpl* me = htparser_get_userdata(self);
    rp_parser_callbacks_on_chunk_header(me->m_callbacks, true);
    return 0;
}

static int
body_cb(htparser* self, const char* data, size_t length)
{
    NOISY_MSG_("(%p, %p, %zu)", self, data, length);
    RpLegacyHttpParserImpl* me = htparser_get_userdata(self);
    rp_parser_callbacks_buffer_body(me->m_callbacks, data, length);
    return 0;
}

static int
on_msg_complete(htparser* self)
{
    NOISY_MSG_("(%p)", self);
    RpLegacyHttpParserImpl* me = htparser_get_userdata(self);
    return rp_parser_callbacks_on_message_complete(me->m_callbacks);
}

static size_t
execute_i(RpParser* self, const char* data, int length)
{
    static htparse_hooks parser_hooks = {
        .on_msg_begin = on_msg_begin,
        .method = NULL,
        .scheme = scheme_cb,
        .host = host_cb,
        .port = port_cb,
        .path = path_cb,
        .args = args_cb,
        .uri = NULL,
        .on_hdrs_begin = on_hdrs_begin,
        .hdr_key = hdr_key_cb,
        .hdr_val = hdr_val_cb,
        .hostname = NULL,
        .on_hdrs_complete = on_hdrs_complete,
        .on_new_chunk = on_new_chunk,
        .on_chunk_complete = on_chunk_complete,
        .on_chunks_complete = on_chunks_complete,
        .body = body_cb,
        .on_msg_complete = on_msg_complete
    };

    NOISY_MSG_("(%p, %p, %d)", self, data, length);
    RpLegacyHttpParserImpl* me = RP_LEGACY_HTTP_PARSER_IMPL(self);
    return !me->m_paused ? htparser_run(me->m_parser, &parser_hooks, data, length) : 0;
}

static inline RpParserStatus_e
error_to_status(htpparse_error error)
{
    switch (error)
    {
        case htparse_error_none:
            return RpParserStatus_Ok;
        default:
            return RpParserStatus_Error;
    }
}

static RpParserStatus_e
get_status_i(RpParser* self)
{
    NOISY_MSG_("(%p)", self);
    RpLegacyHttpParserImpl* me = RP_LEGACY_HTTP_PARSER_IMPL(self);
    return me->m_paused ?
        RpParserStatus_Paused :
        error_to_status(htparser_get_error(me->m_parser));
}

static int
has_transfer_encoding_i(RpParser* self)
{
    NOISY_MSG_("(%p)", self);
    return htparser_uses_transfer_encoding(RP_LEGACY_HTTP_PARSER_IMPL(self)->m_parser);
}

static bool
is_chunked_i(RpParser* self)
{
    NOISY_MSG_("(%p)", self);
    return htparser_is_chunked(RP_LEGACY_HTTP_PARSER_IMPL(self)->m_parser);
}

static bool
is_http_11_i(RpParser* self)
{
    htparser* p = RP_LEGACY_HTTP_PARSER_IMPL(self)->m_parser;
    return htparser_get_major(p) == 1 && htparser_get_minor(p) == 1;
}

static const char*
method_name_i(RpParser* self)
{
    return htparser_get_methodstr(RP_LEGACY_HTTP_PARSER_IMPL(self)->m_parser);
}

static RpCallbackResult_e
pause_i(RpParser* self G_GNUC_UNUSED)
{
    RP_LEGACY_HTTP_PARSER_IMPL(self)->m_paused = true;
    return RpCallbackResult_Success;
}

static void
resume_i(RpParser* self G_GNUC_UNUSED)
{
    RP_LEGACY_HTTP_PARSER_IMPL(self)->m_paused = false;
}

static evhtp_res
status_code_i(RpParser* self)
{
    return htparser_get_status(RP_LEGACY_HTTP_PARSER_IMPL(self)->m_parser);
}

static bool
should_keep_alive_i(RpParser* self)
{
    return htparser_should_keep_alive(RP_LEGACY_HTTP_PARSER_IMPL(self)->m_parser);
}

static void
parser_iface_init(RpParserInterface* iface)
{
    LOGD("(%p)", iface);
    iface->content_length = content_length_i;
    iface->error_message = error_message_i;
    iface->execute = execute_i;
    iface->get_status = get_status_i;
    iface->has_transfer_encoding = has_transfer_encoding_i;
    iface->is_chunked = is_chunked_i;
    iface->is_http_11 = is_http_11_i;
    iface->method_name = method_name_i;
    iface->pause = pause_i;
    iface->resume = resume_i;
    iface->status_code = status_code_i;
    iface->should_keep_alive = should_keep_alive_i;
}

static inline struct htparser*
create_parser(RpMessageType_e type)
{
    struct htparser* self = htparser_new();
    g_assert(self != NULL);

    htparser_init(self,
        type == RpMessageType_Request ? htp_type_request : htp_type_response);
    return self;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpLegacyHttpParserImpl* self = RP_LEGACY_HTTP_PARSER_IMPL(obj);
    g_string_free_and_clear(&self->m_request_uri);
    g_clear_pointer(&self->m_parser, free);
    self->m_callbacks = NULL;

    G_OBJECT_CLASS(rp_legacy_http_parser_impl_parent_class)->dispose(obj);
}

static void
rp_legacy_http_parser_impl_class_init(RpLegacyHttpParserImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_legacy_http_parser_impl_init(RpLegacyHttpParserImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpLegacyHttpParserImpl*
constructed(RpLegacyHttpParserImpl* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_parser = create_parser(self->m_type);
    htparser_set_userdata(self->m_parser, self);
    return self;
}

RpLegacyHttpParserImpl*
rp_legacy_http_parser_impl_new(RpMessageType_e type, RpParserCallbacks* callbacks)
{
    LOGD("(%d, %p)", type, callbacks);
    RpLegacyHttpParserImpl* self = g_object_new(RP_TYPE_LEGACY_HTTP_PARSER_IMPL, NULL);
    self->m_type = type;
    self->m_callbacks = callbacks;
    return constructed(self);
}
