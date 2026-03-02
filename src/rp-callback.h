/*
 * rp-callback.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"
#include "rp-codec.h"

G_BEGIN_DECLS

/**
 * Handle for a callback that can be removed. Destruction of the handle removes the
 * callback.
 */
#define RP_TYPE_CALLBACK_HANDLE rp_callback_handle_get_type()
G_DECLARE_INTERFACE(RpCallbackHandle, rp_callback_handle, RP, CALLBACK_HANDLE, GObject)

typedef RpStatusCode_e (*RpCallback)(gpointer);

struct _RpCallbackHandleInterface {
    GTypeInterface parent_iface;

    RpCallback (*cb)(RpCallbackHandle*);
    gpointer (*arg)(RpCallbackHandle*);
};

typedef UNIQUE_PTR(RpCallbackHandle) RpCallbackHandlePtr;

static inline RpCallback
rp_callback_handle_cb(RpCallbackHandle* self)
{
    return RP_IS_CALLBACK_HANDLE(self) ?
        RP_CALLBACK_HANDLE_GET_IFACE(self)->cb(self) : NULL;
}
static inline gpointer
rp_callback_handle_arg(RpCallbackHandle* self)
{
    return RP_IS_CALLBACK_HANDLE(self) ?
        RP_CALLBACK_HANDLE_GET_IFACE(self)->arg(self) : NULL;
}

G_END_DECLS
