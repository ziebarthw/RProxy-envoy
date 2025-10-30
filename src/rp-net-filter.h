/*
 * rp-net-filter.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <evhtp.h>
#include "rp-net-connection.h"

G_BEGIN_DECLS

/**
 * Status codes returned by filters that can cause future filters to not get iterated to.
 */
typedef enum {
    RpNetworkFilterStatus_Continue,
    RpNetworkFilterStatus_StopIteration
} RpNetworkFilterStatus_e;


/**
 * Callbacks used by individual filter instances to communicate with the filter manager.
 */
#define RP_TYPE_NETWORK_FILTER_CALLBACKS rp_network_filter_callbacks_get_type()
G_DECLARE_INTERFACE(RpNetworkFilterCallbacks, rp_network_filter_callbacks, RP, NETWORK_FILTER_CALLBACKS, GObject)

struct _RpNetworkFilterCallbacksInterface {
    GTypeInterface parent_iface;

    RpNetworkConnection* (*connection)(RpNetworkFilterCallbacks*);
    RpSocket* (*socket)(RpNetworkFilterCallbacks*);
};

static inline RpNetworkConnection*
rp_network_filter_callbacks_connection(RpNetworkFilterCallbacks* self)
{
    return RP_IS_NETWORK_FILTER_CALLBACKS(self) ?
        RP_NETWORK_FILTER_CALLBACKS_GET_IFACE(self)->connection(self) : NULL;
}
static inline RpSocket*
rp_network_filter_callbacks_socket(RpNetworkFilterCallbacks* self)
{
    return RP_IS_NETWORK_FILTER_CALLBACKS(self) ?
        RP_NETWORK_FILTER_CALLBACKS_GET_IFACE(self)->socket(self) : NULL;
}


/**
 * Callbacks used by individual write filter instances to communicate with the filter manager.
 */
#define RP_TYPE_NETWORK_WRITE_FILTER_CALLBACKS rp_network_write_filter_callbacks_get_type()
G_DECLARE_INTERFACE(RpNetworkWriteFilterCallbacks, rp_network_write_filter_callbacks, RP, NETWORK_WRITE_FILTER_CALLBACKS, RpNetworkFilterCallbacks)

struct _RpNetworkWriteFilterCallbacksInterface {
    RpNetworkFilterCallbacksInterface parent_iface;

    void (*inject_write_data_to_filter_chain)(RpNetworkWriteFilterCallbacks*,
                                                evbuf_t*,
                                                bool);
};

static inline void
rp_network_write_filter_callbacks_inject_write_data_to_filter_chain(RpNetworkWriteFilterCallbacks* self,
                                                                    evbuf_t* data,
                                                                    bool end_stream)
{
    if (RP_IS_NETWORK_WRITE_FILTER_CALLBACKS(self))
    {
        RP_NETWORK_WRITE_FILTER_CALLBACKS_GET_IFACE(self)->inject_write_data_to_filter_chain(self, data, end_stream);
    }
}


/**
 * A write path binary connection filter.
 */
#define RP_TYPE_NETWORK_WRITE_FILTER rp_network_write_filter_get_type()
G_DECLARE_INTERFACE(RpNetworkWriteFilter, rp_network_write_filter, RP, NETWORK_WRITE_FILTER, GObject)

struct _RpNetworkWriteFilterInterface {
    GTypeInterface parent_iface;

    RpNetworkFilterStatus_e (*on_write)(RpNetworkWriteFilter*, evbuf_t*, bool);
    void (*initialize_write_filter_callbacks)(RpNetworkWriteFilter*,
                                                RpNetworkWriteFilterCallbacks*);
};

