/*
 * rp-header-utility.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <evhtp.h>

G_BEGIN_DECLS

bool rp_header_utility_should_close_connection(evhtp_proto protocol, evhtp_headers_t* headers);
bool rp_header_utility_request_should_have_no_body(evhtp_headers_t* headers);
bool rp_header_utility_is_connect(evhtp_headers_t* request_headers);
bool rp_header_utility_is_special_1xx(evhtp_headers_t* response_headers);
bool rp_header_utility_is_upgrade(evhtp_headers_t* headers);
bool rp_header_utility_host_has_port(const char* original_host);
bool rp_header_utility_is_connect_response(evhtp_headers_t* request_headers,
                                            evhtp_headers_t* response_headers);

G_END_DECLS
