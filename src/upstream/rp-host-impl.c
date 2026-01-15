/*
 * rp-host-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_host_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_host_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-net-client-conn-impl.h"
#include "rp-upstream.h"
#include "network/rp-default-client-conn-factory.h"
#include "upstream/rp-upstream-impl.h"

struct _RpHostImpl {
    RpHostDescriptionImpl parent_instance;

    gint m_initial_weight;

    guint32/*atomic*/ m_health_flags;
    guint32/*atomic*/ m_weight;
    guint32/*atomic*/ m_handle_count;

    bool m_disable_active_health_check : 1;
};

enum
{
    PROP_0, // Reserved.
    PROP_INITIAL_WEIGHT,
    PROP_DISABLE_HEALTH_CHECK,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void host_iface_init(RpHostInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpHostImpl, rp_host_impl, RP_TYPE_HOST_DESCRIPTION_IMPL,
    G_IMPLEMENT_INTERFACE(RP_TYPE_HOST, host_iface_init)
)

static RpCreateConnectionData
create_connection(RpHostImpl* self, RpDispatcher* dispatcher, RpClusterInfoConstSharedPtr cluster,
                    RpNetworkAddressInstanceConstSharedPtr address, RpUpstreamTransportSocketFactory* socket_factory,
                    RpHostDescription* host)
{
    NOISY_MSG_("(%p, %p, %p, %p, %p)", self, dispatcher, cluster, address, host);

//TODO...    RpUpstreamLocalAddressSelector* source_address_selector = rp_cluster_info_get_upstream_local_address_selector(cluster);
//TODO...    RpUpstreamLocalAddress upstream_local_address = rp_upstream_local_address_selector_get_upstream_local_address(source_address_selector, address);
    RpUpstreamLocalAddress upstream_local_address = rp_upstream_local_address_ctor(NULL);
    RpNetworkTransportSocket* transport_socket = rp_upstream_transport_socket_factory_create_transport_socket(socket_factory, host);
    RpNetworkClientConnectionPtr connection = rp_dispatcher_create_client_connection(dispatcher,
                                                                                        address,
                                                                                        upstream_local_address.m_address,
                                                                                        transport_socket);
    return rp_create_connection_data_ctor(connection, host);
}

static RpCreateConnectionData
create_connection_i(RpHost* self, RpDispatcher* dispatcher)
{
    NOISY_MSG_("(%p, %p)", self, dispatcher);

    RpHostDescription* host_description = RP_HOST_DESCRIPTION(self);
    RpUpstreamTransportSocketFactory* socket_factory = rp_host_description_transport_socket_factory(host_description);
    RpClusterInfoConstSharedPtr cluster = rp_host_description_cluster(host_description);
    RpNetworkAddressInstanceConstSharedPtr address = rp_host_description_address(host_description);
    return create_connection(RP_HOST_IMPL(self), dispatcher, cluster, address, socket_factory, host_description);
}

static void
host_iface_init(RpHostInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_connection = create_connection_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_INITIAL_WEIGHT:
            g_value_set_int(value, RP_HOST_IMPL(obj)->m_initial_weight);
            break;
        case PROP_DISABLE_HEALTH_CHECK:
            g_value_set_boolean(value, RP_HOST_IMPL(obj)->m_disable_active_health_check);
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
        case PROP_INITIAL_WEIGHT:
            RP_HOST_IMPL(obj)->m_initial_weight = g_value_get_int(value);
            break;
        case PROP_DISABLE_HEALTH_CHECK:
            RP_HOST_IMPL(obj)->m_disable_active_health_check = g_value_get_boolean(value);
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
    G_OBJECT_CLASS(rp_host_impl_parent_class)->dispose(obj);
}

static void
rp_host_impl_class_init(RpHostImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_INITIAL_WEIGHT] = g_param_spec_int("initial-weight",
                                                            "Initial weight",
                                                            "Initial Weight",
                                                            0,
                                                            G_MAXINT,
                                                            0,
                                                            G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_DISABLE_HEALTH_CHECK] = g_param_spec_boolean("disable-health-check",
                                                                        "Disable health check",
                                                                        "Disable Health Check",
                                                                        true,
                                                                        G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_host_impl_init(RpHostImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

#if 0
RpHostImpl*
rp_host_impl_new(RpClusterInfoConstSharedPtr cluster, RpUpstreamTransportSocketFactory* socket_factory, const char* hostname, struct sockaddr* address,
                    guint32 initial_weight, bool disable_health_check)
{
    LOGD("(%p, %p, %p(%s), %p, %u, %u)",
        cluster, socket_factory, hostname, hostname, address, initial_weight, disable_health_check);
    g_return_val_if_fail(RP_IS_CLUSTER_INFO(cluster), NULL);
    g_return_val_if_fail(address != NULL, NULL);
    return g_object_new(RP_TYPE_HOST_IMPL,
                        "cluster", cluster,
                        "socket-factory", socket_factory,
                        "hostname", hostname,
                        "dest-address", address,
                        "initial-weight", initial_weight,
                        "disable-health-check", disable_health_check,
                        NULL);
}
#endif//0

#if 0
HostImpl(absl::Status& creation_status, ClusterInfoConstSharedPtr cluster,
        const std::string& hostname, Network::Address::InstanceConstSharedPtr address,
        MetadataConstSharedPtr endpoint_metadata, MetadataConstSharedPtr locality_metadata,
        uint32_t initial_weight, const envoy::config::core::v3::Locality& locality,
        const envoy::config::endpoint::v3::Endpoint::HealthCheckConfig& health_check_config,
        uint32_t priority, const envoy::config::core::v3::HealthStatus health_status,
        TimeSource& time_source, const AddressVector& address_list = {})
    : HostImplBase(initial_weight, health_check_config, health_status),
    HostDescriptionImpl(creation_status, cluster, hostname, address, endpoint_metadata,
                        locality_metadata, locality, health_check_config, priority, time_source,
                        address_list) {}
#endif//0

static UNIQUE_PTR(RpHostImpl)
rp_host_impl_new(RpStatusCode_e* creation_status, RpClusterInfoConstSharedPtr cluster, const char* hostname,
                    RpNetworkAddressInstanceConstSharedPtr address, RpMetadataConstSharedPtr endpoint_metadata,
                    guint32 initial_weight, guint32 priority, RpTimeSource* time_source)
{
    NOISY_MSG_("(%p, %p, %p(%s), %p, %p, %u, %u, %p)",
        creation_status, cluster, hostname, hostname, address, endpoint_metadata, initial_weight, priority, time_source);

    g_return_val_if_fail(creation_status != NULL, NULL);
    g_return_val_if_fail(RP_IS_CLUSTER_INFO(cluster), NULL);
    g_return_val_if_fail(hostname != NULL, NULL);
    g_return_val_if_fail(address != NULL, NULL);
    g_return_val_if_fail(RP_IS_TIME_SOURCE(time_source), NULL);

    return g_object_new(RP_TYPE_HOST_IMPL,
                        "initial-weight", initial_weight,
                        "creation-status", creation_status,
                        "cluster", cluster,
                        "hostname", hostname,
                        "address", address,
                        "endpoint-metadata", endpoint_metadata,
                        "dest-address", address,
                        "priority", priority,
                        "time-source", time_source,
                        NULL);
}

#if 0
absl::StatusOr<std::unique_ptr<HostImpl>> HostImpl::create(
    ClusterInfoConstSharedPtr cluster, const std::string& hostname,
    Network::Address::InstanceConstSharedPtr address, MetadataConstSharedPtr endpoint_metadata,
    MetadataConstSharedPtr locality_metadata, uint32_t initial_weight,
    const envoy::config::core::v3::Locality& locality,
    const envoy::config::endpoint::v3::Endpoint::HealthCheckConfig& health_check_config,
    uint32_t priority, const envoy::config::core::v3::HealthStatus health_status,
    TimeSource& time_source, const AddressVector& address_list) {
  absl::Status creation_status = absl::OkStatus();
  auto ret = std::unique_ptr<HostImpl>(
      new HostImpl(creation_status, cluster, hostname, address, endpoint_metadata,
                   locality_metadata, initial_weight, locality, health_check_config, priority,
                   health_status, time_source, address_list));
  RETURN_IF_NOT_OK(creation_status);
  return ret;
}
#endif//0
UNIQUE_PTR(RpHostImpl)
rp_host_impl_create(RpClusterInfoConstSharedPtr cluster, const char* hostname,
                    RpNetworkAddressInstanceConstSharedPtr address, RpMetadataConstSharedPtr endpoint_metadata,
                    guint32 initial_weight, guint32 priority, RpTimeSource* time_source)
{
    LOGD("(%p, %p(%s), %p, %p, %u, %u, %p)",
        cluster, hostname, hostname, address, endpoint_metadata, initial_weight, priority, time_source);
    RpStatusCode_e creation_status = RpStatusCode_Ok;
    UNIQUE_PTR(RpHostImpl) ret = rp_host_impl_new(&creation_status,
                                                    cluster,
                                                    hostname,
                                                    address,
                                                    endpoint_metadata,
                                                    initial_weight,
                                                    priority,
                                                    time_source);
    if (creation_status != RpStatusCode_Ok)
    {
        LOGE("failed");
        g_clear_object(&ret);
    }
    return ret;
}