/*
 * rp-stream-info.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_stream_info_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_stream_info_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-stream-info.h"

G_DEFINE_INTERFACE(RpUpstreamInfo, rp_upstream_info, G_TYPE_OBJECT)
static void
rp_upstream_info_default_init(RpUpstreamInfoInterface* iface)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpStreamInfo, rp_stream_info, G_TYPE_OBJECT)
static void
rp_stream_info_default_init(RpStreamInfoInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
