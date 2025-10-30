/*
 * rp-timer-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "event/rp-event-impl-base.h"
#include "rp-dispatcher.h"
#include "rp-timer.h"

G_BEGIN_DECLS

typedef struct event_base evbase_t;

/**
 * libevent implementation of Timer.
 */
#define RP_TYPE_TIMER_IMPL rp_timer_impl_get_type()
G_DECLARE_FINAL_TYPE(RpTimerImpl, rp_timer_impl, RP, TIMER_IMPL, RpEventImplBase)

RpTimerImpl* rp_timer_impl_new(evbase_t* evbase,
                                RpTimerCb cb,
                                gpointer arg,
                                RpDispatcher* dispatcher);

G_END_DECLS
