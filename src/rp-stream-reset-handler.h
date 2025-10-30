/*
 * rp-stream-reset-handler.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * Stream reset reasons.
 */
typedef enum {
    RpStreamResetReason_LocalReset,
    RpStreamResetReason_LocalRefusedStreamReset,
    RpStreamResetReason_RemoteReset,
    RpStreamResetReason_RemoteRefusedStreamReset,
    RpStreamResetReason_LocalConnectionFailure,
    RpStreamResetReason_RemoteConnectionFailure,
    RpStreamResetReason_ConnectionTimeout,
    RpStreamResetReason_ConnectionTermination,
    RpStreamResetReason_Overflow,
    RpStreamResetReason_ConnectError,
    RpStreamResetReason_ProtocolError,
    RpStreamResetReason_OverloadManager,
    RpStreamResetReason_Http1PrematureUpstreamHalfClose,
} RpStreamResetReason_e;

/**
 * Handler to reset an underlying HTTP stream.
 */
#define RP_TYPE_STREAM_RESET_HANDLER rp_stream_reset_handler_get_type()
G_DECLARE_INTERFACE(RpStreamResetHandler, rp_stream_reset_handler, RP, STREAM_RESET_HANDLER, GObject)

struct _RpStreamResetHandlerInterface {
    GTypeInterface parent_iface;

    void (*reset_stream)(RpStreamResetHandler*, RpStreamResetReason_e);
};

static inline void
rp_stream_reset_handler_reset_stream(RpStreamResetHandler* self, RpStreamResetReason_e reason)
{
    if (RP_IS_STREAM_RESET_HANDLER(self))
    {
        RP_STREAM_RESET_HANDLER_GET_IFACE(self)->reset_stream(self, reason);
    }
}

G_END_DECLS
