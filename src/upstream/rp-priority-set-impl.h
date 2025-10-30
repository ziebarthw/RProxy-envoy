/*
 * rp-priority-set-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "upstream/rp-host-set-impl.h"

G_BEGIN_DECLS

/**
 * A class for management of the set of hosts in a given cluster.
 */
#define RP_TYPE_PRIORITY_SET_IMPL rp_priority_set_impl_get_type()
G_DECLARE_DERIVABLE_TYPE(RpPrioritySetImpl, rp_priority_set_impl, RP, PRIORITY_SET_IMPL, GObject)

struct _RpPrioritySetImplClass {
    GObjectClass parent_class;

    RpHostSetImpl* (*create_host_set)(RpPrioritySetImpl*, guint32, bool, guint32);
    RpStatusCode_e (*run_update_callbacks)(RpPrioritySetImpl*, RpHostVector, RpHostVector);
    RpStatusCode_e (*run_reference_update_callbacks)(RpPrioritySetImpl*, guint32, RpHostVector, RpHostVector);
};

static inline RpHostSetImpl*
rp_priority_set_impl_create_host_set(RpPrioritySetImpl* self, guint32 priority, bool weighted_priority_health, guint32 overprovisioning_factor)
{
    return RP_IS_PRIORITY_SET_IMPL(self) ?
        RP_PRIORITY_SET_IMPL_GET_CLASS(self)->create_host_set(self, priority, weighted_priority_health, overprovisioning_factor) :
        NULL;
}
static inline RpStatusCode_e
rp_priority_set_impl_run_update_callbacks(RpPrioritySetImpl* self, RpHostVector hosts_added, RpHostVector hosts_removed)
{
    return RP_IS_PRIORITY_SET_IMPL(self) ?
        RP_PRIORITY_SET_IMPL_GET_CLASS(self)->run_update_callbacks(self, hosts_added, hosts_removed) :
        RpStatusCode_Ok;
}
static inline RpStatusCode_e rp_priority_set_impl_run_reference_update_callbacks(RpPrioritySetImpl* self, guint32 priority, RpHostVector hosts_added, RpHostVector hosts_removed)
{
    return RP_IS_PRIORITY_SET_IMPL(self) ?
        RP_PRIORITY_SET_IMPL_GET_CLASS(self)->run_reference_update_callbacks(self, priority, hosts_added, hosts_removed) :
        RpStatusCode_Ok;
}

G_END_DECLS
