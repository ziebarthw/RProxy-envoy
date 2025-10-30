/*
 * rp-rewrite-urls-filter.c
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

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include "rp-filter-factory.h"
#include "rp-filter-manager.h"
#include "rp-headers.h"
#include "rp-http-utility.h"
#include "rp-state-filter.h"
#include "rp-rewrite-urls-filter.h"

#define RP_REWRITE_URLS_FILTER_CB(s) (RpRewriteUrlsFilterCb*)(s)

#define ENCODER_FILTER_CALLBACKS(s) \
    rp_pass_through_filter_encoder_callbacks_(RP_PASS_THROUGH_FILTER(s))
#define STREAM_FILTER_CALLBACKS(s) \
    RP_STREAM_FILTER_CALLBACKS(ENCODER_FILTER_CALLBACKS(s))
#define STREAM_INFO(s) \
    rp_stream_filter_callbacks_stream_info(STREAM_FILTER_CALLBACKS(s))
#define FILTER_STATE(s) rp_stream_info_filter_state(STREAM_INFO(s))
#define ORIGINAL_URI(s) \
    rp_filter_state_get_data(FILTER_STATE(s), original_uri_key)
#define REWRITE_URLS(s) \
    rp_filter_state_get_data(FILTER_STATE(s), rewrite_urls_key)
#define PASSTHROUGH(s) \
    rp_filter_state_get_data(FILTER_STATE(s), passthrough_key)

typedef struct _RpRewriteUrlsFilterCb RpRewriteUrlsFilterCb;
struct _RpRewriteUrlsFilterCb {
    RpFilterFactoryCb parent_instance;
    RpRewriteUrlsCfg m_config;
};

struct _RpRewriteUrlsFilter {
    RpPassThroughFilter parent_instance;

    SHARED_PTR(RpRewriteUrlsCfg) m_config;

    SHARED_PTR(lztq) m_rewrite_urls;

    UNIQUE_PTR(GRegex) m_regex;
    UNIQUE_PTR(evbuf_t) m_output_buffer;
    UNIQUE_PTR(gchar) m_pattern;
    UNIQUE_PTR(gchar) m_replacement;

    bool m_rewrite_data;
};

static void stream_encoder_filter_iface_init(RpStreamEncoderFilterInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpRewriteUrlsFilter, rp_rewrite_urls_filter, RP_TYPE_PASS_THROUGH_FILTER,
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_ENCODER_FILTER, stream_encoder_filter_iface_init)
)

#define PARENT_STREAM_ENCODER_FILTER_IFACE(s) \
    ((RpStreamEncoderFilterInterface*)g_type_interface_peek_parent(RP_STREAM_ENCODER_FILTER_GET_IFACE(s)))

static inline lztq*
ensure_rewrite_urls(RpRewriteUrlsFilter* self)
{
    NOISY_MSG_("(%p)", self);
    if (self->m_rewrite_urls)
    {
        NOISY_MSG_("pre-allocated rewrite urls %p", self->m_rewrite_urls);
        return self->m_rewrite_urls;
    }
    self->m_rewrite_urls = REWRITE_URLS(self);
    NOISY_MSG_("allocated rewrite urls %p", self->m_rewrite_urls);
    return self->m_rewrite_urls;
}

static inline gchar*
create_pattern(RpRewriteUrlsFilter* self)
{
    NOISY_MSG_("(%p)", self);

    lztq* rewrite_urls = ensure_rewrite_urls(self);
    NOISY_MSG_("rewrite_urls %p, %zu urls", rewrite_urls, lztq_size(rewrite_urls));

    GString* s = g_string_new(NULL);
    gchar* sep = "";
    for (lztq_elem* elem = lztq_first(rewrite_urls); elem; elem = lztq_next(elem))
    {
        gchar* rewrite_url = lztq_elem_data(elem);
        LOGD("rewrite_url %p(%s)", rewrite_url, rewrite_url);

        g_string_append_printf(s, "%s%s", sep, rewrite_url);

        sep = "|";
    }
    NOISY_MSG_("rewrite_urls \"%s\"", s->str);

    // Release the GString object, stealing the underlying character buffer as
    // the return value.
    return g_string_free_and_steal(s);
}

static inline gchar*
create_replacement(RpRewriteUrlsFilter* self)
{
    NOISY_MSG_("(%p)", self);

    g_autofree gchar* scheme = NULL;
    g_autofree gchar* host = NULL;
    gint port;
    g_uri_split_network(ORIGINAL_URI(self), 0, &scheme, &host, &port, NULL);

    return port > 0 ? g_strdup_printf("%s://%s:%d/", scheme, host, port) :
                        g_strdup_printf("%s://%s/", scheme, host);
}

static inline GRegex*
create_regex(RpRewriteUrlsFilter* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_pattern = create_pattern(self);
    self->m_replacement = create_replacement(self);
NOISY_MSG_("replacement %p(%s)", self->m_replacement, self->m_replacement);
    return g_regex_new(self->m_pattern, G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);
}

static inline int
rewrite_header_cb(evhtp_header_t* header, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", header, arg);

    GMatchInfo* match_info;
    RpRewriteUrlsFilter* self = RP_REWRITE_URLS_FILTER(arg);
    g_regex_match_full(self->m_regex, header->val, header->vlen, 0, 0, &match_info, NULL);
    if (g_match_info_matches(match_info))
    {
        NOISY_MSG_("%d matches", g_match_info_get_match_count(match_info));
        gchar* res = g_regex_replace(self->m_regex,
                                        header->val,
                                        header->vlen,
                                        0,
                                        self->m_replacement,
                                        G_REGEX_MATCH_DEFAULT,
                                        NULL);
NOISY_MSG_("\"%.*s\" -> \"%s\"", (int)header->vlen, header->val, res);
        evhtp_header_val_set(header, res, true);
    }
    g_match_info_free(match_info);
    return 0;
}

static inline void
rewrite_headers(RpRewriteUrlsFilter* self, evhtp_headers_t* response_headers)
{
    NOISY_MSG_("(%p, %p)", self, response_headers);
    evhtp_headers_for_each(response_headers, rewrite_header_cb, self);
}

static inline bool
should_rewrite_data(evhtp_headers_t* response_headers)
{
    NOISY_MSG_("(%p)", response_headers);
    return util_is_text_content_type(
            evhtp_header_find(response_headers, RpHeaderValues.ContentType));
}

static void
rewrite_data(RpRewriteUrlsFilter* self, evbuf_t* input_buffer)
{
    NOISY_MSG_("(%p, %p(%zu))", self, input_buffer, evbuffer_get_length(input_buffer));

    gsize len = evbuffer_get_length(input_buffer);
    if (!len)
    {
        NOISY_MSG_("no data");
        return;
    }

//    NOISY_MSG_("input_buffer\n\"%.*s\"",
//        (int)evbuffer_get_length(input_buffer), (char*)evbuffer_pullup(input_buffer, -1));

    GMatchInfo* match_info;
    gchar* string = (gchar*)evbuffer_pullup(input_buffer, -1);
    g_regex_match_full(self->m_regex, string, len, 0, 0, &match_info, NULL);
    if (g_match_info_matches(match_info))
    {
        NOISY_MSG_("%d matches", g_match_info_get_match_count(match_info));
        g_autofree gchar* res = g_regex_replace(self->m_regex,
                                                string,
                                                len,
                                                0,
                                                self->m_replacement,
                                                G_REGEX_MATCH_DEFAULT,
                                                NULL);
        evbuffer_drain(input_buffer, -1);
        evbuffer_add(input_buffer, res, strlen(res));

//        NOISY_MSG_("input_buffer\n\"%.*s\"",
//            (int)evbuffer_get_length(input_buffer), (char*)evbuffer_pullup(input_buffer, -1));
    }
    g_match_info_free(match_info);

    NOISY_MSG_("done");
}

static inline evbuf_t*
ensure_output_buffer(RpRewriteUrlsFilter* self)
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


static RpFilterHeadersStatus_e
encode_headers_i(RpStreamEncoderFilter* self, evhtp_headers_t* response_headers, bool end_stream)
{
    NOISY_MSG_("(%p(%s), %p, %u)", self, G_OBJECT_TYPE_NAME(self), response_headers, end_stream);

    if (PASSTHROUGH(self))
    {
        NOISY_MSG_("passthrough");
        return RpFilterHeadersStatus_Continue;
    }

    RpRewriteUrlsFilter* me = RP_REWRITE_URLS_FILTER(self);
    if (ensure_rewrite_urls(me))
    {
        NOISY_MSG_("creating regex");
        me->m_rewrite_data = should_rewrite_data(response_headers);
        me->m_regex = create_regex(me);
        // Rewrite individual headers (as appropriate).
        rewrite_headers(me, response_headers);
    }

    return RpFilterHeadersStatus_Continue;
}
static RpFilterDataStatus_e
encode_data_i(RpStreamEncoderFilter* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuffer_get_length(data), end_stream);

    RpRewriteUrlsFilter* me = RP_REWRITE_URLS_FILTER(self);
    if (me->m_regex && me->m_rewrite_data)
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
        rewrite_data(me, data);
    }
    return RpFilterDataStatus_Continue;
}

static void
encode_complete_i(RpStreamEncoderFilter* self)
{
    NOISY_MSG_("(%p)", self);
    PARENT_STREAM_ENCODER_FILTER_IFACE(self)->encode_complete(self);
}

static RpFilterTrailerStatus_e
encode_trailers_i(RpStreamEncoderFilter* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    return PARENT_STREAM_ENCODER_FILTER_IFACE(self)->encode_trailers(self, trailers);
}

static void
set_encoder_filter_callbacks_i(RpStreamEncoderFilter* self, RpStreamEncoderFilterCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    PARENT_STREAM_ENCODER_FILTER_IFACE(self)->set_encoder_filter_callbacks(self, callbacks);
}

static void
stream_encoder_filter_iface_init(RpStreamEncoderFilterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->encode_headers = encode_headers_i;
    iface->encode_data = encode_data_i;
    iface->encode_complete = encode_complete_i;
    iface->encode_trailers = encode_trailers_i;
    iface->set_encoder_filter_callbacks = set_encoder_filter_callbacks_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpRewriteUrlsFilter* me = RP_REWRITE_URLS_FILTER(obj);
    g_clear_pointer(&me->m_regex, g_regex_unref);
    g_clear_pointer(&me->m_pattern, g_free);
    g_clear_pointer(&me->m_replacement, g_free);
    g_clear_pointer(&me->m_output_buffer, evbuffer_free);

    G_OBJECT_CLASS(rp_rewrite_urls_filter_parent_class)->dispose(obj);
}

static void
rp_rewrite_urls_filter_class_init(RpRewriteUrlsFilterClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_rewrite_urls_filter_init(RpRewriteUrlsFilter* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpRewriteUrlsFilter*
rewrite_urls_filter_new(RpRewriteUrlsCfg* config)
{
    NOISY_MSG_("(%p)", config);
    RpRewriteUrlsFilter* self = g_object_new(RP_TYPE_REWRITE_URLS_FILTER, NULL);
    self->m_config = config;
    return self;
}

static void
filter_factory_cb(RpFilterFactoryCb* self, RpFilterChainFactoryCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);

    RpRewriteUrlsFilterCb* me = RP_REWRITE_URLS_FILTER_CB(self);
    RpRewriteUrlsFilter* filter = rewrite_urls_filter_new(&me->m_config);
    rp_filter_chain_factory_callbacks_add_stream_encoder_filter(callbacks, RP_STREAM_ENCODER_FILTER(filter));
}

static inline RpRewriteUrlsFilterCb
rewrite_urls_filter_cb_ctor(RpRewriteUrlsCfg* proto_config)
{
    NOISY_MSG_("(%p)", proto_config);
    RpRewriteUrlsFilterCb self = {
        .parent_instance = rp_filter_factory_cb_ctor(filter_factory_cb, g_free),
        .m_config = *proto_config
    };
    return self;
}

static inline RpRewriteUrlsFilterCb*
rewrite_urls_filter_cb_new(RpRewriteUrlsCfg* proto_config)
{
    NOISY_MSG_("(%p)", proto_config);
    RpRewriteUrlsFilterCb* self = g_new0(RpRewriteUrlsFilterCb, 1);
    *self = rewrite_urls_filter_cb_ctor(proto_config);
    return self;
}

RpFilterFactoryCb*
rp_rewrite_urls_filter_create_filter_factory(RpRewriteUrlsCfg* proto_config, RpFactoryContext* context)
{
    LOGD("(%p, %p)", proto_config, context);
    g_return_val_if_fail(proto_config != NULL, NULL);
    g_return_val_if_fail(RP_IS_FACTORY_CONTEXT(context), NULL);
    return (RpFilterFactoryCb*)rewrite_urls_filter_cb_new(proto_config);
}
