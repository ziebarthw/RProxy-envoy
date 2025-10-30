/*
 * rp-parser.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#include "rp-parser.h"

GType
RpMessageType_e_get_type(void)
{
    static gsize type_id = 0;

    if (g_once_init_enter(&type_id))
    {
        static const GEnumValue MessageType_values[] = {
            {RpMessageType_Request, "Request Message Type", "Request"},
            {RpMessageType_Response, "Response Message Type", "Response"},
            {0, NULL, NULL},
        };
        GType RpMessageType_e_type = g_enum_register_static("RpMessageType", MessageType_values);
        g_once_init_leave(&type_id, RpMessageType_e_type);
    }
    return (GType)type_id;
}

G_DEFINE_INTERFACE(RpParser, rp_parser, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(RpParserCallbacks, rp_parser_callbacks, G_TYPE_OBJECT)

static size_t
execute_(RpParser* self, const char* slice, int len)
{
    return 0;
}

static void
resume_(RpParser* self)
{
}

static RpCallbackResult_e
pause_(RpParser* self)
{
    return RpCallbackResult_Success;
}

static RpParserStatus_e
get_status_(RpParser* self)
{
    return RpParserStatus_Ok;
}

static evhtp_res
status_code_(RpParser* self)
{
    return EVHTP_RES_BADREQ;
}

static bool
is_http_11_(RpParser* self)
{
    return true;
}

static uint64_t
content_length_(RpParser* self)
{
    return 0;
}

static bool
is_chunked_(RpParser* self)
{
    return false;
}

static const char*
method_name_(RpParser* self)
{
    return "";
}

static const char*
error_message_(RpParser* self)
{
    return "";
}

static int
has_transfer_encoding_(RpParser* self)
{
    return 0;
}

static void
rp_parser_default_init(RpParserInterface* iface)
{
    iface->execute = execute_;
    iface->resume = resume_;
    iface->pause = pause_;
    iface->get_status = get_status_;
    iface->status_code = status_code_;
    iface->is_http_11 = is_http_11_;
    iface->content_length = content_length_;
    iface->is_chunked = is_chunked_;
    iface->method_name = method_name_;
    iface->error_message = error_message_;
    iface->has_transfer_encoding = has_transfer_encoding_;
}

static void
rp_parser_callbacks_default_init(RpParserCallbacksInterface* iface)
{
}