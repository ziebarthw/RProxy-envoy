/*
 * rp-nfmi-active-write-filter.h
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

#define RP_TYPE_NFMI_ACTIVE_WRITE_FILTER rp_nfmi_active_write_filter_get_type()
G_DECLARE_FINAL_TYPE(RpNfmiActiveWriteFilter, rp_nfmi_active_write_filter, RP, NFMI_ACTIVE_WRITE_FILTER, GObject)

RpNfmiActiveWriteFilter* rp_nfmi_active_write_filter_new(RpNetworkFilterManagerImpl* parent,
                                                        RpNetworkWriteFilter* filter);
GSList* rp_nfmi_active_write_filter_get_entry(RpNfmiActiveWriteFilter* self);
void rp_nfmi_active_write_filter_set_entry(RpNfmiActiveWriteFilter* self,
                                            GSList* entry);
RpNetworkWriteFilter* rp_nfmi_active_write_filter_get_filter(RpNfmiActiveWriteFilter* self);

G_END_DECLS
