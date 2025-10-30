/*
 * rp-conn-manager-config.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-codec.h"
#include "rp-downstream-filter-manager.h"//TODO...move RpLocalReply elsewhere...
#include "rp-net-connection.h"
#include "rp-filter-factory.h"
#include "rp-rds.h"
#include "lzq.h"

G_BEGIN_DECLS

#define DEFAULT_MAX_REQUEST_HEADERS_KB 60 //TODO...move elsewhere.
#define DEFAULT_MAX_HEADERS_COUNT 100 //TODO...move elsewhere.

/**
 * Type that indicates how port should be stripped from Host header.
 */
typedef enum {
    // Removes the port from host/authority header only if the port matches with the listener port.
    RpStripPortType_MatchingHost,
    // Removes any port from host/authority header.
    RpStripPortType_Any,
    // Keeps the port in host/authority header as is.
    RpStripPortType_None
} RpStripPortType_e;

/**
 * Abstract configuration for the connection manager.
 */
#define RP_TYPE_CONNECTION_MANAGER_CONFIG rp_connection_manager_config_get_type()
G_DECLARE_INTERFACE(RpConnectionManagerConfig, rp_connection_manager_config, RP, CONNECTION_MANAGER_CONFIG, GObject)

struct _RpConnectionManagerConfigInterface {
    GTypeInterface parent_iface;

    //TODO...accessLogs()
    RpHttpServerConnection* (*create_codec)(RpConnectionManagerConfig*,
                                        RpNetworkConnection*,
                                        evbuf_t*,
                                        RpHttpServerConnectionCallbacks*);
    RpFilterChainFactory* (*filter_factory)(RpConnectionManagerConfig*);
    guint32 (*max_request_headers_kb)(RpConnectionManagerConfig*);
    guint32 (*max_request_headers_count)(RpConnectionManagerConfig*);
    guint64 (*stream_idle_timeout)(RpConnectionManagerConfig*);
    bool (*is_routable)(RpConnectionManagerConfig*);
    guint64 (*request_timeout)(RpConnectionManagerConfig*);
    guint64 (*request_headers_timeout)(RpConnectionManagerConfig*);
    guint64 (*delayed_close_timeout)(RpConnectionManagerConfig*);
    guint64 (*max_stream_duration)(RpConnectionManagerConfig*);
    RpRouteConfigProvider* (*route_config_provider)(RpConnectionManagerConfig*);
    const char* (*server_name)(RpConnectionManagerConfig*);
    const char* (*scheme_to_set)(RpConnectionManagerConfig*);
    bool (*should_scheme_match_upstream)(RpConnectionManagerConfig*);
    bool (*use_remote_address)(RpConnectionManagerConfig*);
    //TODO...interAddressConfig()
    guint32 (*xff_num_trusted_hops)(RpConnectionManagerConfig*);
    bool (*skip_xff_append)(RpConnectionManagerConfig*);
    const char* (*via)(RpConnectionManagerConfig*);
    //TODO...forwardClientCert()
    //TODO...const Network::Address::Instance& localAddress();
    const char* (*user_agent)(RpConnectionManagerConfig*);
    bool (*proxy_100_continue)(RpConnectionManagerConfig*);
    bool (*stream_error_on_invalid_http_messaging)(RpConnectionManagerConfig*);
    const struct RpHttp1Settings_s* (*http1_settings)(RpConnectionManagerConfig*);
    bool (*should_normalize_path)(RpConnectionManagerConfig*);
    bool (*should_merge_slashes)(RpConnectionManagerConfig*);
    RpStripPortType_e (*strip_port_type)(RpConnectionManagerConfig*);
    RpLocalReply* (*local_reply)(RpConnectionManagerConfig*);
    bool (*should_strip_trailing_host_dot)(RpConnectionManagerConfig*);
    guint64 (*max_requests_per_connection)(RpConnectionManagerConfig*);
    //TODO...proxyStatusConfig();
    bool (*append_x_forwarded_port)(RpConnectionManagerConfig*);
    bool (*append_local_overload)(RpConnectionManagerConfig*);
    //TODO...addProxyProtocolConnectionState()

