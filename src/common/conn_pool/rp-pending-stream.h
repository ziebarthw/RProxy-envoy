/*
 * rp-pending-stream.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-conn-pool-base.h"

G_BEGIN_DECLS

/*
 * PendingStream is the base class tracking streams for which a connection has been created but not
 * yet established.
 */
#define RP_TYPE_PENDING_STREAM rp_pending_stream_get_type()
G_DECLARE_DERIVABLE_TYPE(RpPendingStream, rp_pending_stream, RP, PENDING_STREAM, GObject)

struct _RpPendingStreamClass {
    GObjectClass parent_class;

    RpConnectionPoolAttachContextPtr (*context)(RpPendingStream*);
};

static inline RpConnectionPoolAttachContextPtr
rp_pending_stream_context(RpPendingStream* self)
{
    return RP_IS_PENDING_STREAM(self) ?
        RP_PENDING_STREAM_GET_CLASS(self)->context(self) : NULL;
}
bool rp_pending_stream_can_send_early_data_(RpPendingStream* self);

G_END_DECLS
