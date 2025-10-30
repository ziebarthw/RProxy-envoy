/*
 * rp-event-impl-base.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <event2/event_struct.h>

G_BEGIN_DECLS

/**
 * Base class for libevent event implementations. The event struct is embedded inside of this class
 * and derived classes are expected to assign it inside of the constructor.
 */
#define RP_TYPE_EVENT_IMPL_BASE rp_event_impl_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpEventImplBase, rp_event_impl_base, RP, EVENT_IMPL_BASE, GObject)

struct _RpEventImplBaseClass {
    GObjectClass parent_class;

};

struct event* rp_event_impl_base_raw_event_(RpEventImplBase* self);

G_END_DECLS
