/*
 * rp-callback-impl.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-callback.h"

G_BEGIN_DECLS

/**
 * Utility class for managing callbacks.
 *
 * @see ThreadSafeCallbackManager for dealing with callbacks across multiple threads
 */
#define RP_TYPE_CALLBACK_MANAGER rp_callback_manager_get_type()
G_DECLARE_FINAL_TYPE(RpCallbackManager, rp_callback_manager, RP, CALLBACK_MANAGER, GObject)

RpCallbackManager* rp_callback_manager_new(void);
RpCallbackHandlePtr rp_callback_manager_add(RpCallbackManager* self, RpCallback cb, gpointer arg);
GList* rp_callback_manager_callbacks(RpCallbackManager* self);

#define RP_TYPE_CALLBACK_HOLDER rp_callback_holder_get_type()
G_DECLARE_FINAL_TYPE(RpCallbackHolder, rp_callback_holder, RP, CALLBACK_HOLDER, GObject)

RpCallbackHolder* rp_callback_holder_new(RpCallbackManager* parent, RpCallback cb, gpointer arg);

G_END_DECLS
