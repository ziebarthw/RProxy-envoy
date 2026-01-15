/*
 * rp-priority-set-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "macrologger.h"

#if (defined(rp_priority_set_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_priority_set_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-upstream-impl.h"

typedef struct _RpPrioritySetImplPrivate RpPrioritySetImplPrivate;
struct _RpPrioritySetImplPrivate {

    UNIQUE_PTR(GPtrArray)/*<std::unique_ptr<HostSet>>*/ m_host_sets;
    /*mutable*/ RpHostMap* m_const_cross_priority_host_map;//never null
    RpPrioritySetImpl* m_parent;
    bool m_batch_update;
};

static void priority_set_iface_init(RpPrioritySetInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpPrioritySetImpl, rp_priority_set_impl, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpPrioritySetImpl)
    G_IMPLEMENT_INTERFACE(RP_TYPE_PRIORITY_SET, priority_set_iface_init)
)

#define PRIV(obj) \
    ((RpPrioritySetImplPrivate*) rp_priority_set_impl_get_instance_private(RP_PRIORITY_SET_IMPL(obj)))

static GPtrArray*
host_sets_per_priority_i(RpPrioritySet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_host_sets;
}

static RpHostMapConstSharedPtr
cross_priority_host_map_i(RpPrioritySet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_const_cross_priority_host_map;
}

static void
priority_set_iface_init(RpPrioritySetInterface* iface)
{
    LOGD("(%p)", iface);
    iface->host_sets_per_priority = host_sets_per_priority_i;
    iface->cross_priority_host_map = cross_priority_host_map_i;
}

OVERRIDE void
dispose(GObject* object)
{
    NOISY_MSG_("(%p)", object);
    G_OBJECT_CLASS(rp_priority_set_impl_parent_class)->dispose(object);
}

OVERRIDE RpHostSetImplPtr
create_host_set(RpPrioritySetImpl* self, guint32 priority)
{
    NOISY_MSG_("(%p, %u)", self, priority);
    return g_object_new(RP_TYPE_HOST_SET_IMPL, "priority", priority, NULL);
}

OVERRIDE RpStatusCode_e
run_update_callbacks(RpPrioritySetImpl* self, RpHostVector* hosts_added, RpHostVector* hosts_removed)
{
    NOISY_MSG_("(%p, %p, %p)", self, hosts_added, hosts_removed);
//TODO...
return RpStatusCode_Ok;
}

OVERRIDE RpStatusCode_e
run_reference_update_callbacks(RpPrioritySetImpl* self, guint32 priority, RpHostVector* hosts_added, RpHostVector* hosts_removed)
{
    NOISY_MSG_("(%p, %u, %p, %p)", self, priority, hosts_added, hosts_removed);
//TODO...
return RpStatusCode_Ok;
}

static void
rp_priority_set_impl_class_init(RpPrioritySetImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    klass->create_host_set = create_host_set;
    klass->run_update_callbacks = run_update_callbacks;
    klass->run_reference_update_callbacks = run_reference_update_callbacks;
}

static void
rp_priority_set_impl_init(RpPrioritySetImpl* self)
{
    LOGD("(%p)", self);
    RpPrioritySetImplPrivate* me = PRIV(self);
    me->m_batch_update = false;
    me->m_host_sets = g_ptr_array_new();
    RpHostMap* host_map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)g_hash_table_unref/*HostSharedPtr*/);
    // REVISIT - this is really not applicable/reproducable in C.(?)
    me->m_const_cross_priority_host_map = host_map;
}

RpPrioritySetImpl*
rp_priority_set_impl_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_PRIORITY_SET_IMPL, NULL);
}

const RpHostSet*
rp_priority_set_impl_get_or_create_host_set(RpPrioritySetImpl* self, guint32 priority)
{
    LOGD("(%p, %u)", self, priority);
    g_return_val_if_fail(RP_IS_PRIORITY_SET_IMPL(self), NULL);
    RpPrioritySetImplPrivate* me = PRIV(self);
    if (me->m_host_sets->len < priority + 1)
    {
        for (guint i = me->m_host_sets->len; i <= priority; ++i)
        {
            RpHostSetImplPtr host_set = rp_priority_set_impl_create_host_set(self, i);
            //TODO...host_sets_priority_update_cbs_.push_back(...)
            g_ptr_array_add(me->m_host_sets, g_steal_pointer(&host_set));
        }
    }
    return me->m_host_sets->pdata[priority];
}

RpHostMap**
rp_priority_set_impl_const_cross_priority_host_map_(RpPrioritySetImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_PRIORITY_SET_IMPL(self), NULL);
    return &PRIV(self)->m_const_cross_priority_host_map;
}
