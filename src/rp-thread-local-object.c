/*
 * rp-thread-local-object.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "macrologger.h"

#if (defined(rp_thread_local_object_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_thread_local_object_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-thread-local-object.h"

G_DEFINE_INTERFACE(RpThreadLocalObject, rp_thread_local_object, G_TYPE_OBJECT)
static void
rp_thread_local_object_default_init(RpThreadLocalObjectInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
