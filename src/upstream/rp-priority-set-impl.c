/*
 * rp-priority-set-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_priority_set_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_priority_set_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-priority-set-impl.h"

typedef struct _RpPrioritySetImplPrivate RpPrioritySetImplPrivate;
struct _RpPrioritySetImplPrivate {

    GArray* m_host_sets;
    RpHostMap m_const_cross_priority_host_map;//never null
    RpPrioritySetImpl* m_parent;
    bool m_batch_update;
};

static void priority_set_iface_init(RpPrioritySetInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpPrioritySetImpl, rp_priority_set_impl, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpPrioritySetImpl)
    G_IMPLEMENT_INTERFACE(RP_TYPE_PRIORITY_SET_IMPL, priority_set_iface_init)
)

#define PRIV(obj) \
    ((RpPrioritySetImplPrivate*) rp_priority_set_impl_get_instance_private(RP_PRIORITY_SET_IMPL(obj)))

static GArray*
host_sets_per_priority_i(RpPrioritySet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_host_sets;
}

static RpHostMap
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

OVERRIDE GObject*
constructor(GType type, guint n_construct_properties, GObjectConstructParam *construct_properties)
{
    LOGD("(%lu, %u, %p)", type, n_construct_properties, construct_properties);

    GObject* obj = G_OBJECT_CLASS(rp_priority_set_impl_parent_class)->constructor(type, n_construct_properties, construct_properties);
    NOISY_MSG_("obj %p", obj);

    RpPrioritySetImpl* self = RP_PRIORITY_SET_IMPL(obj);

    return obj;
}

OVERRIDE void
dispose(GObject* object)
{
    LOGD("(%p)", object);
    G_OBJECT_CLASS(rp_priority_set_impl_parent_class)->dispose(object);
}

OVERRIDE RpHostSetImpl*
create_host_set(RpPrioritySetImpl* self, guint32 priority, bool weighted_priority_health, guint32 overprovisioning_factor)
{
    NOISY_MSG_("(%p, %u, %u, %u)", self, priority, weighted_priority_health, overprovisioning_factor);
    return g_object_new(RP_TYPE_HOST_SET_IMPL,
                        "priority", priority,
                        "weighted-priority-health", weighted_priority_health,
                        "overprovisioning-factor", overprovisioning_factor,
                        NULL);
}

OVERRIDE RpStatusCode_e
run_update_callbacks(RpPrioritySetImpl* self, RpHostVector hosts_added, RpHostVector hosts_removed)
{
    NOISY_MSG_("(%p, %p, %p)", self, hosts_added, hosts_removed);
//TODO...
return RpStatusCode_Ok;
}

OVERRIDE RpStatusCode_e
run_reference_update_callbacks(RpPrioritySetImpl* self, guint32 priority, RpHostVector hosts_added, RpHostVector hosts_removed)
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
    object_class->constructor = constructor;
    object_class->dispose = dispose;

    klass->create_host_set = create_host_set;
    klass->run_update_callbacks = run_update_callbacks;
    klass->run_reference_update_callbacks = run_reference_update_callbacks;
}

static void
rp_priority_set_impl_init(RpPrioritySetImpl* self G_GNUC_UNUSED)
{
    LOGD("(%p)", self);
}
