/*
 * rp-legacy-http-parser-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-parser.h"

G_BEGIN_DECLS

#define RP_TYPE_LEGACY_HTTP_PARSER_IMPL rp_legacy_http_parser_impl_get_type()
G_DECLARE_FINAL_TYPE(RpLegacyHttpParserImpl, rp_legacy_http_parser_impl, RP, LEGACY_HTTP_PARSER_IMPL, GObject)

RpLegacyHttpParserImpl* rp_legacy_http_parser_impl_new(RpMessageType_e type,
                                                        RpParserCallbacks* callbacks);

G_END_DECLS
