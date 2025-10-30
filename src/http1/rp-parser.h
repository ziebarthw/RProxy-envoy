/*
 * rp-parser.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <evhtp.h>

G_BEGIN_DECLS

typedef enum {
    RpMessageType_Request,
    RpMessageType_Response
} RpMessageType_e;

GType RpMessageType_e_get_type(void) G_GNUC_CONST;
#define RP_TYPE_MESSAGE_TYPE (RpMessageType_e_get_type())

typedef enum {
    RpCallbackResult_Error = -1,
    RpCallbackResult_Success = 0,
    RpCallbackResult_NoBody = 1,
    RpCallbackResult_NoBodyData = 2
} RpCallbackResult_e;

#define RP_TYPE_PARSER_CALLBACKS rp_parser_callbacks_get_type()
G_DECLARE_INTERFACE(RpParserCallbacks, rp_parser_callbacks, RP, PARSER_CALLBACKS, GObject)

struct _RpParserCallbacksInterface {
    GTypeInterface parent_iface;

    RpCallbackResult_e (*on_message_begin)(RpParserCallbacks*);
    RpCallbackResult_e (*on_url)(RpParserCallbacks*, const char*, size_t);
    RpCallbackResult_e (*on_status)(RpParserCallbacks*, const char*, size_t);
    RpCallbackResult_e (*on_header_field)(RpParserCallbacks*, const char*, size_t);
    RpCallbackResult_e (*on_header_value)(RpParserCallbacks*, const char*, size_t);
    RpCallbackResult_e (*on_headers_complete)(RpParserCallbacks*);
    void (*buffer_body)(RpParserCallbacks*, const char*, size_t);
    RpCallbackResult_e (*on_message_complete)(RpParserCallbacks*);
    void (*on_chunk_header)(RpParserCallbacks*, bool);
};

static inline RpCallbackResult_e
rp_parser_callbacks_on_message_begin(RpParserCallbacks* self)
{
    return RP_IS_PARSER_CALLBACKS(self) ?
        RP_PARSER_CALLBACKS_GET_IFACE(self)->on_message_begin(self) :
        RpCallbackResult_Error;
}
static inline RpCallbackResult_e
rp_parser_callbacks_on_url(RpParserCallbacks* self, const char* data, size_t length)
{
    return RP_IS_PARSER_CALLBACKS(self) ?
        RP_PARSER_CALLBACKS_GET_IFACE(self)->on_url(self, data, length) :
        RpCallbackResult_Error;
}
static inline RpCallbackResult_e
rp_parser_callbacks_on_status(RpParserCallbacks* self, const char* data, size_t length)
{
    return RP_IS_PARSER_CALLBACKS(self) ?
        RP_PARSER_CALLBACKS_GET_IFACE(self)->on_status(self, data, length) :
        RpCallbackResult_Error;
}
static inline RpCallbackResult_e
rp_parser_callbacks_on_header_field(RpParserCallbacks* self, const char* data, size_t length)
{
    return RP_IS_PARSER_CALLBACKS(self) ?
        RP_PARSER_CALLBACKS_GET_IFACE(self)->on_header_field(self, data, length) :
        RpCallbackResult_Error;
}
static inline RpCallbackResult_e
rp_parser_callbacks_on_header_value(RpParserCallbacks* self, const char* data, size_t length)
{
    return RP_IS_PARSER_CALLBACKS(self) ?
        RP_PARSER_CALLBACKS_GET_IFACE(self)->on_header_value(self, data, length) :
        RpCallbackResult_Error;
}
static inline RpCallbackResult_e
rp_parser_callbacks_on_headers_complete(RpParserCallbacks* self)
{
    return RP_IS_PARSER_CALLBACKS(self) ?
        RP_PARSER_CALLBACKS_GET_IFACE(self)->on_headers_complete(self) :
        RpCallbackResult_Error;
}
static inline void
rp_parser_callbacks_buffer_body(RpParserCallbacks* self, const char* data, size_t length)
{
    if (RP_IS_PARSER_CALLBACKS(self))
    {
        RP_PARSER_CALLBACKS_GET_IFACE(self)->buffer_body(self, data, length);
    }
}
static inline RpCallbackResult_e
rp_parser_callbacks_on_message_complete(RpParserCallbacks* self)
{
    return RP_IS_PARSER_CALLBACKS(self) ?
        RP_PARSER_CALLBACKS_GET_IFACE(self)->on_message_complete(self) :
        RpCallbackResult_Error;
}
static inline void
rp_parser_callbacks_on_chunk_header(RpParserCallbacks* self, bool is_final_chunk)
{
    if (RP_IS_PARSER_CALLBACKS(self))
    {
        RP_PARSER_CALLBACKS_GET_IFACE(self)->on_chunk_header(self, is_final_chunk);
    }
}

#define RP_TYPE_PARSER rp_parser_get_type()
G_DECLARE_INTERFACE(RpParser, rp_parser, RP, PARSER, GObject)

typedef enum {
    RpParserStatus_Error = -1,
    RpParserStatus_Ok = 0,
    RpParserStatus_Paused,
} RpParserStatus_e;

struct _RpParserInterface {
    GTypeInterface parent_iface;

    size_t (*execute)(RpParser*, const char*, int);
    void (*resume)(RpParser*);
    RpCallbackResult_e (*pause)(RpParser*);
    RpParserStatus_e (*get_status)(RpParser*);
    evhtp_res (*status_code)(RpParser*);
    bool (*is_http_11)(RpParser*);
    guint64 (*content_length)(RpParser*);
    bool (*is_chunked)(RpParser*);
    const char* (*method_name)(RpParser*);
    const char* (*error_message)(RpParser*);
    int (*has_transfer_encoding)(RpParser*);
    bool (*should_keep_alive)(RpParser*);
};

static inline size_t
rp_parser_execute(RpParser* self, const char* data, int length)
{
    return RP_IS_PARSER(self) ?
            RP_PARSER_GET_IFACE(self)->execute(self, data, length) : 0;
}
static inline void
rp_parser_resume(RpParser* self)
{
    if (RP_IS_PARSER(self))
    {
        RP_PARSER_GET_IFACE(self)->resume(self);
    }
}
static inline RpCallbackResult_e
rp_parser_pause(RpParser* self)
{
    return RP_IS_PARSER(self) ?
        RP_PARSER_GET_IFACE(self)->pause(self) : RpCallbackResult_Error;
}
static inline RpParserStatus_e
rp_parser_get_status(RpParser* self)
{
    return RP_IS_PARSER(self) ?
        RP_PARSER_GET_IFACE(self)->get_status(self) : RpParserStatus_Error;
}
static inline evhtp_res
rp_parser_status_code(RpParser* self)
{
    return RP_IS_PARSER(self) ?
        RP_PARSER_GET_IFACE(self)->status_code(self) : EVHTP_RES_NOTIMPL;
}
static inline bool
rp_parser_is_http_11(RpParser* self)
{
    return RP_IS_PARSER(self) ?
        RP_PARSER_GET_IFACE(self)->is_http_11(self) : false;
}
static inline guint64
rp_parser_content_length(RpParser* self)
{
    return RP_IS_PARSER(self) ?
        RP_PARSER_GET_IFACE(self)->content_length(self) : 0;
}
static inline bool
rp_parser_is_chunked(RpParser* self)
{
    return RP_IS_PARSER(self) ?
        RP_PARSER_GET_IFACE(self)->is_chunked(self) : false;
}
static inline const char*
rp_parser_method_name(RpParser* self)
{
    return RP_IS_PARSER(self) ?
        RP_PARSER_GET_IFACE(self)->method_name(self) : NULL;
}
static inline const char*
rp_parser_error_message(RpParser* self)
{
    return RP_IS_PARSER(self) ?
        RP_PARSER_GET_IFACE(self)->error_message(self) : "";
}
static inline int
rp_parser_has_transfer_encoding(RpParser* self)
{
    return RP_IS_PARSER(self) ?
        RP_PARSER_GET_IFACE(self)->has_transfer_encoding(self) : 0;
}
static inline bool
rp_parser_should_keep_alive(RpParser* self)
{
    return RP_IS_PARSER(self) ?
        RP_PARSER_GET_IFACE(self)->should_keep_alive(self) : false;
}

G_END_DECLS
