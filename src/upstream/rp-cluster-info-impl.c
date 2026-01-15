/*
 * rp-cluster-info-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_info_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_info_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-resource-manager-impl.h"
#include "upstream/rp-upstream-impl.h"

struct _RpClusterInfoImpl {
    GObject parent_instance;

    RpResourceManagerImpl* m_resource_manager;
    RpServerFactoryContext* m_server_context;
    char* m_name;
    RpClusterCfg m_config;

    guint64 m_max_requests_per_connection;

    bool m_added_via_api;
};

enum
{
    PROP_0, // Reserved.
    PROP_SERVER_CONTEXT,
    PROP_CONFIG,
    PROP_ADDED_VIA_API,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void filter_chain_factory_iface_init(RpFilterChainFactoryInterface* iface);
static void cluster_info_iface_init(RpClusterInfoInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpClusterInfoImpl, rp_cluster_info_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_FILTER_CHAIN_FACTORY, filter_chain_factory_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_CLUSTER_INFO, cluster_info_iface_init)
)

static bool
create_filter_chain_i(RpFilterChainFactory* self, RpFilterChainManager* manager)
{
    NOISY_MSG_("(%p, %p)", self, manager);
    //TODO...if (http_filter_factories_.empty())...
    return false;
}

static void
filter_chain_factory_iface_init(RpFilterChainFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_filter_chain = create_filter_chain_i;
}

static bool
added_via_api_i(RpClusterInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_INFO_IMPL(self)->m_added_via_api;
}

static const char*
name_i(RpClusterInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_INFO_IMPL(self)->m_name;
}

static RpResourceManager*
resource_manager_i(RpClusterInfo* self, RpResourcePriority_e priority)
{
    NOISY_MSG_("(%p, %d)", self, priority);
    return RP_RESOURCE_MANAGER(RP_CLUSTER_INFO_IMPL(self)->m_resource_manager);
}

static float
per_upstream_preconnect_ratio_i(RpClusterInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_INFO_IMPL(self)->m_config.preconnect_policy.per_upstream_preconnect_ratio;
}

static RpUpstreamLocalAddressSelector*
get_upstream_local_address_selector_i(RpClusterInfo* self)
{
    NOISY_MSG_("(%p)", self);
//TODO...
return NULL;
}

static guint64
max_requests_per_connection_i(RpClusterInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_INFO_IMPL(self)->m_max_requests_per_connection;
}

static RpDiscoveryType_e
type_i(RpClusterInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CLUSTER_INFO_IMPL(self)->m_config.type;
}

static RpTransportSocketFactoryContextPtr
transport_socket_context_i(RpClusterInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_server_factory_context_get_transport_socket_factory_context(
            RP_CLUSTER_INFO_IMPL(self)->m_server_context);
}

static evhtp_proto*
upstream_http_protocol_i(RpClusterInfo* self, evhtp_proto downstream_protocol)
{
    NOISY_MSG_("(%p, %d)", self, downstream_protocol);
    evhtp_proto* rval = g_malloc0(sizeof(*rval));
    //TODO...features_ & USE_DOWNSTREAM_PROTOCOL
    if (downstream_protocol != EVHTP_PROTO_INVALID)
    {
        if (downstream_protocol == EVHTP_PROTO_10)
        {
            *rval = EVHTP_PROTO_11;
            return rval;
        }
        *rval = downstream_protocol;
        return rval;
    }
    //TODO...HTTP2, HTTP3, etc.
    *rval = EVHTP_PROTO_11;
    return rval;
}

static void
cluster_info_iface_init(RpClusterInfoInterface* iface)
{
    LOGD("(%p)", iface);
    iface->added_via_api = added_via_api_i;
    iface->name = name_i;
    iface->resource_manager = resource_manager_i;
    iface->per_upstream_preconnect_ratio = per_upstream_preconnect_ratio_i;
    iface->get_upstream_local_address_selector = get_upstream_local_address_selector_i;
    iface->max_requests_per_connection = max_requests_per_connection_i;
    iface->type = type_i;
    iface->transport_socket_context = transport_socket_context_i;
    iface->upstream_http_protocol = upstream_http_protocol_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_SERVER_CONTEXT:
            g_value_set_object(value, RP_CLUSTER_INFO_IMPL(obj)->m_server_context);
            break;
        case PROP_CONFIG:
            g_value_set_pointer(value, &RP_CLUSTER_INFO_IMPL(obj)->m_config);
            break;
        case PROP_ADDED_VIA_API:
            g_value_set_boolean(value, RP_CLUSTER_INFO_IMPL(obj)->m_added_via_api);
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
        case PROP_SERVER_CONTEXT:
            RP_CLUSTER_INFO_IMPL(obj)->m_server_context = g_value_get_object(value);
            break;
        case PROP_CONFIG:
            RP_CLUSTER_INFO_IMPL(obj)->m_config = *((RpClusterCfg*)g_value_get_pointer(value));
            break;
        case PROP_ADDED_VIA_API:
            RP_CLUSTER_INFO_IMPL(obj)->m_added_via_api = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_cluster_info_impl_parent_class)->constructed(obj);

    RpClusterInfoImpl* self = RP_CLUSTER_INFO_IMPL(obj);
    self->m_name = g_strdup(self->m_config.name);
    self->m_resource_manager = rp_resource_manager_impl_new(self->m_name, 1024, 1024, 1024, 3, G_MAXUINT64, G_MAXUINT64);
self->m_max_requests_per_connection = 1024;//TODO...config...
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpClusterInfoImpl* self = RP_CLUSTER_INFO_IMPL(obj);
    g_clear_pointer(&self->m_name, g_free);
    g_clear_object(&self->m_resource_manager);

    G_OBJECT_CLASS(rp_cluster_info_impl_parent_class)->dispose(obj);
}

static void
rp_cluster_info_impl_class_init(RpClusterInfoImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    obj_properties[PROP_SERVER_CONTEXT] = g_param_spec_object("server-context",
                                                    "Server context",
                                                    "ServerFactoryContext Instance",
                                                    RP_TYPE_SERVER_FACTORY_CONTEXT,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_CONFIG] = g_param_spec_pointer("config",
                                                    "Config",
                                                    "ClusterCfg",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_ADDED_VIA_API] = g_param_spec_boolean("added-via-api",
                                                    "Added via api",
                                                    "Added Via API flag",
                                                    false,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_cluster_info_impl_init(RpClusterInfoImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpClusterInfoImpl*
rp_cluster_info_impl_new(RpServerFactoryContext* server_context, RpClusterCfg* config, bool added_via_api)
{
    LOGD("(%p, %p, %u)", server_context, config, added_via_api);
    g_return_val_if_fail(RP_IS_SERVER_FACTORY_CONTEXT(server_context), NULL);
    g_return_val_if_fail(config != NULL, NULL);
    return g_object_new(RP_TYPE_CLUSTER_INFO_IMPL,
                        "server-context", server_context,
                        "config", config,
                        "added-via-api", added_via_api,
                        NULL);
}
