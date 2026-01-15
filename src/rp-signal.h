/*
 * rp-signal.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"

G_BEGIN_DECLS

typedef int signal_t;

/**
 * Callback invoked when a signal event fires.
 */
typedef void (*RpSignalCb)(gpointer);

/**
 * An abstract signal event. Free the event to stop listening on the signal.
 */
#define RP_TYPE_SIGNAL_EVENT rp_signal_event_get_type()
G_DECLARE_INTERFACE(RpSignalEvent, rp_signal_event, RP, SIGNAL_EVENT, GObject)

struct _RpSignalEventInterface {
    GTypeInterface parent_iface;

};

typedef UNIQUE_PTR(RpSignalEvent) RpSignalEventPtr;

G_END_DECLS
