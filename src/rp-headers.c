/*
 * rp-headers.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#include "rp-headers.h"

struct RpCustomHeaderValues_s RpCustomHeaderValues = {

    .Accept = "accept",
    .AcceptEncoding = "accept-encoding",
    .AccessControlRequestHeaders = "access-control-request-headers",
    .AccessControlRequestMethod = "access-control-request-method",
    .AccessControlAllowOrigin = "access-control-allow-origin",
    .AccessControlAllowHeaders = "access-control-allow-headers",
    .AccessControlAllowMethods = "access-control-allow-methods",
    .AccessControlExposeHeaders = "access-control-expose-headers",
    .AccessControlMaxAge = "access-control-max-age",
    .AccessControlAllowCredentials = "access-control-allow-credentials",
    .AccessControlRequestPrivateNetwork = "access-control-request-private-network",
    .AccessControlAllowPrivateNetwork = "access-control-allow-private-network",
    .CacheControl = "cache-control",
    .CacheStatus = "cache-status",
    .CdnLoop = "cdn-loop",
    .ContentEncoding = "content-encoding",
    .Origin = "origin",
    .Referer = "referer",

    .AcceptEncodingValues = {
        .Gzip = "gzip",
        .Identity = "identity",
        .Wildcard = "*"
    },

    .ContentEncodingValues = {
        .Brotli = "br",
        .Deflate = "deflate",
        .Gzip = "gzip",
        .Zstd = "zstd"
    },

};

struct RpHeaderValues_s RpHeaderValues = {

    .Connection = "connection",
    .ContentLength = "content-length",
    .ContentType = "content-type",
    .Cookie = "cookie",
    .Date = "date",
    .Expect = "expect",
    .ForwardedClientCert = "x-forwarded-client-cert",
    .ForwardedFor = "x-forwarded-for",
    .ForwardedHost = "x-forwarded-host",
    .ForwardedPort = "x-forwarded-port",
    .ForwardedProto = "x-forwarded-proto",
    .Host = ":authority",
    .HostLegacy = "host",
    .Http2Settings = "http2-settings",
    .KeepAlive = "keep-alive",
    .Location = "location",
    .Method = ":method",
    .Path = ":path",
    .Protocol = ":protocol",
    .ProxyConnection = "proxy-connection",
    .ProxyStatus = "proxy-status",
    .Range = "range",
    .RequestId = "x-request-id",
    .Scheme = ":scheme",
    .Server = "server",
    .SetCookie = "set-cookie",
    .Status = ":status",
    .TransferEncoding = "transfer-encoding",
    .TE = "te",
    .Upgrade = "upgrade",
    .UserAgent = "user-agent",
    .Via = "via",
    .WWWAuthenticate = "www-authenticate",
    .XContentTypeOptions = "x-content-type-options",
    .XSquashDebug = "x-squash-debug",
    .EarlyData = "early-data",

    .ConnectionValues = {
        .Close = "close",
        .Http2Settings = "http2-settings",
        .KeepAlive = "keep-alive",
        .Upgrade = "upgrade"
    },

    .UpgradeValues = {
        .H2c = "h2c",
        .WebSocket = "websocket",
        .ConnectUdp = "connect-udp"
    },

    .ContentTypeValues = {
        .Text = "text/plain",
        .TextUtf8 = "text/plain; charset=UTF-8",
        .Html = "text/html; charset=UTF-8",
        .Json = "application/json",
        .FormUrlEncoded = "application/x-www-form-urlencoded"
    },

    .ExpectValues = {
        ._100Continue = "100-continue"
    },

    .MethodValues = {
        .Connect = "CONNECT",
        .Delete = "DELETE",
        .Get = "GET",
        .Head = "HEAD",
        .Options = "OPTIONS",
        .Patch = "PATCH",
        .Post = "POST",
        .Put = "PUT",
        .Trace = "TRACE"
    },

    .ProtocolValues = {
        .Bytestream = "bytestream"
    },

    .SchemeValues = {
        .Http = "http",
        .Https = "https"
    },

    .TransferCodingValues = {
        .Brotli = "br",
        .Compress = "compress",
        .Chunked = "chunked",
        .Deflate = "deflate",
        .Gzip = "gzip",
        .Identity = "identity",
        .Zstd = "zstd"
    },

    .TEValues = {
        .Trailers = "trailers"
    },

    .XContentTypeOptionValues = {
        .Nosniff = "nosniff"
    },

    .ProtocolStrings = {
        .Http10String = "HTTP/1.0",
        .Http11String = "HTTP/1.1",
        .Http2String = "HTTP/2",
        .Http3String = "HTTP/3"
    },

};
