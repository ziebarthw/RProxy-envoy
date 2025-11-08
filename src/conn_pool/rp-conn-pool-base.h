/*
 * rp-conn-pool-base.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "common/rp-conn-pool.h"
#include "rp-net-connection.h"

G_BEGIN_DECLS

typedef struct _RpPendingStream * RpPendingStreamPtr;
typedef struct _RpConnectionPoolActiveClient * RpConnectionPoolActiveClientPtr;
typedef struct _RpConnectionPoolAttachContext * RpConnectionPoolAttachContextPtr;
typedef struct _RpHost * RpHostPtr;

typedef enum {
    RpConnectionPoolActiveClientState_Connecting,
    RpConnectionPoolActiveClientState_ReadyForEarlyData,
    RpConnectionPoolActiveClientState_Ready,
    RpConnectionPoolActiveClientState_Busy,
    RpConnectionPoolActiveClientState_Draining,
    RpConnectionPoolActiveClientState_Closed
} RpConnectionPoolActiveClientState_e;

typedef enum {
    RpConnectionResult_FailedToCreateConnection,
    RpConnectionResult_CreatedNewConnection,
    RpConnectionResult_ShouldNotConnect,
    RpConnectionResult_NoConnectionRateLimited,
    RpConnectionResult_CreatedButRateLimited,
} RpConnectionResult_e;

/*
 * A placeholder struct for whatever data a given connection pool needs to
 * successfully attach an upstream connection to a downstream connection.
 */
typedef struct _RpConnectionPoolAttachContext RpConnectionPoolAttachContext;
struct _RpConnectionPoolAttachContext {
    gpointer m_data;
};
static inline RpConnectionPoolAttachContext
rp_connection_pool_attach_context_ctor(gpointer data)
{
    RpConnectionPoolAttachContext self = {
        .m_data = data
    };
    return self;
}


/*
 * Base class that handles stream queueing logic shared between connection pool implementations.
 */
#define RP_TYPE_CONN_POOL_IMPL_BASE rp_conn_pool_impl_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpConnPoolImplBase, rp_conn_pool_impl_base, RP, CONN_POOL_IMPL_BASE, GObject)

struct _RpConnPoolImplBaseClass {
    GObjectClass parent_class;

    bool (*track_stream_capacity)(RpConnPoolImplBase*);
    RpConnectionPoolActiveClientPtr (*instantiate_active_client)(RpConnPoolImplBase*);
    RpCancellable* (*new_pending_stream)(RpConnPoolImplBase*, RpConnectionPoolAttachContextPtr, bool);
    void (*attach_stream_to_client)(RpConnPoolImplBase*,
                                    RpConnectionPoolActiveClientPtr,
                                    RpConnectionPoolAttachContextPtr);
    bool (*enforce_max_requests)(RpConnPoolImplBase*);
    void (*on_pool_failure)(RpConnPoolImplBase*,
                            RpHostDescription*,
                            const char*,
                            RpPoolFailureReason_e,
                            RpConnectionPoolAttachContextPtr);
    void (*on_pool_ready)(RpConnPoolImplBase*,
                            RpConnectionPoolActiveClientPtr,
                            RpConnectionPoolAttachContextPtr);
    void (*on_connected)(RpConnPoolImplBase*, RpConnectionPoolActiveClientPtr);
    void (*on_connect_failed)(RpConnPoolImplBase*, RpConnectionPoolActiveClientPtr);
};

static inline bool
rp_conn_pool_impl_base_track_stream_capacity(RpConnPoolImplBase* self)
{
    return RP_IS_CONN_POOL_IMPL_BASE(self) ?
        RP_CONN_POOL_IMPL_BASE_GET_CLASS(self)->track_stream_capacity(self) :
        false;
}
static inline RpCancellable*
rp_conn_pool_impl_base_new_pending_stream(RpConnPoolImplBase* self,
                                            RpConnectionPoolAttachContextPtr context,
                                            bool can_send_early_data)
{
    return RP_IS_CONN_POOL_IMPL_BASE(self) ?
        RP_CONN_POOL_IMPL_BASE_GET_CLASS(self)->new_pending_stream(self, context, can_send_early_data) :
        NULL;
}
static inline void
rp_conn_pool_impl_base_attach_stream_to_client(RpConnPoolImplBase* self,
                                    RpConnectionPoolActiveClientPtr client,
                                    RpConnectionPoolAttachContextPtr context)
{
    if (RP_IS_CONN_POOL_IMPL_BASE(self))
    {
        RP_CONN_POOL_IMPL_BASE_GET_CLASS(self)->attach_stream_to_client(self, client, context);
    }
}
static inline bool
rp_conn_pool_impl_base_enforce_max_requests(RpConnPoolImplBase* self)
{
    return RP_IS_CONN_POOL_IMPL_BASE(self) ?
        RP_CONN_POOL_IMPL_BASE_GET_CLASS(self)->enforce_max_requests(self) :
        false;
}
static inline void
rp_conn_pool_impl_base_on_pool_failure(RpConnPoolImplBase* self, RpHostDescription* host_description,
                                        const char* failure_reason,
                                        RpPoolFailureReason_e pool_failure_reason,
                                        RpConnectionPoolAttachContextPtr context)
{
    if (RP_IS_CONN_POOL_IMPL_BASE(self))
    {
        RP_CONN_POOL_IMPL_BASE_GET_CLASS(self)->on_pool_failure(self,
                                                                host_description,
                                                                failure_reason,
                                                                pool_failure_reason,
                                                                context);
    }
}
static inline
void rp_conn_pool_impl_base_on_pool_ready(RpConnPoolImplBase* self,
                                            RpConnectionPoolActiveClientPtr client,
                                            RpConnectionPoolAttachContextPtr context)
{
    if (RP_IS_CONN_POOL_IMPL_BASE(self))
    {
        RP_CONN_POOL_IMPL_BASE_GET_CLASS(self)->on_pool_ready(self, client, context);
    }
}

