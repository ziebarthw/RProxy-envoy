/*
 * rp-stream-info-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-router.h"
#include "rp-stream-info.h"

G_BEGIN_DECLS

#define RP_TYPE_STREAM_INFO_IMPL rp_stream_info_impl_get_type()
G_DECLARE_DERIVABLE_TYPE(RpStreamInfoImpl, rp_stream_info_impl, RP, STREAM_INFO_IMPL, GObject)

struct _RpStreamInfoImplClass {
    GObjectClass parent_class;

};

RpStreamInfoImpl* rp_stream_info_impl_new(evhtp_proto protocol,
                                            /*TODO...TimeSource& time_source,*/
                                            RpConnectionInfoProvider* downstream_info_provider,
                                            RpFilterStateLifeSpan_e life_span,
                                            RpFilterState* ancestor_filter_state);
void rp_stream_info_impl_set_route_(RpStreamInfoImpl* self, RpRoute* route);

G_END_DECLS
