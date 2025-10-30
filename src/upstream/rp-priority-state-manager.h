/*
 * rp-priority-state-manager.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-local-info.h"
#include "rp-upstream.h"
#include "rp-net-transport-socket.h"
#include "upstream/rp-cluster-impl-base.h"
#include "upstream/rp-host-impl.h"

G_BEGIN_DECLS

/**
 * Manages PriorityState of a cluster. PriorityState is a per-priority binding of a set of hosts
 * with its corresponding locality weight map. This is useful to store priorities/hosts/localities
 * before updating the cluster priority set.
 */
#define RP_TYPE_PRIORITY_STATE_MANAGER rp_priority_state_manager_get_type()
G_DECLARE_FINAL_TYPE(RpPriorityStateManager, rp_priority_state_manager, RP, PRIORITY_STATE_MANAGER, GObject)

RpPriorityStateManager* rp_priority_state_manager_new(RpClusterImplBase* cluster, RpLocalInfo* local_info/*, RpHostUpdateCb*/);
void rp_priority_state_manager_register_host_for_priority(RpPriorityStateManager* self,
                                                            RpUpstreamTransportSocketFactory* socket_factory,
                                                            const char* hostname,
                                                            struct sockaddr* address,
                                                            guint32 initial_weight,
                                                            bool disable_health_check,
                                                            guint32 priority,
                                                            GSList* address_list);

G_END_DECLS
