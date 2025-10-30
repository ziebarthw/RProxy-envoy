/*
 * rp-priority-state-manager.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_priority_state_manager_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_priority_state_manager_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-priority-state-manager.h"

struct _RpPriorityStateManager {

    RpClusterImplBase* m_parent;
    RpLocalInfo* m_local_info;
    GArray* m_priority_state;
};

enum
{
    PROP_0, // Reserved.
    PROP_PARENT,
    PROP_LOCAL_INFO,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_FINAL_TYPE(RpPriorityStateManager, rp_priority_state_manager, G_TYPE_OBJECT)

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    LOGD("(%p, %u, %p, %p)", obj, prop_id, value, pspec);
    switch (prop_id)
    {
        case PROP_PARENT:
            g_value_set_object(value, RP_PRIORITY_STATE_MANAGER(obj)->m_parent);
            break;
        case PROP_LOCAL_INFO:
            g_value_set_object(value, RP_PRIORITY_STATE_MANAGER(obj)->m_local_info);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    LOGD("(%p, %u, %p, %p)", obj, prop_id, value, pspec);
    switch (prop_id)
    {
        case PROP_PARENT:
            RP_PRIORITY_STATE_MANAGER(obj)->m_parent = g_value_get_object(value);
            break;
        case PROP_LOCAL_INFO:
            RP_PRIORITY_STATE_MANAGER(obj)->m_local_info = g_value_get_object(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
dispose(GObject* object)
{
    LOGD("(%p)", object);
    G_OBJECT_CLASS(rp_priority_state_manager_parent_class)->dispose(object);
}

static void
rp_priority_state_manager_class_init(RpPriorityStateManagerClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_PARENT] = g_param_spec_object("cluster",
                                                    "Cluster",
                                                    "Parent ClusterImplBase Instance",
                                                    RP_TYPE_CLUSTER_IMPL_BASE,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_LOCAL_INFO] = g_param_spec_object("local-info",
                                                    "Local info",
                                                    "Local Info Instance",
                                                    RP_TYPE_LOCAL_INFO,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_priority_state_manager_init(RpPriorityStateManager* self G_GNUC_UNUSED)
{
    LOGD("(%p)", self);
}

RpPriorityStateManager*
rp_priority_state_manager_new(RpClusterImplBase* cluster, RpLocalInfo* local_info/*, RpHostUpdateCb*/)
{
    LOGD("(%p, %p)", cluster, local_info);
    g_return_val_if_fail(RP_IS_CLUSTER_IMPL_BASE(cluster), NULL);
    g_return_val_if_fail(RP_IS_LOCAL_INFO(local_info), NULL);
    return g_object_new(RP_TYPE_PRIORITY_STATE_MANAGER,
                        "cluster", cluster,
                        "local-info", local_info,
                        NULL);
}

static inline GArray*
ensure_priority_state(RpPriorityStateManager* self)
{
    NOISY_MSG_("(%p)", self);
    if (self->m_priority_state)
    {
        NOISY_MSG_("pre-allocated priority state %p", self->m_priority_state);
        return self->m_priority_state;
    }
    self->m_priority_state = g_array_sized_new(false, true, sizeof(GSList*), 128); //REVISIT - max priority value.
    NOISY_MSG_("allocated priority state %p", self->m_priority_state);
    return self->m_priority_state;
}

void
rp_priority_state_manager_register_host_for_priority(RpPriorityStateManager* self,
                                                        RpUpstreamTransportSocketFactory* socket_factory,
                                                        const char* hostname,
                                                        struct sockaddr* address,
                                                        guint32 initial_weight,
                                                        bool disable_health_check,
                                                        guint32 priority,
                                                        GSList* address_list)
{
    LOGD("(%p, %p, %p(%s), %p, %u, %u, %u, %p)",
        self, socket_factory, hostname, hostname, address, initial_weight, disable_health_check, priority, address_list);
    g_return_if_fail(RP_IS_PRIORITY_STATE_MANAGER(self));
    g_return_if_fail(address != NULL);
    RpClusterImplBase* parent = self->m_parent;
    RpClusterInfo* info = rp_cluster_info(RP_CLUSTER(parent));
    RpHostImpl* host = rp_host_impl_new(info, socket_factory, hostname, address, initial_weight, disable_health_check, priority, address_list);
    GArray* priority_state = ensure_priority_state(self);
    GSList* hosts = g_array_index(priority_state, GSList*, priority);

    hosts = g_slist_append(hosts, host);
    self->m_priority_state = g_array_insert_val(priority_state, priority, hosts);
}
