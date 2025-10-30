/*
 * rp-route-configuration.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"

G_BEGIN_DECLS

typedef enum {
    // No TLS requirement for the virtual host.
    RpTlsRequirementType_None,

    // External requests must use TLS. If a request is external and it is not
    // using TLS, a 301 redirect will be sent telling the client to use HTTPS.
    RpTlsRequirementType_External_Only,

    // All requests must use TLS. If a request is not using TLS, a 301 redirect
    // will be sent telling the client to use HTTPS.
    RpTlsRequirementType_All
} RpTlsRequirementType_e;


// [#protodoc-title: HTTP route components]
// * Routing :ref:`architecture overview <arch_overview_http_routing>`
// * HTTP :ref:`router filter <config_http_filters_router>`


// A route is both a specification of how to match a request as well as an indication of what to do
// next (e.g., redirect, forward, rewrite, etc.).
//
// .. attention::
//
//   Envoy supports routing on HTTP method via :ref:`header matching
//   <envoy_v3_api_msg_config.route.v3.HeaderMatcher>`.
typedef struct _RpRouteCfg RpRouteCfg;
struct _RpRouteCfg {

    // Name for the route.
    char* name;

    char* path_specifier;

    // Indicates that prefix/path matching should be case sensitive. The default
    // is true. Ignored for safe_regex matching.
    bool case_sensitive;
};


// The top level element in the routing configuration is a virtual host. Each virtual host has
// a logical name as well as a set of domains that get routed to it based on the incoming request's
// host header. This allows a single listener to service multiple top level domain path trees. Once
// a virtual host is selected based on the domain, the routes are processed in order to see which
// upstream cluster to route to or whether to perform a redirect.
typedef struct _RpVirtualHostCfg RpVirtualHostCfg;
struct _RpVirtualHostCfg {

    RpTlsRequirementType_e tls_requirement_type;

    // The logical name of the virtual host. This is used when emitting certain
    // statistics but is not relevant for routing.
    char* name;

    // A list of domains (host/authority header) that will be matched to this
    // virtual host. Wildcard hosts are supported in the suffix or prefix form.
    //
    // Domain search order:
    //  1. Exact domain names: ``www.foo.com``.
    //  2. Suffix domain wildcards: ``*.foo.com`` or ``*-bar.foo.com``.
    //  3. Prefix domain wildcards: ``foo.*`` or ``foo-*``.
    //  4. Special wildcard ``*`` matching any domain.
    //
    // .. note::
    //
    //   The wildcard will not match the empty string.
    //   e.g. ``*-bar.foo.com`` will match ``baz-bar.foo.com`` but not ``-bar.foo.com``.
    //   The longest wildcards match first.
    //   Only a single virtual host in the entire route configuration can match on ``*``. A domain
    //   must be unique across all virtual hosts or the config will fail to load.
    //
    // Domains cannot contain control characters. This is validated by the well_known_regex HTTP_HEADER_VALUE.
    lztq* domains;


};


// [#protodoc-title: HTTP route configuration]
// * Routing :ref:`architecture overview <arch_overview_http_routing>`
// * HTTP :ref:`router filter <config_http_filters_router>`


typedef struct _RpRouteConfiguration RpRouteConfiguration;
struct _RpRouteConfiguration {

    // The name of the route configuration. For example, it might match
    // :ref:`route_config_name
    // <envoy_v3_api_field_extensions.filters.network.http_connection_manager.v3.Rds.route_config_name>` in
    // :ref:`envoy_v3_api_msg_extensions.filters.network.http_connection_manager.v3.Rds`.
    char* name;

    // An array of virtual hosts that make up the route table.
    lztq* virtual_hosts;

    // The maximum bytes of the response :ref:`direct response body
    // <envoy_v3_api_field_config.route.v3.DirectResponseAction.body>` size. If not specified the default
    // is 4096.
    //
    // .. warning::
    //
    //   Envoy currently holds the content of :ref:`direct response body
    //   <envoy_v3_api_field_config.route.v3.DirectResponseAction.body>` in memory. Be careful setting
    //   this to be larger than the default 4KB, since the allocated memory for direct response body
    //   is not subject to data plane buffering controls.
    //
    guint32 max_direct_response_body_size_bytes;

    // By default, port in :authority header (if any) is used in host matching.
    // With this option enabled, Envoy will ignore the port number in the :authority header (if any) when picking VirtualHost.
    // NOTE: this option will not strip the port number (if any) contained in route config
    // :ref:`envoy_v3_api_msg_config.route.v3.VirtualHost`.domains field.
    bool ignore_port_in_host_matching;

    // Ignore path-parameters in path-matching.
    // Before RFC3986, URI were like(RFC1808): <scheme>://<net_loc>/<path>;<params>?<query>#<fragment>
    // Envoy by default takes ":path" as "<path>;<params>".
    // For users who want to only match path on the "<path>" portion, this option should be true.
    bool ignore_path_paramaters_in_path_matching;

};

G_END_DECLS
