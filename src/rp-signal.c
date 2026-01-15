/*
 * rp-signal.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_signal_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_signal_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-signal.h"

G_DEFINE_INTERFACE(RpSignalEvent, rp_signal_event, G_TYPE_OBJECT);
static void
rp_signal_event_default_init(RpSignalEventInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
