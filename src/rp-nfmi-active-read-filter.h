/*
 * rp-nfmi-active-read-filter.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-net-filter.h"

G_BEGIN_DECLS

typedef struct _RpNetworkFilterManagerImpl RpNetworkFilterManagerImpl;

#define RP_TYPE_NFMI_ACTIVE_READ_FILTER rp_nfmi_active_read_filter_get_type()
G_DECLARE_FINAL_TYPE(RpNfmiActiveReadFilter, rp_nfmi_active_read_filter, RP, NFMI_ACTIVE_READ_FILTER, GObject)

RpNfmiActiveReadFilter* rp_nfmi_active_read_filter_new(RpNetworkFilterManagerImpl* parent,
                                                        RpNetworkReadFilter* filter);
bool rp_nfmi_active_read_filter_initialized(RpNfmiActiveReadFilter* self);
void rp_nfmi_active_read_filter_set_initialized(RpNfmiActiveReadFilter* self, bool v);
GSList* rp_nfmi_active_read_filter_get_entry(RpNfmiActiveReadFilter* self);
void rp_nfmi_active_read_filter_set_entry(RpNfmiActiveReadFilter* self,
                                            GSList* entry);
RpNetworkReadFilter* rp_nfmi_active_read_filter_get_filter(RpNfmiActiveReadFilter* self);

G_END_DECLS