static inline RpNetworkFilterStatus_e
rp_network_write_filter_on_write(RpNetworkWriteFilter* self, evbuf_t* data, bool end_stream)
{
    return RP_IS_NETWORK_WRITE_FILTER(self) ?
        RP_NETWORK_WRITE_FILTER_GET_IFACE(self)->on_write(self, data, end_stream) :
        RpNetworkFilterStatus_Continue;
}
static inline void
rp_network_write_filter_initialize_write_filter_callbacks(RpNetworkWriteFilter* self,
                                                    RpNetworkWriteFilterCallbacks* callbacks)
{
    if (RP_IS_NETWORK_WRITE_FILTER(self))
    {
        RP_NETWORK_WRITE_FILTER_GET_IFACE(self)->initialize_write_filter_callbacks(self, callbacks);
    }
}


/**
 * Callbacks used by individual read filter instances to communicate with the filter manager.
 */
#define RP_TYPE_NETWORK_READ_FILTER_CALLBACKS rp_network_read_filter_callbacks_get_type()
G_DECLARE_INTERFACE(RpNetworkReadFilterCallbacks, rp_network_read_filter_callbacks, RP, NETWORK_READ_FILTER_CALLBACKS, RpNetworkFilterCallbacks)

struct _RpNetworkReadFilterCallbacksInterface {
    RpNetworkFilterCallbacksInterface parent_iface;

    void (*continue_reading)(RpNetworkReadFilterCallbacks*);
    void (*inject_read_data_to_filter_chain)(RpNetworkReadFilterCallbacks*,
                                                evbuf_t*,
                                                bool);
    //TODO...virtual Upstream::HostDescriptionConstSharedPtr upstreamHost();
    //TODO...virtual void upstreamHost(Upstream::HostDescriptionConstSharedPtr host);
    void (*start_upstream_secure_transport)(RpNetworkReadFilterCallbacks*);
};

static inline void
rp_network_read_filter_callbacks_continue_reading(RpNetworkReadFilterCallbacks* self)
{
    if (RP_IS_NETWORK_READ_FILTER_CALLBACKS(self))
    {
        RP_NETWORK_READ_FILTER_CALLBACKS_GET_IFACE(self)->continue_reading(self);
    }
}
static inline void
rp_network_read_filter_callbacks_inject_read_data_to_filter_chain(RpNetworkReadFilterCallbacks* self,
                                                            evbuf_t* data,
                                                            bool end_stream)
{
    if (RP_IS_NETWORK_READ_FILTER_CALLBACKS(self))
    {
        RP_NETWORK_READ_FILTER_CALLBACKS_GET_IFACE(self)->inject_read_data_to_filter_chain(self, data, end_stream);
    }
}
static inline void
rp_network_read_filter_callbacks_start_upstream_secure_transport(RpNetworkReadFilterCallbacks* self)
{
    if (RP_IS_NETWORK_READ_FILTER_CALLBACKS(self))
    {
        RP_NETWORK_READ_FILTER_CALLBACKS_GET_IFACE(self)->start_upstream_secure_transport(self);
    }
}


/**
 * A read path binary connection filter.
 */
#define RP_TYPE_NETWORK_READ_FILTER rp_network_read_filter_get_type()
G_DECLARE_INTERFACE(RpNetworkReadFilter, rp_network_read_filter, RP, NETWORK_READ_FILTER, GObject)

struct _RpNetworkReadFilterInterface {
    GTypeInterface parent_iface;

    RpNetworkFilterStatus_e (*on_data)(RpNetworkReadFilter*, evbuf_t*, bool);
    RpNetworkFilterStatus_e (*on_new_connection)(RpNetworkReadFilter*);
    void (*initialize_read_filter_callbacks)(RpNetworkReadFilter*,
                                                RpNetworkReadFilterCallbacks*);
    bool (*start_upstream_secure_transport)(RpNetworkReadFilter*);
};

