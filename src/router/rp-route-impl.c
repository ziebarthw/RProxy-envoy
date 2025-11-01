/*
 * rp-route-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_route_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_route_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-headers.h"
#include "rp-route-configuration.h"
#include "rp-state-filter.h"
#include "router/rp-route-impl.h"

struct _RpRouteImpl {
    GObject parent_instance;

    SHARED_PTR(RpRouteConfig) m_parent;
    SHARED_PTR(rule_cfg_t) m_rule_cfg;

    UNIQUE_PTR(char) m_cluster_name;
};

static void route_iface_init(RpRouteInterface* iface);
static void response_entry_iface_init(RpResponseEntryInterface* iface);
static void route_entry_iface_init(RpRouteEntryInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpRouteImpl, rp_route_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_ROUTE, route_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_RESPONSE_ENTRY, response_entry_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_ROUTE_ENTRY, route_entry_iface_init)
)

static inline char*
ensure_cluster_name(RpRouteImpl* self)
{
    NOISY_MSG_("(%p)", self);
    if (self->m_cluster_name)
    {
        NOISY_MSG_("pre-allocated cluster name %p", self->m_cluster_name);
        return self->m_cluster_name;
    }
    self->m_cluster_name = g_strdup_printf("%p", self->m_rule_cfg);
    NOISY_MSG_("allocated cluster name %p", self->m_cluster_name);
    return self->m_cluster_name;
}

static const char*
route_name_i(RpRoute* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_ROUTE_IMPL(self)->m_rule_cfg->name;
}

static RpDirectResponseEntry*
direct_response_entry_i(RpRoute* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return NULL;
}

static RpRouteEntry*
route_entry_i(RpRoute* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_ROUTE_ENTRY(self);
}

static void
route_iface_init(RpRouteInterface* iface)
{
    LOGD("(%p)", iface);
    iface->route_name = route_name_i;
    iface->direct_response_entry = direct_response_entry_i;
    iface->route_entry = route_entry_i;
}

static void
finalize_response_headers_i(RpResponseEntry* self G_GNUC_UNUSED, evhtp_headers_t* response_headers,
                            RpStreamInfo* stream_info G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p, %p)", self, response_headers, stream_info);
    evhtp_kv_rm_and_free(response_headers,
        evhtp_headers_find_header(response_headers, "alt-svc"));
}

static void
response_entry_iface_init(RpResponseEntryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->finalize_response_headers = finalize_response_headers_i;
}

static bool
append_xfh_i(RpRouteEntry* self)
{
    NOISY_MSG_("(%p)", self);
    //TODO...
    return false;
}

static const char*
cluster_name_i(RpRouteEntry* self)
{
    NOISY_MSG_("(%p)", self);
    return ensure_cluster_name(RP_ROUTE_IMPL(self));
}

static char*
current_url_path_after_rewrite_i(RpRouteEntry* self G_GNUC_UNUSED, evhtp_headers_t* request_headers)
{
    NOISY_MSG_("(%p, %p)", self, request_headers);
    //TODO...
    return g_strdup(evhtp_header_find(request_headers, RpHeaderValues.Path));
}

static char*
get_request_host_value_i(RpRouteEntry* self G_GNUC_UNUSED, evhtp_headers_t* request_headers)
{
    NOISY_MSG_("(%p, %p)", self, request_headers);
    //TODO...host_rewrite_ logic...
    return g_strdup(evhtp_header_find(request_headers, RpHeaderValues.HostLegacy));
}

static RpResourcePriority_e
priority_i(RpRouteEntry* self)
{
    NOISY_MSG_("(%p)", self);
    //TODO...get the actual priority from the RouteConfiguration...
    return RpResourcePriority_Default;
}

static void
finalize_request_headers_i(RpRouteEntry* self, evhtp_headers_t* request_headers, RpStreamInfo* stream_info, bool insert_rproxy_original_path)
{
    NOISY_MSG_("(%p, %p, %p, %u)", self, request_headers, stream_info, insert_rproxy_original_path);
}

static void
route_entry_iface_init(RpRouteEntryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->append_xfh = append_xfh_i;
    iface->cluster_name = cluster_name_i;
    iface->current_url_path_after_rewrite = current_url_path_after_rewrite_i;
    iface->get_request_host_value = get_request_host_value_i;
    iface->priority = priority_i;
    iface->finalize_request_headers = finalize_request_headers_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpRouteImpl* self = RP_ROUTE_IMPL(obj);
    g_clear_pointer(&self->m_cluster_name, g_free);

    G_OBJECT_CLASS(rp_route_impl_parent_class)->dispose(obj);
}

static void
rp_route_impl_class_init(RpRouteImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_route_impl_init(RpRouteImpl* self G_GNUC_UNUSED)
{
    LOGD("(%p)", self);
}

RpRouteImpl*
rp_route_impl_new(SHARED_PTR(RpRouteConfig) parent, SHARED_PTR(rule_cfg_t) rule_cfg)
{
    LOGD("(%p, %p)", parent, rule_cfg);
    g_return_val_if_fail(RP_IS_ROUTE_CONFIG(parent), NULL);
    g_return_val_if_fail(rule_cfg != NULL, NULL);
    RpRouteImpl* self = g_object_new(RP_TYPE_ROUTE_IMPL, NULL);
    self->m_parent = parent;
    self->m_rule_cfg = rule_cfg;
    return self;
}

SHARED_PTR(rule_cfg_t)
rp_route_impl_get_rule_cfg(RpRouteImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_ROUTE_IMPL(self), NULL);
    return self->m_rule_cfg;
}
