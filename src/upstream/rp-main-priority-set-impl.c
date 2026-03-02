/*
 * rp-main-priority-set-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_main_priority_set_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_main_priority_set_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-host-map-snap.h"
#include "upstream/rp-upstream-impl.h"

struct _RpMainPrioritySetImpl {
    RpPrioritySetImpl parent_instance;

    /*mutable*/ RpHostMap* m_mutable_cross_priority_host_map;
};

static void priority_set_iface_init(RpPrioritySetInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpMainPrioritySetImpl, rp_main_priority_set_impl, RP_TYPE_PRIORITY_SET_IMPL,
    G_IMPLEMENT_INTERFACE(RP_TYPE_PRIORITY_SET, priority_set_iface_init)
)

#define PARENT_PRIORITY_SET_IFACE(s) \
    ((RpPrioritySetInterface*)g_type_interface_peek_parent(RP_PRIORITY_SET_GET_IFACE(s)))

static const RpHostSetPtrVector*
host_sets_per_priority_i(RpPrioritySet* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_PRIORITY_SET_IFACE(self)->host_sets_per_priority(self);
}

static RpHostMapSnap*
cross_priority_host_map_i(RpPrioritySet* self)
{
    NOISY_MSG_("(%p)", self);

    RpMainPrioritySetImpl* me = RP_MAIN_PRIORITY_SET_IMPL(self);
    RpHostMapSnap** const_cross_priority_host_map_ =
        rp_priority_set_impl_const_cross_priority_host_map_(RP_PRIORITY_SET_IMPL(self));
    // Publish pending edits (main thread only)
    if (me->m_mutable_cross_priority_host_map != NULL)
    {
        RpHostMapSnap* new_snap = rp_host_map_snap_from_mutable_and_freeze(me->m_mutable_cross_priority_host_map);

        g_clear_pointer(&me->m_mutable_cross_priority_host_map, rp_host_map_unref);
        rp_host_map_snap_atomic_publish_steal(const_cross_priority_host_map_, &new_snap);
        // new_snap is now NULL
        g_assert(me->m_mutable_cross_priority_host_map == NULL);
    }

    // Return a retained snapshot
    RpHostMapSnap* snap = g_atomic_pointer_get(const_cross_priority_host_map_);
    g_assert(snap != NULL);
    return rp_host_map_snap_ref(snap);
}

static void
update_cross_priority_host_map(RpMainPrioritySetImpl* self, guint32 priority,
                                const RpHostVector* hosts_added, const RpHostVector* hosts_removed)
{
    NOISY_MSG_("(%p, %u, %p, %p)", self, priority, hosts_added, hosts_removed);

    if (rp_host_vector_is_empty(hosts_added) && rp_host_vector_is_empty(hosts_removed))
    {
        NOISY_MSG_("empty");
        return;
    }

    // Lazy-create working copy from the published snapshot
    if (self->m_mutable_cross_priority_host_map == NULL)
    {
        RpHostMapSnap* snap = g_atomic_pointer_get(
            rp_priority_set_impl_const_cross_priority_host_map_(RP_PRIORITY_SET_IMPL(self)));
        self->m_mutable_cross_priority_host_map = rp_host_map_clone_from_snapshot(snap);
    }

    // removals: only erase if existing host's priority == priority
    for (guint i = 0; i < rp_host_vector_len(hosts_removed); ++i)
    {
        RpHost* host = rp_host_vector_get(hosts_removed, i);
        RpHost* existing = rp_host_map_find(self->m_mutable_cross_priority_host_map, host);
        if (existing &&
            rp_host_description_priority((RpHostDescriptionConstSharedPtr)existing) == priority)
        {
            rp_host_map_remove(self->m_mutable_cross_priority_host_map, existing);
        }
    }

    // additions: insert/replace addr->host
    for (guint i = 0; i < rp_host_vector_len(hosts_added); ++i)
    {
        RpHost* host = rp_host_vector_get(hosts_added, i);
        if (!rp_host_map_find(self->m_mutable_cross_priority_host_map, host))
        {
            NOISY_MSG_("%p, adding host %p to mutable cross priority host map %p", self, host, self->m_mutable_cross_priority_host_map);
            rp_host_map_add(self->m_mutable_cross_priority_host_map, host);
        }
    }
}

static void
update_hosts_i(RpPrioritySet* self, guint32 priority, RpPrioritySetUpdateHostsParams* params,
                const RpHostVector* hosts_added, const RpHostVector* hosts_removed, bool* weighted_priority_health, guint32* overprovisioning_factor,
                RpHostMapSnap* cross_priority_host_map)
{
    NOISY_MSG_("(%p, %u, %p, %p, %p, %p, %p, %p)",
        self, priority, params, hosts_added, hosts_removed, weighted_priority_health, overprovisioning_factor, cross_priority_host_map);
    update_cross_priority_host_map(RP_MAIN_PRIORITY_SET_IMPL(self), priority, hosts_added, hosts_removed);
    PARENT_PRIORITY_SET_IFACE(self)->update_hosts(self,
                                                    priority,
                                                    params,
                                                    hosts_added,
                                                    hosts_removed,
                                                    weighted_priority_health,
                                                    overprovisioning_factor,
                                                    NULL);
}

static void
priority_set_iface_init(RpPrioritySetInterface* iface)
{
    LOGD("(%p)", iface);
    iface->host_sets_per_priority = host_sets_per_priority_i;
    iface->cross_priority_host_map = cross_priority_host_map_i;
    iface->update_hosts = update_hosts_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_main_priority_set_impl_parent_class)->dispose(obj);
}

static void
rp_main_priority_set_impl_class_init(RpMainPrioritySetImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_main_priority_set_impl_init(RpMainPrioritySetImpl* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_mutable_cross_priority_host_map = NULL;
}

RpMainPrioritySetImpl*
rp_main_priority_set_impl_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_MAIN_PRIORITY_SET_IMPL, NULL);
}
