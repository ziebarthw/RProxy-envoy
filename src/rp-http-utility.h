/*
 * rp-http-utilty.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <evhtp.h>
#include "rp-headers.h"

G_BEGIN_DECLS

typedef struct string_view_s string_view;
struct string_view_s {
    const char* m_data;
    size_t m_length;
};

static inline struct string_view_s
string_view_ltrim(struct string_view_s self)
{
    for (size_t i=0; i < self.m_length; ++i)
    {
        if (!g_ascii_isspace(self.m_data[i]))
        {
            break;
        }
        ++self.m_data;
        --self.m_length;
    }
    return self;
}

static inline struct string_view_s
string_view_ctor(const char* data, size_t length)
{
    struct string_view_s self = {
        .m_data = data,
        .m_length = length
    };
    return self;
}

typedef struct _RpAuthorityAttributes RpAuthorityAttributes;
struct _RpAuthorityAttributes {
    bool m_is_ip_address;
    string_view m_host;
    guint16 m_port;
};

static inline RpAuthorityAttributes
rp_authority_attributes_ctor(bool is_ip_address, string_view host, guint16 port)
{
    RpAuthorityAttributes self = {
        .m_is_ip_address = is_ip_address,
        .m_host = host,
        .m_port = port
    };
    return self;
}

evhtp_res http_utility_get_response_status(evhtp_headers_t* headers);
const char* http_utility_get_protocol_string(evhtp_proto protocol);
char* http_utility_build_original_uri(evhtp_headers_t* request_headers);
bool http_utility_is_upgrade(evhtp_headers_t* headers);
static inline bool
http_utility_scheme_is_http(const char* scheme)
{
    return scheme &&
            g_ascii_strcasecmp(scheme, RpHeaderValues.SchemeValues.Http) == 0;
}
static inline bool
http_utility_scheme_is_https(const char* scheme)
{
    return scheme &&
            g_ascii_strcasecmp(scheme, RpHeaderValues.SchemeValues.Https) == 0;
}
static inline bool
http_utility_scheme_is_valid(const char* scheme)
{
    return http_utility_scheme_is_http(scheme) || http_utility_scheme_is_https(scheme);
}
RpAuthorityAttributes http_utility_parse_authority(const char* host);

G_END_DECLS
