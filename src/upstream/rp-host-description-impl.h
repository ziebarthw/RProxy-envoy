/*
 * rp-host-description-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-upstream.h"
#include "upstream/rp-host-description-impl-base.h"

G_BEGIN_DECLS

/**
 * Final implementation of most of Upstream::HostDescription, providing const
 * of the address-related member variables.
 *
 * See also LogicalHostDescriptionImpl in
 * source/extensions/clusters/common/logical_host.h for a variant that allows
 * safe dynamic update to addresses.
 */
#define RP_TYPE_HOST_DESCRIPTION_IMPL rp_host_description_impl_get_type()
G_DECLARE_DERIVABLE_TYPE(RpHostDescriptionImpl, rp_host_description_impl, RP, HOST_DESCRIPTION_IMPL, RpHostDescriptionImplBase)

struct _RpHostDescriptionImplClass {
    RpHostDescriptionImplBaseClass parent_class;
    
};

G_END_DECLS
