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

#include "upstream/rp-upstream-impl.h"

struct _RpMainPrioritySetImpl {
    RpPrioritySetImpl parent_instance;

    /*mutable*/ RpHostMapSharedPtr m_mutable_cross_priority_host_map;
};

static void priority_set_iface_init(RpPrioritySetInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpMainPrioritySetImpl, rp_main_priority_set_impl, RP_TYPE_PRIORITY_SET_IMPL,
    G_IMPLEMENT_INTERFACE(RP_TYPE_PRIORITY_SET, priority_set_iface_init)
)

#define PARENT_PRIORITY_SET_IFACE(s) \
    ((RpPrioritySetInterface*)g_type_interface_peek_parent(RP_PRIORITY_SET_GET_IFACE(s)))

static const RpHostSetPtrVector
host_sets_per_priority_i(RpPrioritySet* self)
{
    NOISY_MSG_("(%p)", self);
    return PARENT_PRIORITY_SET_IFACE(self)->host_sets_per_priority(self);
}

static RpHostMapConstSharedPtr
cross_priority_host_map_i(RpPrioritySet* self)
{
    NOISY_MSG_("(%p)", self);
    RpMainPrioritySetImpl* me = RP_MAIN_PRIORITY_SET_IMPL(self);
    RpHostMap** const_cross_priority_host_map_ = rp_priority_set_impl_const_cross_priority_host_map_(RP_PRIORITY_SET_IMPL(self));
    if (me->m_mutable_cross_priority_host_map != NULL)
    {
        *const_cross_priority_host_map_ = g_steal_pointer(&me->m_mutable_cross_priority_host_map);
    }
    return *const_cross_priority_host_map_;
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
    G_OBJECT_CLASS(rp_main_priority_set_impl_parent_class)->dispose(object);
}

static void
rp_main_priority_set_impl_class_init(RpMainPrioritySetImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_main_priority_set_impl_init(RpMainPrioritySetImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpMainPrioritySetImpl*
rp_main_priority_set_impl_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_MAIN_PRIORITY_SET_IMPL, NULL);
}
