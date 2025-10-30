/*
 * rp-http-utilty.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_http_utility_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_http_utility_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <netdb.h>

#include "rp-header-utility.h"
#include "rp-headers.h"
#include "rp-http-utility.h"

evhtp_res
http_utility_get_response_status(evhtp_headers_t* headers)
{
    const char* status = evhtp_header_find(headers, RpHeaderValues.Status);
    if (!status)
    {
        LOGD("no status in headers");
        return 0;
    }
    return (evhtp_res)atoi(status);
}

const char*
http_utility_get_protocol_string(evhtp_proto protocol)
{
    switch (protocol)
    {
        case EVHTP_PROTO_10:
            return RpHeaderValues.ProtocolStrings.Http10String;
        case EVHTP_PROTO_11:
            return RpHeaderValues.ProtocolStrings.Http11String;
        default:
            return "Unsupported";
    }
}

char*
http_utility_build_original_uri(evhtp_headers_t* request_headers)
{
    LOGD("(%p)", request_headers);
    const char* path = evhtp_header_find(request_headers, RpHeaderValues.Path);
NOISY_MSG_("path \"%s\"", path);
    if (!path)
    {
        NOISY_MSG_("no path");
        return g_strdup("");
    }
const char* host_legacy = evhtp_header_find(request_headers, RpHeaderValues.HostLegacy);
NOISY_MSG_("host legacy \"%s\"", host_legacy);
    return g_strdup_printf("%s://%s%s", evhtp_header_find(request_headers, RpHeaderValues.Scheme),
                                        host_legacy,
                                        path);
}

bool
http_utility_is_upgrade(evhtp_headers_t* headers)
{
    return (evhtp_header_find(headers, RpHeaderValues.Upgrade) &&
            rp_header_utility_is_upgrade(headers));
}

static inline struct sockaddr_in
parse_v4_address(const gchar* ip_address, guint16 port)
{
    struct sockaddr_in sa4;
    memset(&sa4, 0, sizeof(sa4));
    if (evutil_inet_pton(AF_INET, ip_address, &sa4.sin_addr) != 1)
    {
        sa4.sin_family = AF_UNSPEC;
        return sa4;
    }
    sa4.sin_family = AF_INET;
    sa4.sin_port = htons(port);
    return sa4;
}

static inline struct sockaddr_in6
parse_v6_address(const gchar* ip_address, guint16 port)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    struct addrinfo* res = NULL;
    hints.ai_flags = AI_NUMERICHOST|AI_NUMERICSERV;
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    if (evutil_getaddrinfo(ip_address, NULL, &hints, &res) != 0)
    {
        struct sockaddr_in6 sa6;
        sa6.sin6_family = AF_UNSPEC;
        return sa6;
    }
    struct sockaddr_in6 sa6 = *((struct sockaddr_in6*)res->ai_addr);
    evutil_freeaddrinfo(res);
    sa6.sin6_port = htons(port);
    return sa6;
}

static inline bool
parse_address(string_view* ip_address, guint16 port)
{
    NOISY_MSG_("(%p, %u)", ip_address, port);
    g_autofree gchar* ip_address_ = g_strndup(ip_address->m_data, ip_address->m_length);
    struct sockaddr_in sa4 = parse_v4_address(ip_address_, port);
    if (sa4.sin_family == AF_INET)
    {
        NOISY_MSG_("ipv4 address");
        return true;
    }
    struct sockaddr_in6 sa6 = parse_v6_address(ip_address_, port);
    if (sa6.sin6_family == AF_INET6)
    {
        NOISY_MSG_("ipv6 address");
        return true;
    }
    return false;
}

RpAuthorityAttributes
http_utility_parse_authority(const char* host)
{
    LOGD("(%p(%s))", host, host);
    char* colon_pos = strrchr(host, ':');
    struct string_view_s host_to_resolve = string_view_ctor(host, strlen(host));
    guint16 port = 0;
    if (colon_pos && host_to_resolve.m_data[host_to_resolve.m_length - 1] != ']')
    {
        host_to_resolve.m_length = colon_pos - host_to_resolve.m_data;
        port = g_ascii_strtoull(colon_pos + 1, NULL, 10);
    }
    bool is_ip_address = false;
    struct string_view_s potential_ip_address = host_to_resolve;
    if (potential_ip_address.m_length &&
        potential_ip_address.m_data[0] == '[' &&
        potential_ip_address.m_data[potential_ip_address.m_length - 1] == ']')
    {
        ++potential_ip_address.m_data;
        potential_ip_address.m_length -= 2;
    }
    if (parse_address(&potential_ip_address, port))
    {
        NOISY_MSG_("yep");
        is_ip_address = true;
        host_to_resolve = potential_ip_address;
    }
    return rp_authority_attributes_ctor(is_ip_address, host_to_resolve, port);
}
