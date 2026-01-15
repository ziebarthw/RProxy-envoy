/*
 * rp-headers.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

//source/common/http/headers.h
struct RpCustomHeaderValues_s {

    const char* Accept;
    const char* AcceptEncoding;
    const char* AccessControlRequestHeaders;
    const char* AccessControlRequestMethod;
    const char* AccessControlAllowOrigin;
    const char* AccessControlAllowHeaders;
    const char* AccessControlAllowMethods;
    const char* AccessControlExposeHeaders;
    const char* AccessControlMaxAge;
    const char* AccessControlAllowCredentials;
    const char* AccessControlRequestPrivateNetwork;
    const char* AccessControlAllowPrivateNetwork;
    //TODO...
    const char* CacheControl;
    const char* CacheStatus;
    const char* CdnLoop;
    const char* ContentEncoding;
    //TODO...
    const char* Origin;
    const char* Referer;

    struct {
        const char* Gzip;
        const char* Identity;
        const char* Wildcard;
    } AcceptEncodingValues;

    //TODO...

    struct {
        const char* Brotli;
        const char* Deflate;
        const char* Gzip;
        const char* Zstd;
    } ContentEncodingValues;

    //TODO...

};

struct RpHeaderValues_s {

    const char* Connection;
    const char* ContentLength;
    const char* ContentType;
    const char* Cookie;
    const char* Date;
    const char* Expect;
    const char* ForwardedClientCert;
    const char* ForwardedFor;
    const char* ForwardedHost;
    const char* ForwardedPort;
    const char* ForwardedProto;
    const char* Host;
    const char* HostLegacy;
    const char* Http2Settings;
    const char* KeepAlive;
    const char* Location;
    const char* Method;
    const char* Path;
    const char* Protocol;
    const char* ProxyConnection;
    const char* ProxyStatus;
    const char* Range;
    const char* RequestId;
    const char* Scheme;
    const char* Server;
    const char* SetCookie;
    const char* Status;
    const char* TransferEncoding;
    const char* TE;
    const char* Upgrade;
    const char* UserAgent;
    const char* Via;
    const char* WWWAuthenticate;
    const char* XContentTypeOptions;
    const char* XSquashDebug;
    const char* EarlyData;

    struct {
        const char* Close;
        const char* Http2Settings;
        const char* KeepAlive;
        const char* Upgrade;
    } ConnectionValues;

    struct {
        const char* H2c;
        const char* WebSocket;
        const char* ConnectUdp;
    } UpgradeValues;

    struct {
        const char* Text;
        const char* TextUtf8;
        const char* Html;
        const char* Json;
        const char* FormUrlEncoded;
    } ContentTypeValues;

    struct {
        const char* _100Continue;
    } ExpectValues;

    struct {
        const char* Connect;
        const char* Delete;
        const char* Get;
        const char* Head;
        const char* Options;
        const char* Patch;
        const char* Post;
        const char* Put;
        const char* Trace;
    } MethodValues;

    struct {
        const char* Bytestream;
    } ProtocolValues;

    struct {
        const char* Http;
        const char* Https;
    } SchemeValues;

    struct {
        const char* Brotli;
        const char* Compress;
        const char* Chunked;
        const char* Deflate;
        const char* Gzip;
        const char* Identity;
        const char* Zstd;
    } TransferCodingValues;

    struct {
        const char* Trailers;
    } TEValues;

    struct {
        const char* Nosniff;
    } XContentTypeOptionValues;

    struct {
        const char* Http10String;
        const char* Http11String;
        const char* Http2String;
        const char* Http3String;
    } ProtocolStrings;

};

extern struct RpCustomHeaderValues_s RpCustomHeaderValues;
extern struct RpHeaderValues_s RpHeaderValues;

G_END_DECLS
