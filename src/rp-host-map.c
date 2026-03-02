/*
 * rp-host-map.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_host_map_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_host_map_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-host-map-snap.h"
#include "rp-upstream.h"

struct _RpHostMap {
    gatomicrefcount ref_count;

    GHashTable* m_host_map; // absl::flat_hash_map<std::string, Upstream::HostSharedPtr>
};

RpHostMap*
rp_host_map_new(void)
{
    LOGD("()");
    RpHostMap* self = g_new0(RpHostMap, 1);
    self->m_host_map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
    g_atomic_ref_count_init(&self->ref_count);
    return self;
}

static inline void
rp_host_map_free(RpHostMap* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(self != NULL);
    g_hash_table_destroy(self->m_host_map);
    g_free(self);
}

RpHostMap*
rp_host_map_ref(RpHostMap* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(self != NULL, NULL);
    g_atomic_ref_count_inc(&self->ref_count);
    return self;
}

void
rp_host_map_unref(const RpHostMap* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(self != NULL);
    if (g_atomic_ref_count_dec((gatomicrefcount*)&self->ref_count))
    {
        NOISY_MSG_("freeing %p", self);
        rp_host_map_free((RpHostMap*)self);
    }
}

guint
rp_host_map_ref_count(const RpHostMap* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(self != NULL, G_MAXUINT);
    return self->ref_count;
}

static inline const char*
key_from_host(RpHost* host)
{
    RpNetworkAddressInstanceConstSharedPtr addr = rp_host_description_address((RpHostDescriptionConstSharedPtr)host);
    return rp_network_address_instance_as_string(addr);
}

void
rp_host_map_add_take(RpHostMap* self, RpHost* host)
{
    LOGD("(%p, %p(%u))", self, host, G_OBJECT(host)->ref_count);
    g_return_if_fail(self != NULL);
    g_return_if_fail(RP_IS_HOST(host));
    // self must unref elements; this transfers ownership of hosts's current ref into self
    const char* key = key_from_host(host);
    if (!g_hash_table_replace(self->m_host_map, g_strdup(key), (gpointer)host))
    {
        LOGD("key \"%s\" already exists", key);
    }
}

void
rp_host_map_add(RpHostMap* self, RpHost* host)
{
    LOGD("(%p, %p(%u))", self, host, G_OBJECT(host)->ref_count);
    g_return_if_fail(RP_IS_HOST(host));
    rp_host_map_add_take(self, g_object_ref(host));
}

RpHost*
rp_host_map_find(const RpHostMap* self, RpHost* host)
{
    LOGD("(%p, %p)", self, host);
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(RP_IS_HOST(host), NULL);
    return g_hash_table_lookup(self->m_host_map, key_from_host(host));
}

gboolean
rp_host_map_remove(RpHostMap* self, RpHost* host)
{
    LOGD("(%p, %p)", self, host);
    g_return_val_if_fail(self != NULL, false);
    g_return_val_if_fail(RP_IS_HOST(host), false);
    return g_hash_table_remove(self->m_host_map, key_from_host(host));
}

bool
rp_host_map_is_empty(const RpHostMap* self)
{
    return !self || !g_hash_table_size(self->m_host_map);
}

const GHashTable*
rp_host_map_hash_table(const RpHostMap* self)
{
    return self ? self->m_host_map : NULL;
}

guint
rp_host_map_size(const RpHostMap* self)
{
    return self ? g_hash_table_size(self->m_host_map) : 0;
}

RpHostMap*
rp_host_map_clone_from_snapshot(const RpHostMapSnap* src)
{
    LOGD("(%p)", src);
    g_return_val_if_fail(src != NULL, NULL);

    RpHostMap* self = rp_host_map_new();
    RpHostMap* src_ = rp_host_map_snap_host_map(src);

    GHashTableIter it;
    gpointer k, v;
    g_hash_table_iter_init(&it, src_->m_host_map);
    while (g_hash_table_iter_next(&it, &k, &v))
    {
        g_hash_table_insert(self->m_host_map, g_strdup((const gchar*)k), g_object_ref(v)); // RpHost*
    }
    return self;
}
