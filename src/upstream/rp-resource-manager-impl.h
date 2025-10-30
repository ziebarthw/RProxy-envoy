/*
 * rp-resource-manager-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-resource-manager.h"

G_BEGIN_DECLS

/**
 * Implementation of ResourceManager.
 * NOTE: This implementation makes some assumptions which favor simplicity over correctness.
 * 1) Primarily, it assumes that traffic will be mostly balanced over all the worker threads since
 *    no attempt is made to balance resources between them. It is possible that starvation can
 *    occur during high contention.
 * 2) Though atomics are used, it is possible for resources to temporarily go above the supplied
 *    maximums. This should not effect overall behavior.
 */
#define RP_TYPE_RESOURCE_MANAGER_IMPL rp_resource_manager_impl_get_type()
G_DECLARE_FINAL_TYPE(RpResourceManagerImpl, rp_resource_manager_impl, RP, RESOURCE_MANAGER_IMPL, GObject)

RpResourceManagerImpl* rp_resource_manager_impl_new(const char* runtime_key,
                                                    guint64 max_connections,
                                                    guint64 max_pending_requests,
                                                    guint64 max_requests,
                                                    guint64 max_retries,
                                                    guint64 max_connection_pools,
                                                    guint64 max_connections_per_host);

G_END_DECLS
