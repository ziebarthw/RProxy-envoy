/*
 * rp-host-description-impl-base.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_host_description_impl_base_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_host_description_impl_base_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <stdio.h>
#include "rp-net-transport-socket.h"
#include "rp-factory-context.h"
#include "upstream/rp-upstream-impl.h"

typedef struct _RpHostDescriptionImplBasePrivate RpHostDescriptionImplBasePrivate;
struct _RpHostDescriptionImplBasePrivate {

    RpClusterInfoConstSharedPtr m_cluster;
    RpUpstreamTransportSocketFactory* m_socket_factory;
    RpNetworkAddressInstanceConstSharedPtr m_dest_address;
    RpMetadataConstSharedPtr m_endpoint_metadata;
    const char* m_hostname;

    RpStatusCode_e* m_creation_status;

    RpTimeSource* m_time_source;
    RpMonotonicTime m_creation_time;

    guint32/*atomic*/ m_priority;

    guint32 m_cx_active;
};

enum
{
    PROP_0, // Reserved.
    PROP_CLUSTER,
    PROP_HOSTNAME,
    PROP_DEST_ADDRESS,
    PROP_PRIORITY,
    PROP_TIME_SOURCE,
    PROP_CREATION_STATUS,
    PROP_ENDPOINT_METADATA,
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

static RpClusterInfoConstSharedPtr
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
    return __atomic_load_n(&PRIV(self)->m_priority, __ATOMIC_ACQUIRE);
}

static void
set_priority_i(RpHostDescription* self, guint32 priority)
{
    NOISY_MSG_("(%p, %u)", self, priority);
    __atomic_store_n(&PRIV(self)->m_priority, priority, __ATOMIC_RELEASE);
}

static RpMetadataConstSharedPtr
metadata_i(RpHostDescription* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_endpoint_metadata;
}

static void
set_metadata_i(RpHostDescription* self, RpMetadataConstSharedPtr new_metadata)
{
    NOISY_MSG_("(%p)", self);
    PRIV(self)->m_endpoint_metadata = new_metadata;
}

static RpUpstreamTransportSocketFactory*
transport_socket_factory_i(RpHostDescription* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_socket_factory;
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
    iface->metadata = metadata_i;
    iface->set_metadata = set_metadata_i;
    iface->transport_socket_factory = transport_socket_factory_i;
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
        case PROP_PRIORITY:
            g_value_set_uint(value, PRIV(obj)->m_priority);
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
            PRIV(obj)->m_dest_address = g_object_ref(g_value_get_object(value));
            break;
        case PROP_PRIORITY:
            PRIV(obj)->m_priority = g_value_get_uint(value);
            break;
        case PROP_CREATION_STATUS:
            PRIV(obj)->m_creation_status = g_value_get_pointer(value);
            break;
        case PROP_TIME_SOURCE:
            PRIV(obj)->m_time_source = g_value_get_object(value);
            break;
        case PROP_ENDPOINT_METADATA:
            PRIV(obj)->m_endpoint_metadata = g_value_get_pointer(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static RpUpstreamTransportSocketFactory*
resolve_transport_socket_factory(RpHostDescriptionImplBase* self, RpNetworkAddressInstanceConstSharedPtr dest_address, RpMetadataConstSharedPtr endpoint_metadata)
{
    extern RpUpstreamTransportSocketConfigFactory* default_upstream_transport_socket_config_factory;
    NOISY_MSG_("(%p, %p, %p)", self, dest_address, endpoint_metadata);
    RpTransportSocketFactoryContextPtr context = rp_cluster_info_transport_socket_context(PRIV(self)->m_cluster);
    return rp_upstream_transport_socket_config_factory_create_transport_socket_factory(default_upstream_transport_socket_config_factory,
                                                                                        endpoint_metadata,
                                                                                        context);
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_host_description_impl_base_parent_class)->constructed(obj);

    RpHostDescriptionImplBasePrivate* me = PRIV(obj);
    me->m_socket_factory = resolve_transport_socket_factory(RP_HOST_DESCRIPTION_IMPL_BASE(obj),
                                                            me->m_dest_address,
                                                            me->m_endpoint_metadata);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpHostDescriptionImplBasePrivate* me = PRIV(obj);
    g_clear_object(&me->m_dest_address);

    G_OBJECT_CLASS(rp_host_description_impl_base_parent_class)->dispose(obj);
}

static void
rp_host_description_impl_base_class_init(RpHostDescriptionImplBaseClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    obj_properties[PROP_CLUSTER] = g_param_spec_object("cluster",
                                                        "Cluster",
                                                        "ClusterInfo Instance",
                                                        RP_TYPE_CLUSTER_INFO,
                                                        G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_DEST_ADDRESS] = g_param_spec_object("dest-address",
                                                            "Dest address",
                                                            "Destination Socket Address (sockaddr)",
                                                            RP_TYPE_NETWORK_ADDRESS_INSTANCE,
                                                            G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_HOSTNAME] = g_param_spec_string("hostname",
                                                        "Hostname",
                                                        "Hostname",
                                                        NULL,
                                                        G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_PRIORITY] = g_param_spec_uint("priority",
                                                        "Priority",
                                                        "Priority",
                                                        0,
                                                        G_MAXUINT32,
                                                        0,
                                                        G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_TIME_SOURCE] = g_param_spec_object("time-source",
                                                            "Time source",
                                                            "TimeSource Instance",
                                                            RP_TYPE_TIME_SOURCE,
                                                            G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_CREATION_STATUS] = g_param_spec_pointer("creation-status",
                                                                "Creation status",
                                                                "Creation status",
                                                                G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_ENDPOINT_METADATA] = g_param_spec_pointer("endpoint-metadata",
                                                                    "Endpoint metadata",
                                                                    "Endpoint metadata",
                                                                    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_host_description_impl_base_init(RpHostDescriptionImplBase* self)
{
    NOISY_MSG_("(%p)", self);
    RpHostDescriptionImplBasePrivate* me = PRIV(self);
    me->m_creation_time = rp_time_source_monotonic_time(me->m_time_source);
}

RpNetworkAddressInstanceConstSharedPtr
rp_host_description_impl_base_dest_address_(RpHostDescriptionImplBase* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HOST_DESCRIPTION_IMPL_BASE(self), NULL);
    return PRIV(self)->m_dest_address;
}
