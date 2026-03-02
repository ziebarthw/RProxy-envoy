/*
 * rp-host-map-snap.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_host_map_snap_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_host_map_snap_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-host-map-snap.h"

struct _RpHostMapSnap {
  gatomicrefcount ref_count;
  RpHostMap* m_map; // key: gchar* (owned), value: RpHost* (ref'd)
  bool m_frozen;    // debug aid: prevent accidental mutation after publish
};

RpHostMapSnap*
rp_host_map_snap_new_empty_frozen(void)
{
    LOGD("()");
    RpHostMapSnap* self = g_new0(RpHostMapSnap, 1);
    g_atomic_ref_count_init(&self->ref_count);

    self->m_map = rp_host_map_new();
    self->m_frozen = TRUE; // published snapshots are frozen
    return self;
}

RpHostMapSnap*
rp_host_map_snap_ref(RpHostMapSnap* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(self != NULL, NULL);
    g_atomic_ref_count_inc(&self->ref_count);
    return self;
}

void
rp_host_map_snap_unref(RpHostMapSnap* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(self != NULL);
    if (g_atomic_ref_count_dec(&self->ref_count))
    {
        NOISY_MSG_("freeing %p, host map %p", self, self->m_map);
        rp_host_map_unref(self->m_map);
        g_free(self);
    }
}

guint
rp_host_map_snap_ref_count(RpHostMapSnap* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(self != NULL, G_MAXUINT);
    return self->ref_count;
}

RpHostMapSnap*
rp_host_map_snap_from_mutable_and_freeze(const RpHostMap* src)
{
    LOGD("(%p)", src);
    g_return_val_if_fail(src != NULL, NULL);

    RpHostMapSnap* dst = rp_host_map_snap_new_empty_frozen();
    dst->m_frozen = FALSE;

    const GHashTable* src_ht = rp_host_map_hash_table(src);
    GHashTableIter it;
    gpointer k, v;
    g_hash_table_iter_init(&it, (GHashTable*)src_ht);
    while (g_hash_table_iter_next(&it, &k, &v))
    {
        rp_host_map_add(dst->m_map, v);
    }

    dst->m_frozen = TRUE;
    return dst;
}

void
rp_host_map_snap_freeze(RpHostMapSnap* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(self != NULL);
    self->m_frozen = TRUE;
}

bool
rp_host_map_snap_frozen(RpHostMapSnap* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(self != NULL, false);
    return self->m_frozen;
}

RpHostMap*
rp_host_map_snap_host_map(const RpHostMapSnap* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(self != NULL, NULL);
    return self->m_map;
}
