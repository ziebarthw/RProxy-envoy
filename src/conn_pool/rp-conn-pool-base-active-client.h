/*
 * rp-conn-pool-base-active-client.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-conn-pool-base.h"
#include "rp-net-connection.h"

G_BEGIN_DECLS

/*
 * ActiveClient provides a base class for connection pool clients that handles connection timings
 * as well as managing the connection timeout.
*/
#define RP_TYPE_CONNECTION_POOL_ACTIVE_CLIENT rp_connection_pool_active_client_get_type()
G_DECLARE_DERIVABLE_TYPE(RpConnectionPoolActiveClient, rp_connection_pool_active_client, RP, CONNECTION_POOL_ACTIVE_CLIENT, GObject)

struct _RpConnectionPoolActiveClientClass {
    GObjectClass parent_class;

    void (*release_resources)(RpConnectionPoolActiveClient*);
    guint32 (*effective_concurrent_stream_limit)(RpConnectionPoolActiveClient*);
    evhtp_proto (*protocol)(RpConnectionPoolActiveClient*);
    gint64 (*current_unused_capacity)(RpConnectionPoolActiveClient*);
    void (*initialize_read_filters)(RpConnectionPoolActiveClient*);
    void (*close)(RpConnectionPoolActiveClient*);
    guint64 (*id)(RpConnectionPoolActiveClient*);
    bool (*closing_with_incomplete_stream)(RpConnectionPoolActiveClient*);
    guint32 (*num_active_streams)(RpConnectionPoolActiveClient*);
    bool (*ready_for_stream)(RpConnectionPoolActiveClient*);
    bool (*had_negative_delta_on_stream_closed)(RpConnectionPoolActiveClient*);
    void (*drain)(RpConnectionPoolActiveClient*);
    bool (*has_handshake_completed)(RpConnectionPoolActiveClient*);
    bool (*supports_early_data)(RpConnectionPoolActiveClient*);

    void (*set_requested_server_name)(RpConnectionPoolActiveClient*,
                                        const char*);
    void (*connect)(RpConnectionPoolActiveClient*);
};

