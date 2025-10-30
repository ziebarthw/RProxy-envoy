/*
 * rp-net-conn-impl-base.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-dispatcher.h"
#include "rp-net-filter-mgr-impl.h"

G_BEGIN_DECLS

typedef enum {
    RpDelayedCloseState_None,
    RpDelayedCloseState_CloseAfterFlush,
    RpDelayedCloseState_CloseAfterFlushAndWait
} RpDelayedCloseState_e;

#define RP_TYPE_NETWORK_CONNECTION_IMPL_BASE rp_network_connection_impl_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpNetworkConnectionImplBase, rp_network_connection_impl_base, RP, NETWORK_CONNECTION_IMPL_BASE, GObject)

struct _RpNetworkConnectionImplBaseClass {
    GObjectClass parent_class;

    void (*close_connection_immediately)(RpNetworkConnectionImplBase*);
};

static inline void
rp_network_connection_impl_base_close_connection_immediately(RpNetworkConnectionImplBase* self)
{
    if (RP_IS_NETWORK_CONNECTION_IMPL_BASE(self))
    {
        RP_NETWORK_CONNECTION_IMPL_BASE_GET_CLASS(self)->close_connection_immediately(self);
    }
}

bool rp_network_connection_impl_base_in_delayed_close(RpNetworkConnectionImplBase* self);
void rp_network_connection_impl_base_raise_connection_event(RpNetworkConnectionImplBase* self,
                                                            RpNetworkConnectionEvent_e event);
void rp_network_connection_impl_base_set_local_close_reason(RpNetworkConnectionImplBase* self,
                                                            const char* local_close_reason);
RpDispatcher* rp_network_connection_impl_base_dispatcher_(RpNetworkConnectionImplBase* self);

G_END_DECLS
