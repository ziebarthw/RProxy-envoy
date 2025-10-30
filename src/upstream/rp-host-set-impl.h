/*
 * rp-host-set-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-codec.h"
#include "rp-upstream.h"

G_BEGIN_DECLS

/**
 * A class for management of the set of hosts for a given priority level.
 */
#define RP_TYPE_HOST_SET_IMPL rp_host_set_impl_get_type()
G_DECLARE_DERIVABLE_TYPE(RpHostSetImpl, rp_host_set_impl, RP, HOST_SET_IMPL, GObject)

struct _RpHostSetImplClass {
    GObjectClass parent_class;

    RpStatusCode_e (*run_update_callbacks)(RpHostSetImpl*, RpHostVector, RpHostVector);
};

static inline RpStatusCode_e
rp_host_set_impl_run_update_callbacks(RpHostSetImpl* self, RpHostVector hosts_added, RpHostVector hosts_removed)
{
    return RP_IS_HOST_SET_IMPL(self) ?
        RP_HOST_SET_IMPL_GET_CLASS(self)->run_update_callbacks(self, hosts_added, hosts_removed) :
        RpStatusCode_Ok;
}

G_END_DECLS