static inline void
rp_connection_pool_active_client_release_resources(RpConnectionPoolActiveClient* self)
{
    if (RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self))
    {
        RP_CONNECTION_POOL_ACTIVE_CLIENT_GET_CLASS(self)->release_resources(self);
    }
}
static inline guint32
rp_connection_pool_active_client_effective_concurrent_stream_limit(RpConnectionPoolActiveClient* self)
{
    return RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self) ?
        RP_CONNECTION_POOL_ACTIVE_CLIENT_GET_CLASS(self)->effective_concurrent_stream_limit(self) :
        0;
}
static inline evhtp_proto
rp_connection_pool_active_client_protocol(RpConnectionPoolActiveClient* self)
{
    return RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self) ?
        RP_CONNECTION_POOL_ACTIVE_CLIENT_GET_CLASS(self)->protocol(self) :
        EVHTP_PROTO_INVALID;
}
static inline gint64
rp_connection_pool_active_client_current_unused_capacity(RpConnectionPoolActiveClient* self)
{
    return RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self) ?
        RP_CONNECTION_POOL_ACTIVE_CLIENT_GET_CLASS(self)->current_unused_capacity(self) :
        0;
}
static inline void
rp_connection_pool_active_client_initialize_read_filters(RpConnectionPoolActiveClient* self)
{
    if (RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self))
    {
        RP_CONNECTION_POOL_ACTIVE_CLIENT_GET_CLASS(self)->initialize_read_filters(self);
    }
}
static inline void
rp_connection_pool_active_client_close(RpConnectionPoolActiveClient* self)
{
    if (RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self))
    {
        RP_CONNECTION_POOL_ACTIVE_CLIENT_GET_CLASS(self)->close(self);
    }
}
static inline guint64
rp_connection_pool_active_client_id(RpConnectionPoolActiveClient* self)
{
    return RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self) ?
        RP_CONNECTION_POOL_ACTIVE_CLIENT_GET_CLASS(self)->id(self) : 0;
}
static inline bool
rp_connection_pool_active_client_closing_with_incomplete_stream(RpConnectionPoolActiveClient* self)
{
    return RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self) ?
        RP_CONNECTION_POOL_ACTIVE_CLIENT_GET_CLASS(self)->closing_with_incomplete_stream(self) :
        0;
}
static inline guint32
rp_connection_pool_active_client_num_active_streams(RpConnectionPoolActiveClient* self)
{
    return RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self) ?
        RP_CONNECTION_POOL_ACTIVE_CLIENT_GET_CLASS(self)->num_active_streams(self) :
        0;
}
static inline bool
rp_connection_pool_active_client_ready_for_stream(RpConnectionPoolActiveClient* self)
{
    return RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self) ?
        RP_CONNECTION_POOL_ACTIVE_CLIENT_GET_CLASS(self)->ready_for_stream(self) :
        false;
}
static inline bool
rp_connection_pool_active_client_had_negative_delta_on_stream_closed(RpConnectionPoolActiveClient* self)
{
    return RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self) ?
        RP_CONNECTION_POOL_ACTIVE_CLIENT_GET_CLASS(self)->had_negative_delta_on_stream_closed(self) :
        false;
}
static inline void
rp_connection_pool_active_client_drain(RpConnectionPoolActiveClient* self)
{
    if (RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self)) \
        RP_CONNECTION_POOL_ACTIVE_CLIENT_GET_CLASS(self)->drain(self);
}
static inline bool
rp_connection_pool_active_client_has_handshake_completed(RpConnectionPoolActiveClient* self)
{
    return RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self) ?
        RP_CONNECTION_POOL_ACTIVE_CLIENT_GET_CLASS(self)->has_handshake_completed(self) :
        false;
}
static inline bool
rp_connection_pool_active_client_supports_early_data(RpConnectionPoolActiveClient* self)
{
    return RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self) ?
        RP_CONNECTION_POOL_ACTIVE_CLIENT_GET_CLASS(self)->supports_early_data(self) :
        false;
}
static inline void
rp_connection_pool_active_client_set_requested_server_name(RpConnectionPoolActiveClient* self, const char* requested_server_name)
{
    if (RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self)) \
        RP_CONNECTION_POOL_ACTIVE_CLIENT_GET_CLASS(self)->set_requested_server_name(self, requested_server_name);
}
static inline void
rp_connection_pool_active_client_connect(RpConnectionPoolActiveClient* self)
{
    if (RP_IS_CONNECTION_POOL_ACTIVE_CLIENT(self)) \
        RP_CONNECTION_POOL_ACTIVE_CLIENT_GET_CLASS(self)->connect(self);
}

void rp_connection_pool_active_client_release_resources_base(RpConnectionPoolActiveClient* self);
void rp_connection_pool_active_client_on_connect_timeout(RpConnectionPoolActiveClient* self);
void rp_connection_pool_active_client_on_connection_duration_timeout(RpConnectionPoolActiveClient* self);
RpConnectionPoolActiveClientState_e rp_connection_pool_active_client_state(RpConnectionPoolActiveClient* self);
void rp_connection_pool_active_client_set_state(RpConnectionPoolActiveClient* self,
                                                RpConnectionPoolActiveClientState_e state);
RpHostDescription* rp_connection_pool_active_client_get_real_host_description(RpConnectionPoolActiveClient* self);
void rp_connection_pool_active_client_set_real_host_description(RpConnectionPoolActiveClient* self,
                                                                RpHostDescription* host_desc);
guint32 rp_connection_pool_active_client_decr_remaining_streams(RpConnectionPoolActiveClient* self);
void rp_connection_pool_active_client_set_remaining_streams(RpConnectionPoolActiveClient* self,
                                                            guint32 count);
void rp_connection_pool_active_client_set_has_handshake_completed(RpConnectionPoolActiveClient* self,
                                                                    bool v);
RpConnPoolImplBase* rp_connection_pool_active_client_parent_(RpConnectionPoolActiveClient* self);

G_END_DECLS
