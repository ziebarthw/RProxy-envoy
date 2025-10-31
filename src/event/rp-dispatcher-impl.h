/*
 * rp-dispatcher-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"
#include "rp-dispatcher.h"
#include "rp-time.h"

G_BEGIN_DECLS

/**
 * libevent implementation of Event::Dispatcher.
 */
#define RP_TYPE_DISPATCHER_IMPL rp_dispatcher_impl_get_type()
G_DECLARE_FINAL_TYPE(RpDispatcherImpl, rp_dispatcher_impl, RP, DISPATCHER_IMPL, GObject)

RpDispatcherImpl* rp_dispatcher_impl_new(const char* name,
                                            UNIQUE_PTR(RpTimeSystem) time_system,
                                            SHARED_PTR(evthr_t) thr);
evbase_t* rp_dispatcher_base(RpDispatcherImpl* self);
evthr_t* rp_dispatcher_thr(RpDispatcherImpl* self);

G_END_DECLS
