/*
 * rp-resource-manager-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_resource_manager_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_resource_manager_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include <stdio.h>
#include <inttypes.h>
#include "upstream/rp-managed-resource-impl.h"
#include "upstream/rp-resource-manager-impl.h"

struct _RpResourceManagerImpl {
    GObject parent_instance;

    RpManagedResourceImpl* m_connections;
    RpManagedResourceImpl* m_pending_requests;
    RpManagedResourceImpl* m_requests;
    RpManagedResourceImpl* m_connection_pools;

    const char* m_runtime_key;
    guint64 m_max_connections;
    guint64 m_max_pending_requests;
    guint64 m_max_requests;
    guint64 m_max_retries;
    guint64 m_max_connection_pools;
    guint64 m_max_connections_per_host;
};

enum
{
    PROP_0, // Reserved.
    PROP_RUNTIME_KEY,
    PROP_MAX_CONNECTIONS,
    PROP_MAX_PENDING_REQUESTS,
    PROP_MAX_REQUESTS,
    PROP_MAX_RETRIES,
    PROP_MAX_CONNECTION_POOLS,
    PROP_MAX_CONNECTIONS_PER_HOST,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void resource_manager_iface_init(RpResourceManagerInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpResourceManagerImpl, rp_resource_manager_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_RESOURCE_MANAGER, resource_manager_iface_init)
)

static RpResourceLimit*
connection_pools_i(RpResourceManager* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_RESOURCE_LIMIT(RP_RESOURCE_MANAGER_IMPL(self)->m_connection_pools);
}

static RpResourceLimit*
connections_i(RpResourceManager* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_RESOURCE_LIMIT(RP_RESOURCE_MANAGER_IMPL(self)->m_connection_pools);
}

static RpResourceLimit*
pending_requests_i(RpResourceManager* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_RESOURCE_LIMIT(RP_RESOURCE_MANAGER_IMPL(self)->m_connection_pools);
}

static RpResourceLimit*
requests_i(RpResourceManager* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_RESOURCE_LIMIT(RP_RESOURCE_MANAGER_IMPL(self)->m_connection_pools);
}

static guint64
max_connections_pre_host_i(RpResourceManager* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_RESOURCE_MANAGER_IMPL(self)->m_max_connections_per_host;
}

static void
resource_manager_iface_init(RpResourceManagerInterface* iface)
{
    LOGD("(%p)", iface);
    iface->connection_pools = connection_pools_i;
    iface->connections = connections_i;
    iface->pending_requests = pending_requests_i;
    iface->requests = requests_i;
    iface->max_connections_pre_host = max_connections_pre_host_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_RUNTIME_KEY:
            g_value_set_string(value, RP_RESOURCE_MANAGER_IMPL(obj)->m_runtime_key);
            break;
        case PROP_MAX_CONNECTION_POOLS:
            g_value_set_uint64(value, RP_RESOURCE_MANAGER_IMPL(obj)->m_max_connection_pools);
            break;
        case PROP_MAX_CONNECTIONS:
            g_value_set_uint64(value, RP_RESOURCE_MANAGER_IMPL(obj)->m_max_connections);
            break;
        case PROP_MAX_CONNECTIONS_PER_HOST:
            g_value_set_uint64(value, RP_RESOURCE_MANAGER_IMPL(obj)->m_max_connections_per_host);
            break;
        case PROP_MAX_PENDING_REQUESTS:
            g_value_set_uint64(value, RP_RESOURCE_MANAGER_IMPL(obj)->m_max_pending_requests);
            break;
        case PROP_MAX_REQUESTS:
            g_value_set_uint64(value, RP_RESOURCE_MANAGER_IMPL(obj)->m_max_requests);
            break;
        case PROP_MAX_RETRIES:
            g_value_set_uint64(value, RP_RESOURCE_MANAGER_IMPL(obj)->m_max_retries);
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
        case PROP_RUNTIME_KEY:
            RP_RESOURCE_MANAGER_IMPL(obj)->m_runtime_key = g_value_get_string(value);
            break;
        case PROP_MAX_CONNECTION_POOLS:
            RP_RESOURCE_MANAGER_IMPL(obj)->m_max_connection_pools = g_value_get_uint64(value);
            break;
        case PROP_MAX_CONNECTIONS:
            RP_RESOURCE_MANAGER_IMPL(obj)->m_max_connections = g_value_get_uint64(value);
            break;
        case PROP_MAX_CONNECTIONS_PER_HOST:
            RP_RESOURCE_MANAGER_IMPL(obj)->m_max_connections_per_host = g_value_get_uint64(value);
            break;
        case PROP_MAX_PENDING_REQUESTS:
            RP_RESOURCE_MANAGER_IMPL(obj)->m_max_pending_requests = g_value_get_uint64(value);
            break;
        case PROP_MAX_REQUESTS:
            RP_RESOURCE_MANAGER_IMPL(obj)->m_max_requests = g_value_get_uint64(value);
            break;
        case PROP_MAX_RETRIES:
            RP_RESOURCE_MANAGER_IMPL(obj)->m_max_retries = g_value_get_uint64(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static inline RpManagedResourceImpl*
managed_resource_new(guint64 max, const char* runtime_key, const char* suffix)
{
    NOISY_MSG_("(%zu, %p(%s))", max, runtime_key, runtime_key);
    g_autofree gchar* key = g_strconcat(runtime_key, suffix, NULL);
    return rp_manage_resource_impl_new(max, key);
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_resource_manager_impl_parent_class)->constructed(obj);

    RpResourceManagerImpl* self = RP_RESOURCE_MANAGER_IMPL(obj);
    const char* runtime_key = self->m_runtime_key;
    self->m_connections = managed_resource_new(self->m_max_connections, runtime_key, "max_connections");
    self->m_pending_requests = managed_resource_new(self->m_max_pending_requests, runtime_key, "max_pending_requests");
    self->m_requests = managed_resource_new(self->m_max_requests, runtime_key, "max_requests");
    self->m_connection_pools = managed_resource_new(self->m_max_connection_pools, runtime_key, "max_connection_pools");
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpResourceManagerImpl* self = RP_RESOURCE_MANAGER_IMPL(obj);
    g_clear_object(&self->m_connection_pools);
    g_clear_object(&self->m_connections);
    g_clear_object(&self->m_pending_requests);
    g_clear_object(&self->m_requests);

    G_OBJECT_CLASS(rp_resource_manager_impl_parent_class)->dispose(obj);
}

static void
rp_resource_manager_impl_class_init(RpResourceManagerImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    obj_properties[PROP_RUNTIME_KEY] = g_param_spec_string("runtime-key",
                                                    "Runtime key",
                                                    "Runtime Key",
                                                    "",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_MAX_CONNECTION_POOLS] = g_param_spec_uint64("max-connection-pools",
                                                    "Max connection pools",
                                                    "Max Connection Pools",
                                                    0,
                                                    G_MAXUINT64,
                                                    G_MAXUINT64,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_MAX_CONNECTIONS] = g_param_spec_uint64("max-connections",
                                                    "Max connections",
                                                    "Max Connections",
                                                    0,
                                                    G_MAXUINT64,
                                                    G_MAXUINT64,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_MAX_CONNECTIONS_PER_HOST] = g_param_spec_uint64("max-connections-per-host",
                                                    "Max connections per host",
                                                    "Max Connections Per Host",
                                                    0,
                                                    G_MAXUINT64,
                                                    G_MAXUINT64,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_MAX_PENDING_REQUESTS] = g_param_spec_uint64("max-pending-requests",
                                                    "Max pending requests",
                                                    "Max Pending Requests",
                                                    0,
                                                    G_MAXUINT64,
                                                    G_MAXUINT64,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_MAX_REQUESTS] = g_param_spec_uint64("max-requests",
                                                    "Max requests",
                                                    "Max Requests",
                                                    0,
                                                    G_MAXUINT64,
                                                    G_MAXUINT64,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_MAX_RETRIES] = g_param_spec_uint64("max-retries",
                                                    "Max retries",
                                                    "Max Retries",
                                                    0,
                                                    G_MAXUINT64,
                                                    G_MAXUINT64,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_resource_manager_impl_init(RpResourceManagerImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpResourceManagerImpl*
rp_resource_manager_impl_new(const char* runtime_key, guint64 max_connections, guint64 max_pending_requests,
                                guint64 max_requests, guint64 max_retries, guint64 max_connection_pools,
                                guint64 max_connections_per_host)
{
    LOGD("(%p(%s), %lu, %lu, %lu, %lu, %lu, %lu)",
        runtime_key, runtime_key, max_connections, max_pending_requests, max_requests, max_retries,
        max_connection_pools, max_connections_per_host);
    return g_object_new(RP_TYPE_RESOURCE_MANAGER_IMPL,
                        "runtime-key", runtime_key,
                        "max-connections", max_connections,
                        "max-pending-requests", max_pending_requests,
                        "max-requests", max_requests,
                        "max-retries", max_retries,
                        "max-connection-pools", max_connection_pools,
                        "max-connections-per-host", max_connections_per_host,
                        NULL);
}
