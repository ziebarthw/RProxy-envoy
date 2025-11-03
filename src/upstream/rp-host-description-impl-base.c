/*
 * rp-host-description-impl-base.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_host_description_impl_base_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_host_description_impl_base_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <stdio.h>
#include "rp-net-transport-socket.h"
#include "rp-upstream.h"
#include "upstream/rp-host-description-impl-base.h"

typedef struct _RpHostDescriptionImplBasePrivate RpHostDescriptionImplBasePrivate;
struct _RpHostDescriptionImplBasePrivate {

    RpClusterInfo* m_cluster;
    const char* m_hostname;
    struct sockaddr* m_dest_address;

    volatile gint m_priority;

    guint32 m_cx_active;
};

enum
{
    PROP_0, // Reserved.
    PROP_CLUSTER,
    PROP_HOSTNAME,
    PROP_DEST_ADDRESS,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void host_description_iface_init(RpHostDescriptionInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpHostDescriptionImplBase, rp_host_description_impl_base, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpHostDescriptionImplBase)
    G_IMPLEMENT_INTERFACE(RP_TYPE_HOST_DESCRIPTION, host_description_iface_init)
)

#define PRIV(obj) \
    ((RpHostDescriptionImplBasePrivate*) rp_host_description_impl_base_get_instance_private(RP_HOST_DESCRIPTION_IMPL_BASE(obj)))

static RpClusterInfo*
cluster_i(RpHostDescription* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_cluster;
}

static const char*
hostname_i(RpHostDescription* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_hostname;
}

static bool
can_create_connection_i(RpHostDescription* self, RpResourcePriority_e priority)
{
    NOISY_MSG_("(%p, %d)", self, priority);
    RpHostDescriptionImplBasePrivate* me = PRIV(self);
    RpResourceManager* resource_manager = rp_cluster_info_resource_manager(
                                            rp_host_description_cluster(self), priority);
    if (me->m_cx_active > rp_resource_manager_max_connections_pre_host(resource_manager))
    {
        return false;
    }
    return rp_resource_limit_can_create(rp_resource_manager_connections(resource_manager));
}

static guint32
priority_i(RpHostDescription* self)
{
    NOISY_MSG_("(%p)", self);
    return (guint32)g_atomic_int_get(&PRIV(self)->m_priority);
}

static void
set_priority_i(RpHostDescription* self, guint32 priority)
{
    NOISY_MSG_("(%p, %u)", self, priority);
    g_atomic_int_set(&PRIV(self)->m_priority, (int)priority);
}

static void
host_description_iface_init(RpHostDescriptionInterface* iface)
{
    LOGD("(%p)", iface);
    iface->cluster = cluster_i;
    iface->hostname = hostname_i;
    iface->can_create_connection = can_create_connection_i;
    iface->priority = priority_i;
    iface->set_priority = set_priority_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_CLUSTER:
            g_value_set_object(value, PRIV(obj)->m_cluster);
            break;
        case PROP_HOSTNAME:
            g_value_set_string(value, PRIV(obj)->m_hostname);
            break;
        case PROP_DEST_ADDRESS:
            g_value_set_pointer(value, PRIV(obj)->m_dest_address);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_CLUSTER:
            PRIV(obj)->m_cluster = g_value_get_object(value);
            break;
        case PROP_HOSTNAME:
            PRIV(obj)->m_hostname = g_value_get_string(value);
            break;
        case PROP_DEST_ADDRESS:
            PRIV(obj)->m_dest_address = g_value_get_pointer(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_host_description_impl_base_parent_class)->dispose(obj);
}

static void
rp_host_description_impl_base_class_init(RpHostDescriptionImplBaseClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_CLUSTER] = g_param_spec_object("cluster",
                                                    "Cluster",
                                                    "ClusterInfo Instance",
                                                    RP_TYPE_CLUSTER_INFO,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_DEST_ADDRESS] = g_param_spec_pointer("dest-address",
                                                    "Dest address",
                                                    "Destination Socket Address (sockaddr)",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_HOSTNAME] = g_param_spec_string("hostname",
                                                    "Hostname",
                                                    "Hostname",
                                                    NULL,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_host_description_impl_base_init(RpHostDescriptionImplBase* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

struct sockaddr*
rp_host_description_impl_base_dest_address_(RpHostDescriptionImplBase* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HOST_DESCRIPTION_IMPL_BASE(self), NULL);
    return PRIV(self)->m_dest_address;
}
