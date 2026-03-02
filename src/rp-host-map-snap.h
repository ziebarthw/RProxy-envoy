/*
 * rp-host-map-snap.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-upstream.h"

G_BEGIN_DECLS

RpHostMapSnap* rp_host_map_snap_new_empty_frozen(void);
RpHostMapSnap* rp_host_map_snap_ref(RpHostMapSnap* self);
void rp_host_map_snap_unref(RpHostMapSnap* self);
guint rp_host_map_snap_ref_count(RpHostMapSnap* self);
RpHostMapSnap* rp_host_map_snap_from_mutable_and_freeze(const RpHostMap* src);
RpHostMap* rp_host_map_snap_host_map(const RpHostMapSnap* self);
void rp_host_map_snap_freeze(RpHostMapSnap* self);
bool rp_host_map_snap_frozen(RpHostMapSnap* self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(RpHostMapSnap, rp_host_map_snap_unref)

static inline void
rp_host_map_snap_atomic_set_ref(RpHostMapSnap **dst, RpHostMapSnap *src /* transfer none */)
{
    RpHostMapSnap *incoming = src ? rp_host_map_snap_ref(src) : NULL;
    RpHostMapSnap *prev = g_atomic_pointer_exchange((gpointer*)dst, incoming);
    g_assert(prev != incoming);
    if (prev) rp_host_map_snap_unref(prev);
}

static inline void
rp_host_map_snap_atomic_publish_steal(RpHostMapSnap **dst,
                                     RpHostMapSnap **src_ptr /* transfer full */)
{
    RpHostMapSnap *incoming = src_ptr ? *src_ptr : NULL;
    if (src_ptr) *src_ptr = NULL;  // steal so caller can't accidentally unref later
    RpHostMapSnap *prev = g_atomic_pointer_exchange((gpointer*)dst, incoming);
    // If this ever fires, you published the exact same pointer you already had.
    // Not necessarily fatal, but it usually indicates redundant work or a logic bug.
    g_assert(prev != incoming);
    if (prev) rp_host_map_snap_unref(prev);
}

G_END_DECLS
