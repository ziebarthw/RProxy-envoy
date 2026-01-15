/*
 * rp-thread-local.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "macrologger.h"

#if (defined(rp_thread_local_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_thread_local_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-thread-local.h"

G_DEFINE_INTERFACE(RpSlot, rp_slot, G_TYPE_OBJECT)
static void
rp_slot_default_init(RpSlotInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpSlotAllocator, rp_slot_allocator, G_TYPE_OBJECT)
static void
rp_slot_allocator_default_init(RpSlotAllocatorInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpThreadLocalInstance, rp_thread_local_instance, RP_TYPE_SLOT_ALLOCATOR)
static void
rp_thread_local_instance_default_init(RpThreadLocalInstanceInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
