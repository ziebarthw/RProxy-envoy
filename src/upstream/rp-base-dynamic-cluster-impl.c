/*
 * rp-base-dynamic-cluster-impl.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_base_dynamic_cluster_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_base_dynamic_cluster_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-upstream-impl.h"

G_DEFINE_ABSTRACT_TYPE(RpBaseDynamicClusterImpl, rp_base_dynamic_cluster_impl, RP_TYPE_CLUSTER_IMPL_BASE)

static void
rp_base_dynamic_cluster_impl_class_init(RpBaseDynamicClusterImplClass* klass G_GNUC_UNUSED)
{
    LOGD("(%p)", klass);
}

static void
rp_base_dynamic_cluster_impl_init(RpBaseDynamicClusterImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpHostVector*
ensure_host_vector(RpHostVector** host_vector)
{
    if (*host_vector)
    {
        NOISY_MSG_("pre-allocated host vector %p", *host_vector);
        return *host_vector;
    }
    *host_vector = rp_host_vector_new();
    NOISY_MSG_("allocated host vector %p", *host_vector);
    return *host_vector;
}

bool
rp_base_dynamic_cluster_impl_update_dynamic_host_list(RpBaseDynamicClusterImpl* self, const RpHostVector* new_hosts,
                                                        RpHostVector** current_priority_hosts, RpHostVector** hosts_added_to_current_priorty,
                                                        RpHostVector** hosts_removed_from_current_priority, const RpHostMap* all_hosts,
                                                        GHashTable* all_new_hosts)
{
    LOGD("(%p, %p, %p, %p, %p, %p, %p)",
        self, new_hosts, current_priority_hosts, hosts_added_to_current_priorty, hosts_removed_from_current_priority, all_hosts, all_new_hosts);

    g_return_val_if_fail(RP_IS_BASE_DYNAMIC_CLUSTER_IMPL(self), false);

    guint64 max_host_weight = 1;
    bool hosts_changed = false;

    g_autoptr(GHashTable) existing_hosts_for_current_priority =
        g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
//TODO...    GHashTable* hosts_with_updated_locality_for_current_priority =
//        g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
    RpHostVector* final_hosts = rp_host_vector_new();
    for (guint i = 0; i < rp_host_vector_len(new_hosts); ++i)
    {
        RpHost* host = rp_host_vector_get(new_hosts, i);
        RpHost* existing_host = rp_host_map_find(all_hosts, host);

        if (existing_host)
        {
            //TODO...existing_host->second->healthFlagClear(...);
        }

        //TODO...individual host members updated checks...

        rp_host_vector_add(final_hosts, host);
        rp_host_vector_add(ensure_host_vector(hosts_added_to_current_priorty), host);
    }

    //TODO...remove hosts logic...

    if (g_hash_table_size(existing_hosts_for_current_priority) > 0/*!empty()*/)
    {
        hosts_changed = true;
    }

    //TODO...

    if (!rp_host_vector_is_empty(*hosts_added_to_current_priorty) ||
        !rp_host_vector_is_empty(*current_priority_hosts))
    {
        *hosts_removed_from_current_priority = g_steal_pointer(current_priority_hosts);
        hosts_changed = true;
    }

    *current_priority_hosts = g_steal_pointer(&final_hosts);
    return hosts_changed;
}
