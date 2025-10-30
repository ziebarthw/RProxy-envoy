/*
 * rp-io-handle.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <evhtp.h>
#include "rp-file-event.h"

G_BEGIN_DECLS

typedef struct _RpDispatcher RpDispatcher;


/**
 * IoHandle: an abstract interface for all I/O operations
 */
#define RP_TYPE_IO_HANDLE rp_io_handle_get_type()
G_DECLARE_INTERFACE(RpIoHandle, rp_io_handle, RP, IO_HANDLE, GObject)

struct _RpIoHandleInterface {
    GTypeInterface parent_iface;

    void (*close)(RpIoHandle*);
    bool (*is_open)(RpIoHandle*);
    bool (*was_connected)(RpIoHandle*);
    int (*read)(RpIoHandle*, evbuf_t*);
    int (*write)(RpIoHandle*, evbuf_t*);
    int (*connect)(RpIoHandle*, struct sockaddr*, const char*);
    struct sockaddr* (*local_address)(RpIoHandle*);
    struct sockaddr* (*peer_address)(RpIoHandle*);
    void (*initialize_file_event)(RpIoHandle*,
                                    RpDispatcher*,
                                    RpFileReadyCb,
                                    gpointer,
                                    guint32);
    void (*activate_file_events)(RpIoHandle*, guint32);
    void (*enable_file_events)(RpIoHandle*, guint32);
    void (*reset_file_events)(RpIoHandle*);
    void (*shutdown)(RpIoHandle*, int);
    //TODO...
    const char* (*interface_name)(RpIoHandle*);
int (*sockfd)(RpIoHandle*);
};

static inline void
rp_io_handle_close(RpIoHandle* self)
{
    if (RP_IS_IO_HANDLE(self)) \
        RP_IO_HANDLE_GET_IFACE(self)->close(self);
}
static inline bool
rp_io_handle_is_open(RpIoHandle* self)
{
    return RP_IS_IO_HANDLE(self) ?
        RP_IO_HANDLE_GET_IFACE(self)->is_open(self) : false;
}
static inline bool
rp_io_handle_was_connected(RpIoHandle* self)
{
    return RP_IS_IO_HANDLE(self) ?
        RP_IO_HANDLE_GET_IFACE(self)->was_connected(self) : false;
}
static inline int
rp_io_handle_read(RpIoHandle* self, evbuf_t* buffer)
{
    return RP_IS_IO_HANDLE(self) ?
        RP_IO_HANDLE_GET_IFACE(self)->read(self, buffer) : -1;
}
static inline int
rp_io_handle_write(RpIoHandle* self, evbuf_t* buffer)
{
    return RP_IS_IO_HANDLE(self) ?
        RP_IO_HANDLE_GET_IFACE(self)->write(self, buffer) : -1;
}
static inline int
rp_io_handle_connect(RpIoHandle* self, struct sockaddr* address, const char* requested_server_name)
{
    return RP_IS_IO_HANDLE(self) ?
        RP_IO_HANDLE_GET_IFACE(self)->connect(self, address, requested_server_name) :
        -1;
}
static inline struct sockaddr*
rp_io_handle_local_address(RpIoHandle* self)
{
    return RP_IS_IO_HANDLE(self) ?
        RP_IO_HANDLE_GET_IFACE(self)->local_address(self) : NULL;
}
static inline struct sockaddr*
rp_io_handle_peer_address(RpIoHandle* self)
{
    return RP_IS_IO_HANDLE(self) ?
        RP_IO_HANDLE_GET_IFACE(self)->peer_address(self) : NULL;
}
static inline void
rp_io_handle_initialize_file_event(RpIoHandle* self, RpDispatcher* dispatcher, RpFileReadyCb cb, gpointer arg, guint32 events)
{
    if (RP_IS_IO_HANDLE(self)) \
        RP_IO_HANDLE_GET_IFACE(self)->initialize_file_event(self, dispatcher, cb, arg, events);
}
static inline void
rp_io_handle_activate_file_events(RpIoHandle* self, guint32 events)
{
    if (RP_IS_IO_HANDLE(self)) \
        RP_IO_HANDLE_GET_IFACE(self)->activate_file_events(self, events);
}
static inline void
rp_io_handle_enable_file_events(RpIoHandle* self, guint32 events)
{
    if (RP_IS_IO_HANDLE(self)) \
        RP_IO_HANDLE_GET_IFACE(self)->enable_file_events(self, events);
}
static inline void
rp_io_handle_reset_file_events(RpIoHandle* self)
{
    if (RP_IS_IO_HANDLE(self)) \
        RP_IO_HANDLE_GET_IFACE(self)->reset_file_events(self);
}
static inline void
rp_io_handle_shutdown(RpIoHandle* self, int how)
{
    if (RP_IS_IO_HANDLE(self)) \
        RP_IO_HANDLE_GET_IFACE(self)->shutdown(self, how);
}
static inline const char*
rp_io_handle_interface_name(RpIoHandle* self)
{
    return RP_IS_IO_HANDLE(self) ?
        RP_IO_HANDLE_GET_IFACE(self)->interface_name(self) : NULL;
}
static inline int
rp_io_handle_sockfd(RpIoHandle* self)
{
    return RP_IS_IO_HANDLE(self) ?
        RP_IO_HANDLE_GET_IFACE(self)->sockfd(self) : -1;
}

G_END_DECLS
