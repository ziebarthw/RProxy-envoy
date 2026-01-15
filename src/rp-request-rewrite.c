/*
 * rp-request-rewrite.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_request_rewrite_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_request_rewrite_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-headers.h"
#include "rp-http-utility.h"
#include "rp-request-rewrite.h"

typedef struct _RewriteHeaderCtx RewriteHeaderCtx;
struct _RewriteHeaderCtx {
    GRegex* m_regex;
    gchar* m_replacement;
    gsize m_host_len;
    gsize m_origin_len;
    bool m_saw_host : 1;
    bool m_saw_origin : 1;
};

static inline RewriteHeaderCtx
rewrite_header_ctx_ctor(GRegex* regex, gchar* replacement)
{
    RewriteHeaderCtx self = {
        .m_regex = regex,
        .m_replacement = replacement,
        .m_host_len = strlen(RpHeaderValues.HostLegacy),
        .m_saw_host = false,
        .m_origin_len = strlen(RpCustomHeaderValues.Origin),
        .m_saw_origin = false
    };
    return self;
}

static inline gchar*
create_pattern(RpRequestRewrite* self)
{
    NOISY_MSG_("(%p)", self);

    g_autofree gchar* scheme = NULL;
    gint port;
    g_uri_split_network(self->m_original_uri, 0, &scheme, &self->m_host, &port, NULL);
    return port != -1 ?
        g_strdup_printf("%s://%s:%d/", scheme, self->m_host, port) :
        g_strdup_printf("%s://%s/", scheme, self->m_host);
}

static inline gchar*
create_replacement(RpRequestRewrite* self, lztq* rewrite_urls)
{
    NOISY_MSG_("(%p, %p)", self, rewrite_urls);

    if (rewrite_urls)
    {
        NOISY_MSG_("rewrite_urls %p, %zu urls", rewrite_urls, lztq_size(rewrite_urls));

        for (lztq_elem* elem = lztq_first(rewrite_urls); elem; elem = lztq_next(elem))
        {
            gchar* rewrite_url = lztq_elem_data(elem);
            LOGD("rewrite_url %p(%s)", rewrite_url, rewrite_url);

            g_autofree gchar* scheme = NULL;
            g_autofree gchar* host = NULL;
            gint port;
            g_uri_split_network(rewrite_url, 0, &scheme, &host, &port, NULL);
            if (g_ascii_strcasecmp(host, self->m_host) == 0)
            {
                NOISY_MSG_("found \"%s:%d\"", host, port);
                return port != -1 ?
                    g_strdup_printf("%s://%s:%d/", scheme, host, port) :
                    g_strdup_printf("%s://%s/", scheme, host);
            }
        }
    }

    RpNetworkAddressInstanceConstSharedPtr addr = rp_host_description_address(self->m_host_description);
//    char buf[256];
//    const char* host_str = util_format_sockaddr_port(addr, buf, sizeof(buf));
const char* host_str = rp_network_address_instance_as_string(addr);
    const char* scheme = self->m_ssl_connection ? "https" : "http";
    return g_strdup_printf("%s://%s/", scheme, host_str);
}

static inline bool
create_regex(RpRequestRewrite* self)
{
    NOISY_MSG_("(%p)", self);

    g_autofree gchar* pattern = create_pattern(self);
    g_autofree gchar* replacement = create_replacement(self, self->m_rewrite_urls);
    // No point in going through a bunch of rigmarole if the pattern and
    // replacement are the same.
    if (g_ascii_strcasecmp(pattern, replacement) == 0)
    {
        NOISY_MSG_("nothing to replace");
        return false;
    }
    self->m_pattern = g_steal_pointer(&pattern);
    self->m_replacement = g_steal_pointer(&replacement);
    self->m_regex = g_regex_new(self->m_pattern, G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);
    return true;
}

static inline gchar*
get_host_value(gchar* replacement)
{
    NOISY_MSG_("(%p(%s))", replacement, replacement);
    g_autofree gchar* host = NULL;
    gint port;
    g_uri_split_network(replacement, 0, NULL, &host, &port, NULL);
    return port != -1 ? g_strdup_printf("%s:%d", host, port) : g_strdup(host);
}

static inline gchar*
get_origin_value(gchar* replacement)
{
    NOISY_MSG_("(%p(%s))", replacement, replacement);
    g_autofree gchar* scheme = NULL;
    g_autofree gchar* host = NULL;
    gint port;
    g_uri_split_network(replacement, 0, &scheme, &host, &port, NULL);
    return port != -1 ? g_strdup_printf("%s://%s:%d", scheme, host, port) :
                        g_strdup_printf("%s://%s", scheme, host);
}

static inline int
rewrite_header_cb_(evhtp_header_t* header, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", header, arg);

    RewriteHeaderCtx* ctx = arg;
    if (!ctx->m_saw_host &&
        header->klen == ctx->m_host_len &&
        g_ascii_strcasecmp(header->key, RpHeaderValues.HostLegacy) == 0)
    {
        NOISY_MSG_("header val %p(%s)", header->val, header->val);
        evhtp_header_val_set(header, get_host_value(ctx->m_replacement), 1);
        ctx->m_saw_host = true;
    }
    else if (!ctx->m_saw_origin &&
        header->klen == ctx->m_origin_len &&
        g_ascii_strcasecmp(header->key, RpCustomHeaderValues.Origin) == 0)
    {
        NOISY_MSG_("header val %p(%s)", header->val, header->val);
        evhtp_header_val_set(header, get_origin_value(ctx->m_replacement), 1);
        ctx->m_saw_origin = true;
    }
    else
    {
        GMatchInfo* match_info;
        g_regex_match_full(ctx->m_regex, header->val, header->vlen, 0, 0, &match_info, NULL);
        if (g_match_info_matches(match_info))
        {
            NOISY_MSG_("%d matches", g_match_info_get_match_count(match_info));
            gchar* res = g_regex_replace(ctx->m_regex,
                                            header->val,
                                            header->vlen,
                                            0,
                                            ctx->m_replacement,
                                            G_REGEX_MATCH_DEFAULT,
                                            NULL);
            NOISY_MSG_("\"%.*s\" -> \"%s\"", (int)header->vlen, header->val, res);
            evhtp_header_val_set(header, res, 1);
        }
        g_match_info_free(match_info);
    }
    return 0;
}

RpRequestRewrite
rp_request_rewrite_ctor(const gchar* original_uri, lztq* rewrite_urls, RpHostDescription* host_description, bool ssl_connection)
{
    LOGD("(%p(%s), %p, %p, %u)",
        original_uri, original_uri, rewrite_urls, host_description, ssl_connection);

    RpRequestRewrite self = {
        .m_rewrite_urls = rewrite_urls,
        .m_host_description = host_description,
        .m_original_uri = original_uri,
        .m_ssl_connection = ssl_connection
    };
    self.m_active = create_regex(&self);
    return self;
}

void
rp_request_rewrite_dtor(RpRequestRewrite* self)
{
    LOGD("(%p)", self);
    g_clear_pointer(&self->m_host, g_free);
    g_clear_pointer(&self->m_pattern, g_free);
    g_clear_pointer(&self->m_replacement, g_free);
    g_clear_pointer(&self->m_regex, g_regex_unref);
}

gchar*
rp_request_rewrite_host_value(RpRequestRewrite* self)
{
    LOGD("(%p)", self);
    return get_host_value(self->m_replacement);
}

gchar*
rp_request_rewrite_original_uri(RpRequestRewrite* self)
{
    LOGD("(%p)", self);
    g_autoptr(GUri) uri = g_uri_parse(self->m_original_uri, G_URI_FLAGS_SCHEME_NORMALIZE, NULL);
    g_autofree gchar* host_value = rp_request_rewrite_host_value(self);
    RpAuthorityAttributes attr = http_utility_parse_authority(host_value);
    g_autofree gchar* host = g_strndup(attr.m_host.m_data, attr.m_host.m_length);
    gint port = attr.m_port ? attr.m_port : -1;
    return g_uri_join(G_URI_FLAGS_NONE,
                        g_uri_get_scheme(uri),
                        g_uri_get_userinfo(uri),
                        host,
                        port,
                        g_uri_get_path(uri),
                        g_uri_get_query(uri),
                        g_uri_get_fragment(uri));
}

void
rp_request_rewrite_process_headers(RpRequestRewrite* self, evhtp_headers_t* request_headers)
{
    LOGD("(%p, %p)", self, request_headers);
    RewriteHeaderCtx arg = rewrite_header_ctx_ctor(self->m_regex, self->m_replacement);
    evhtp_headers_for_each(request_headers, rewrite_header_cb_, &arg);
}

void
rp_request_rewrite_process_data(RpRequestRewrite* self, evbuf_t* input_buffer)
{
    LOGD("(%p, %p(%zu))", self, input_buffer, evbuffer_get_length(input_buffer));

    gsize data_len = evbuffer_get_length(input_buffer);
    if (!data_len)
    {
        NOISY_MSG_("no data");
        return;
    }

    gchar* data = (gchar*)evbuffer_pullup(input_buffer, data_len);
    NOISY_MSG_("input_buffer before\n\"%.*s\"", (int)data_len, data);

    GMatchInfo* match_info;
    g_regex_match_full(self->m_regex, data, data_len, 0, 0, &match_info, NULL);
    if (g_match_info_matches(match_info))
    {
        NOISY_MSG_("%d matches", g_match_info_get_match_count(match_info));
        g_autofree gchar* res = g_regex_replace(self->m_regex,
                                                data,
                                                data_len,
                                                0,
                                                self->m_replacement,
                                                G_REGEX_MATCH_DEFAULT,
                                                NULL);
        evbuffer_drain(input_buffer, data_len);
        evbuffer_add(input_buffer, res, strlen(res));
    }
    g_match_info_free(match_info);

    NOISY_MSG_("input_buffer after\n\"%.*s\"",
        (int)evbuffer_get_length(input_buffer), (char*)evbuffer_pullup(input_buffer, -1));
}
