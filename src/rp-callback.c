/*
 * rp-callback.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "macrologger.h"

#if (defined(rp_callback_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_callback_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-callback.h"

G_DEFINE_INTERFACE(RpCallbackHandle, rp_callback_handle, G_TYPE_OBJECT)
static void
rp_callback_handle_default_init(RpCallbackHandleInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