void rp_conn_pool_impl_base_transition_active_client_state(RpConnPoolImplBase* self,
                                                            RpConnectionPoolActiveClientPtr client,
                                                            RpConnectionPoolActiveClientState_e new_state);
void rp_conn_pool_impl_base_on_upstream_ready(RpConnPoolImplBase* self);
void rp_conn_pool_impl_base_on_upstream_ready_for_early_data(RpConnPoolImplBase* self,
                                                                RpConnectionPoolActiveClientPtr client);
void rp_conn_pool_impl_base_decr_cluster_stream_capacity(RpConnPoolImplBase* self,
                                                            guint32 delta);
void rp_conn_pool_impl_base_incr_cluster_stream_capacity(RpConnPoolImplBase* self,
                                                            guint32 delta);
void rp_conn_pool_impl_base_incr_connecting_and_connected_stream_capacity(RpConnPoolImplBase* self,
                                                                            guint32 delta,
                                                                            RpConnectionPoolActiveClientPtr client);
void rp_conn_pool_impl_base_decr_connecting_and_connected_stream_capacity(RpConnPoolImplBase* self,
                                                                            guint32 delta,
                                                                            RpConnectionPoolActiveClientPtr client);
RpHostPtr rp_conn_pool_impl_base_host(RpConnPoolImplBase* self);
RpResourcePriority_e rp_conn_pool_impl_base_priority(RpConnPoolImplBase* self);
void rp_conn_pool_impl_base_on_pending_stream_cancel(RpConnPoolImplBase* self,
                                                        RpPendingStreamPtr stream,
                                                        RpCancelPolicy_e policy);
float rp_conn_pool_impl_base_per_upstream_preconnect_ratio(RpConnPoolImplBase* self);
void rp_conn_pool_impl_base_add_idle_callback_impl(RpConnPoolImplBase* self,
                                                    idle_cb cb);
bool rp_conn_pool_impl_base_is_idle_impl(RpConnPoolImplBase* self);
void rp_conn_pool_impl_base_drain_connections_impl(RpConnPoolImplBase* self,
                                                    RpDrainBehavior_e drain_behavior);
void rp_conn_pool_impl_base_destruct_all_connections(RpConnPoolImplBase* self);
RpCancellable* rp_conn_pool_impl_base_new_stream_impl(RpConnPoolImplBase* self,
                                                        RpConnectionPoolAttachContextPtr context,
                                                        bool can_send_early_data);
bool rp_conn_pool_impl_base_maybe_preconnect_impl(RpConnPoolImplBase* self,
                                                    float global_preconnect_ratio);
bool rp_conn_pool_impl_base_has_pending_streams(RpConnPoolImplBase* self);
bool rp_conn_pool_impl_base_has_active_streams(RpConnPoolImplBase* self);
RpDispatcher* rp_conn_pool_impl_base_dispatcher(RpConnPoolImplBase* self);
void rp_conn_pool_impl_base_schedule_on_upstream_ready(RpConnPoolImplBase* self);
void rp_conn_pool_impl_base_check_for_idle_and_close_idle_conns_if_draining(RpConnPoolImplBase* self);
RpCancellable* rp_conn_pool_impl_base_add_pending_stream(RpConnPoolImplBase* self,
                                                            RpPendingStreamPtr pending_stream);
void rp_conn_pool_impl_base_on_connection_event(RpConnPoolImplBase* self,
                                                RpConnectionPoolActiveClientPtr client,
                                                const char* failure_reason,
                                                RpNetworkConnectionEvent_e event);
void rp_conn_pool_impl_base_on_stream_closed(RpConnPoolImplBase* self,
                                                RpConnectionPoolActiveClientPtr client,
                                                bool delay_attaching_stream);
GList** rp_conn_pool_impl_base_ready_clients_(RpConnPoolImplBase* self);
GList** rp_conn_pool_impl_base_busy_clients_(RpConnPoolImplBase* self);
GList** rp_conn_pool_impl_base_connecting_clients_(RpConnPoolImplBase* self);

G_END_DECLS
