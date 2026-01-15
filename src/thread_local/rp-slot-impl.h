/*
 * rp-slot-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "thread_local/rp-thread-local-impl.h"

G_BEGIN_DECLS

#define RP_TYPE_SLOT_IMPL rp_slot_impl_get_type()
G_DECLARE_FINAL_TYPE(RpSlotImpl, rp_slot_impl, RP, SLOT_IMPL, GObject);

RpSlotPtr rp_slot_impl_new(RpThreadLocalInstanceImpl* parent, guint32 index);

G_END_DECLS
