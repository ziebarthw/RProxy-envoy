/*
 * rp-header-utility.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_header_utility_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_header_utility_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "utils/header_value_parser.h"
#include "rp-headers.h"
#include "rp-http-utility.h"
#include "rp-header-utility.h"

static inline char*
get_header_value(evhtp_headers_t* headers, const char* name)
{
    NOISY_MSG_("(%p, %p(%s))", headers, name, name);
    const char* value = evhtp_header_find(headers, name);
    if (!value)
    {
        NOISY_MSG_("not found");
        return NULL;
    }
    struct header_value_parser_s parser = header_value_parser_ctor();
    struct tokenizer_cursor_s cursor = tokenizer_cursor_ctor(0, strlen(value));
    struct header_element_s header_element = header_value_parser_parse_header_element(&parser, value, &cursor);
    char* rval =  g_strdup(header_element_get_name(&header_element));
    header_element_dtor(&header_element);
    return rval;
}

static inline char*
get_connection_header_value(evhtp_headers_t* headers)
{
    return get_header_value(headers, RpHeaderValues.Connection);
}

static inline char*
get_proxy_connection_header_value(evhtp_headers_t* headers)
{
    return get_header_value(headers, RpHeaderValues.ProxyConnection);
}

static inline char*
get_status_code(evhtp_headers_t* headers)
{
    return get_header_value(headers, RpHeaderValues.Status);
}

bool
rp_header_utility_should_close_connection(evhtp_proto protocol, evhtp_headers_t* headers)
{
    LOGD("(%d, %p)", protocol, headers);
    if (protocol == EVHTP_PROTO_10)
    {
        NOISY_MSG_("EVHTP_PROTO_10");
        g_autofree char* connection = get_connection_header_value(headers);
        if (connection &&
            g_ascii_strcasecmp(connection, RpHeaderValues.ConnectionValues.KeepAlive) != 0)
        {
            NOISY_MSG_("yep");
            return true;
        }
    }
    else if (protocol == EVHTP_PROTO_11)
    {
        NOISY_MSG_("EVHTP_PROTO_11");
        g_autofree char* connection = get_connection_header_value(headers);
        if (connection &&
            g_ascii_strcasecmp(connection, RpHeaderValues.ConnectionValues.Close) == 0)
        {
            NOISY_MSG_("yep");
            return true;
        }
    }

    if (protocol <= EVHTP_PROTO_11)
    {
        g_autofree char* connection = get_proxy_connection_header_value(headers);
        if (connection &&
            g_ascii_strcasecmp(connection, RpHeaderValues.ConnectionValues.Close) == 0)
        {
            NOISY_MSG_("yep");
            return true;
        }
    }

    NOISY_MSG_("nope");
    return false;
}

static inline bool
is_get_method(const char* method, gsize len)
{
    NOISY_MSG_("(%p(%s), %zu)", method, method, len);
    if (len != 3) return false;
    return g_ascii_tolower(method[0]) == 'g' &&
            g_ascii_tolower(method[1]) == 'e' &&
            g_ascii_tolower(method[2]) == 't';
}

static inline bool
is_head_method(const char* method, gsize len)
{
    NOISY_MSG_("(%p(%s), %zu)", method, method, len);
    if (len != 4) return false;
    return g_ascii_tolower(method[0]) == 'h' &&
            g_ascii_tolower(method[1]) == 'e' &&
            g_ascii_tolower(method[2]) == 'a' &&
            g_ascii_tolower(method[3]) == 'd';
}

static inline bool
is_connect_method(const char* method, gsize len)
{
    NOISY_MSG_("(%p(%s), %zu)", method, method, len);
    if (len != 7) return false;
    return g_ascii_tolower(method[0]) == 'c' &&
            g_ascii_tolower(method[1]) == 'o' &&
            g_ascii_tolower(method[2]) == 'n' &&
            g_ascii_tolower(method[3]) == 'n' &&
            g_ascii_tolower(method[4]) == 'e' &&
            g_ascii_tolower(method[5]) == 'c' &&
            g_ascii_tolower(method[6]) == 't';
}

static inline bool
is_delete_method(const char* method, gsize len)
{
    NOISY_MSG_("(%p(%s), %zu)", method, method, len);
    if (len != 6) return false;
    return g_ascii_tolower(method[0]) == 'd' &&
            g_ascii_tolower(method[1]) == 'e' &&
            g_ascii_tolower(method[2]) == 'l' &&
            g_ascii_tolower(method[3]) == 'e' &&
            g_ascii_tolower(method[4]) == 't' &&
            g_ascii_tolower(method[5]) == 'e';
}

static inline bool
is_trace_method(const char* method, gsize len)
{
    NOISY_MSG_("(%p(%s), %zu)", method, method, len);
    if (len != 5) return false;
    return g_ascii_tolower(method[0]) == 't' &&
            g_ascii_tolower(method[1]) == 'r' &&
            g_ascii_tolower(method[2]) == 'a' &&
            g_ascii_tolower(method[3]) == 'c' &&
            g_ascii_tolower(method[4]) == 'e';
}

static inline const char*
method_value(evhtp_headers_t* headers)
{
    NOISY_MSG_("(%p)", headers);
    const char* method = evhtp_header_find(headers, RpHeaderValues.Method);
    if (method)
    {
        gsize len = strlen(method);
        if (is_get_method(method, len))
        {
            method = RpHeaderValues.MethodValues.Get;
        }
        else if (is_head_method(method, len))
        {
            method = RpHeaderValues.MethodValues.Head;
        }
        else if (is_connect_method(method, len))
        {
            method = RpHeaderValues.MethodValues.Connect;
        }
        else if (is_delete_method(method, len))
        {
            method = RpHeaderValues.MethodValues.Delete;
        }
        else if (is_trace_method(method, len))
        {
            method = RpHeaderValues.MethodValues.Trace;
        }
    }
    return method;
}

bool
rp_header_utility_request_should_have_no_body(evhtp_headers_t* headers)
{
    LOGD("(%p)", headers);
    const char* method = method_value(headers);
    return (method &&
            (method == RpHeaderValues.MethodValues.Get ||
            method == RpHeaderValues.MethodValues.Head ||
            method == RpHeaderValues.MethodValues.Delete ||
            method == RpHeaderValues.MethodValues.Trace ||
            method == RpHeaderValues.MethodValues.Connect));
}

bool
rp_header_utility_is_connect(evhtp_headers_t* request_headers)
{
    const char* method_value = evhtp_header_find(request_headers, RpHeaderValues.Method);
    return method_value &&
        g_ascii_strcasecmp(method_value, RpHeaderValues.MethodValues.Connect) == 0;
}

bool
rp_header_utility_is_special_1xx(evhtp_headers_t* response_headers)
{
    g_autofree char* status_code = get_status_code(response_headers);
    //TODO...proxy_104...
    return status_code && (g_ascii_strcasecmp(status_code, "100") == 0 ||
        g_ascii_strcasecmp(status_code, "102") == 0 ||
        g_ascii_strcasecmp(status_code, "103") == 0);
}

static inline bool
has_header_element(const char* value, const char* name)
{
    struct header_value_parser_s parser = header_value_parser_ctor();
    return header_value_parser_has_header_element(&parser, value, name);
}

bool
rp_header_utility_is_upgrade(evhtp_headers_t* headers)
{
    const char* value = evhtp_header_find(headers, RpHeaderValues.Connection);
    return value && has_header_element(value, RpHeaderValues.ConnectionValues.Upgrade);
}

gchar*
rp_header_utility_get_port_start(const char* host)
{
    gchar* port_start = g_strrstr(host, ":");
    if (!port_start)
    {
        return NULL;
    }
    gchar* v6_end = g_strrstr(host, "]");
    if (!v6_end || v6_end < port_start)
    {
        if ((port_start + 1) > (host + strlen(host)))
        {
            return NULL;
        }
        return port_start;
    }
    return NULL;
}

static inline int
simple_atoi(const char* str)
{
    int result = 0;
    int sign = 1;

    while (g_ascii_isspace(str[0])) ++str;

    if (str[0] == '-')
    {
        sign = -1;
        ++str;
    }
    else if (str[0] == '+')
    {
        ++str;
    }

    while (str[0] >= '0' && str[0] <= '9')
    {
        result = result * 10 + (str[0] - '0');
        ++str;
    }

    return !str[0] ? sign * result : 0;
}

bool
rp_header_utility_host_has_port(const char* original_host)
{
    gchar* port_start = rp_header_utility_get_port_start(original_host);
    if (!port_start)
    {
        return false;
    }
    g_autofree gchar* port_str = g_strndup(port_start + 1, strlen(port_start + 1));
    guint32 port = simple_atoi(port_str);
    if (!port)
    {
        return false;
    }
    return true;
}

bool
rp_header_utility_is_connect_response(evhtp_headers_t* request_headers, evhtp_headers_t* response_headers)
{
    return request_headers &&
            rp_header_utility_is_connect(request_headers) &&
            http_utility_get_response_status(response_headers) == EVHTP_RES_OK;
}