static inline RpNetworkFilterStatus_e
rp_network_read_filter_on_data(RpNetworkReadFilter* self, evbuf_t* data, bool end_stream)
{
    return RP_IS_NETWORK_READ_FILTER(self) ?
        RP_NETWORK_READ_FILTER_GET_IFACE(self)->on_data(self, data, end_stream) :
        RpNetworkFilterStatus_Continue;
}
static inline RpNetworkFilterStatus_e
rp_network_read_filter_on_new_connection(RpNetworkReadFilter* self)
{
    return RP_IS_NETWORK_READ_FILTER(self) ?
        RP_NETWORK_READ_FILTER_GET_IFACE(self)->on_new_connection(self) :
        RpNetworkFilterStatus_Continue;
}
static inline void
rp_network_read_filter_initialize_read_filter_callbacks(RpNetworkReadFilter* self,
                                                RpNetworkReadFilterCallbacks* callbacks)
{
    if (RP_IS_NETWORK_READ_FILTER(self))
    {
        RP_NETWORK_READ_FILTER_GET_IFACE(self)->initialize_read_filter_callbacks(self, callbacks);
    }
}
static inline bool
rp_network_read_filter_start_upstream_secure_transport(RpNetworkReadFilter* self)
{
    return RP_IS_NETWORK_READ_FILTER(self) ?
        RP_NETWORK_READ_FILTER_GET_IFACE(self)->start_upstream_secure_transport(self) :
        false;
}


/**
 * A combination read and write filter. This allows a single filter instance to cover
 * both the read and write paths.
 */
//TODO...#define RP_TYPE_NETWORK_FILTER rp_network_filter_get_type()


/**
 * Interface for adding individual network filters to a manager.
 */
#define RP_TYPE_NETWORK_FILTER_MANAGER rp_network_filter_manager_get_type()
G_DECLARE_INTERFACE(RpNetworkFilterManager, rp_network_filter_manager, RP, NETWORK_FILTER_MANAGER, GObject)

struct _RpNetworkFilterManagerInterface {
    GTypeInterface parent_iface;

    void (*add_write_filter)(RpNetworkFilterManager*, RpNetworkWriteFilter*);
    //TODO...virtual void addFilter(FilterSharedPtr filter);
    void (*add_read_filter)(RpNetworkFilterManager*, RpNetworkReadFilter*);
    void (*remove_read_filter)(RpNetworkFilterManager*, RpNetworkReadFilter*);
    bool (*initialize_read_filters)(RpNetworkFilterManager*);
};

static inline void
rp_network_filter_manager_add_write_filter(RpNetworkFilterManager* self, RpNetworkWriteFilter* filter)
{
    if (RP_IS_NETWORK_FILTER_MANAGER(self))
    {
        RP_NETWORK_FILTER_MANAGER_GET_IFACE(self)->add_write_filter(self, filter);
    }
}
static inline void
rp_network_filter_manager_add_read_filter(RpNetworkFilterManager* self, RpNetworkReadFilter* filter)
{
    if (RP_IS_NETWORK_FILTER_MANAGER(self))
    {
        RP_NETWORK_FILTER_MANAGER_GET_IFACE(self)->add_read_filter(self, filter);
    }
}
static inline void
rp_network_filter_manager_remove_read_filter(RpNetworkFilterManager* self, RpNetworkReadFilter* filter)
{
    if (RP_IS_NETWORK_FILTER_MANAGER(self))
    {
        RP_NETWORK_FILTER_MANAGER_GET_IFACE(self)->remove_read_filter(self, filter);
    }
}
static inline bool
rp_network_filter_manager_initialize_read_filters(RpNetworkFilterManager* self)
{
    return RP_IS_NETWORK_FILTER_MANAGER(self) ?
        RP_NETWORK_FILTER_MANAGER_GET_IFACE(self)->initialize_read_filters(self) :
        false;
}

/**
 * This function is used to wrap the creation of a network filter chain for new connections as
 * they come in. Filter factories create the lambda at configuration initialization time, and then
 * they are used at runtime.
 * @param filter_manager supplies the filter manager for the connection to install filters
 * to. Typically the function will install a single filter, but it's technically possibly to
 * install more than one if desired.
 */
typedef void (*network_filter_factory_cb)(RpNetworkFilterManager*);

G_END_DECLS
