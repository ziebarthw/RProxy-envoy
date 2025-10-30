/*
 * rp-net-filter-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-net-filter.h"

G_BEGIN_DECLS

/**
 * Implementation of Network::ReadFilter that discards read callbacks.
 */
#define RP_TYPE_NETWORK_READ_FILTER_BASE_IMPL rp_network_read_filter_base_impl_get_type()
G_DECLARE_DERIVABLE_TYPE(RpNetworkReadFilterBaseImpl, rp_network_read_filter_base_impl, RP, NETWORK_READ_FILTER_BASE_IMPL, GObject)

struct _RpNetworkReadFilterBaseImplClass {
    GObjectClass parent_class;

};

G_END_DECLS
