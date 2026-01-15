/*
 * rp-signal-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "event/rp-dispatcher-impl.h"
#include "event/rp-event-impl-base.h"

G_BEGIN_DECLS

/**
 * libevent implementation of Event::SignalEvent.
 */
#define RP_TYPE_SIGNAL_EVENT_IMPL rp_signal_event_impl_get_type()
G_DECLARE_FINAL_TYPE(RpSignalEventImpl, rp_signal_event_impl, RP, SIGNAL_EVENT_IMPL, RpEventImplBase)

RpSignalEvent* rp_signal_event_impl_new(RpDispatcherImpl* dispatcher,
                                        signal_t signal_num,
                                        RpSignalCb cb,
                                        gpointer arg);

G_END_DECLS
