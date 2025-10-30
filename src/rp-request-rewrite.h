/*
 * rp-request-rewrite.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"
#include "rp-host-description.h"

G_BEGIN_DECLS

typedef struct _RpRequestRewrite RpRequestRewrite;
struct _RpRequestRewrite {
    GRegex* m_regex;

    lztq* m_rewrite_urls;

    RpHostDescription* m_host_description;

    gchar* m_host;
    gchar* m_pattern;
    gchar* m_replacement;

    const gchar* m_original_uri;

    bool m_ssl_connection : 1;
    bool m_active : 1;
};

RpRequestRewrite rp_request_rewrite_ctor(const gchar* original_uri,
                                            lztq* rewrite_urls,
                                            RpHostDescription* host_description,
                                            bool ssl_connection);
void rp_request_rewrite_dtor(RpRequestRewrite* self);
gchar* rp_request_rewrite_host_value(RpRequestRewrite* self);
gchar* rp_request_rewrite_original_uri(RpRequestRewrite* self);
void rp_request_rewrite_process_headers(RpRequestRewrite* self,
                                        evhtp_headers_t* request_headers);
void rp_request_rewrite_process_data(RpRequestRewrite* self,
                                        evbuf_t* input_buffer);
static inline bool
rp_request_rewrite_active(RpRequestRewrite* self)
{
    return self && self->m_active;
}

G_END_DECLS
