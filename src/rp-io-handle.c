/*
 * rp-io-handle.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_io_handle_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_io_handle_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-io-handle.h"

G_DEFINE_INTERFACE(RpIoHandle, rp_io_handle, G_TYPE_OBJECT)
static void
rp_io_handle_default_init(RpIoHandleInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