    lztq* (*rules)(RpConnectionManagerConfig*);
};

static inline RpHttpServerConnection*
rp_connection_manager_config_create_codec(RpConnectionManagerConfig* self,
                                            RpNetworkConnection* connection,
                                            evbuf_t* data,
                                            RpHttpServerConnectionCallbacks* callbacks)
{
    return RP_IS_CONNECTION_MANAGER_CONFIG(self) ?
        RP_CONNECTION_MANAGER_CONFIG_GET_IFACE(self)->create_codec(self, connection, data, callbacks) :
        NULL;
}
static inline RpFilterChainFactory*
rp_connection_manager_config_filter_factory(RpConnectionManagerConfig* self)
{
    return RP_IS_CONNECTION_MANAGER_CONFIG(self) ?
        RP_CONNECTION_MANAGER_CONFIG_GET_IFACE(self)->filter_factory(self) :
        NULL;
}
static inline guint32
rp_connection_manager_config_max_request_headers_kb(RpConnectionManagerConfig* self)
{
    return RP_IS_CONNECTION_MANAGER_CONFIG(self) ?
        RP_CONNECTION_MANAGER_CONFIG_GET_IFACE(self)->max_request_headers_kb(self) :
        60;
}
static inline guint32
rp_connection_manager_config_max_request_headers_count(RpConnectionManagerConfig* self)
{
    return RP_IS_CONNECTION_MANAGER_CONFIG(self) ?
        RP_CONNECTION_MANAGER_CONFIG_GET_IFACE(self)->max_request_headers_count(self) :
        100;
}
static inline bool
rp_connection_manager_config_is_routable(RpConnectionManagerConfig* self)
{
    return RP_IS_CONNECTION_MANAGER_CONFIG(self) ?
        RP_CONNECTION_MANAGER_CONFIG_GET_IFACE(self)->is_routable(self) : false;
}
static inline guint64
rp_connection_manager_config_max_stream_duration(RpConnectionManagerConfig* self)
{
    return RP_IS_CONNECTION_MANAGER_CONFIG(self) ?
        RP_CONNECTION_MANAGER_CONFIG_GET_IFACE(self)->max_stream_duration(self) :
        0;
}
static inline RpRouteConfigProvider*
rp_connection_manager_config_route_config_provider(RpConnectionManagerConfig* self)
{
    return RP_IS_CONNECTION_MANAGER_CONFIG(self) ?
        RP_CONNECTION_MANAGER_CONFIG_GET_IFACE(self)->route_config_provider(self) :
        NULL;
}
static inline bool
rp_connection_manager_config_proxy_100_continue(RpConnectionManagerConfig* self)
{
    return RP_IS_CONNECTION_MANAGER_CONFIG(self) ?
        RP_CONNECTION_MANAGER_CONFIG_GET_IFACE(self)->proxy_100_continue(self) :
        false;
}
static inline RpLocalReply*
rp_connection_manager_config_local_reply(RpConnectionManagerConfig* self)
{
    return RP_IS_CONNECTION_MANAGER_CONFIG(self) ?
        RP_CONNECTION_MANAGER_CONFIG_GET_IFACE(self)->local_reply(self) :
        NULL;
}
static inline guint64
rp_connection_manager_config_max_requests_per_connection(RpConnectionManagerConfig* self)
{
    return RP_IS_CONNECTION_MANAGER_CONFIG(self) ?
        RP_CONNECTION_MANAGER_CONFIG_GET_IFACE(self)->max_requests_per_connection(self) :
        0;
}
static inline lztq*
rp_connection_manager_config_rules(RpConnectionManagerConfig* self)
{
    return RP_IS_CONNECTION_MANAGER_CONFIG(self) ?
        RP_CONNECTION_MANAGER_CONFIG_GET_IFACE(self)->rules(self) : NULL;
}


G_END_DECLS
