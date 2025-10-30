/*
 * rp-stream-reset-handler.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#include "rp-stream-reset-handler.h"

G_DEFINE_INTERFACE(RpStreamResetHandler, rp_stream_reset_handler, G_TYPE_OBJECT)

static void
reset_stream_i(RpStreamResetHandler* self G_GNUC_UNUSED, RpStreamResetReason_e reason G_GNUC_UNUSED)
{
    g_debug("%s:%s - not implemented [%d]", __FILE__, __func__, __LINE__);
}

static void
rp_stream_reset_handler_default_init(RpStreamResetHandlerInterface* iface)
{
    iface->reset_stream = reset_stream_i;
}
