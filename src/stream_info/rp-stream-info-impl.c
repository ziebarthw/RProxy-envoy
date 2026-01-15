/*
 * rp-stream-info-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_stream_info_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_stream_info_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "stream_info/rp-filter-state-impl.h"
#include "stream_info/rp-stream-info-impl.h"

typedef struct _RpStreamInfoImplPrivate RpStreamInfoImplPrivate;
struct _RpStreamInfoImplPrivate {

    RpFilterStateImpl* m_filter_state;
    RpClusterInfoConstSharedPtr m_upstream_cluster_info;
    RpRoute* m_route;

    evhtp_headers_t* m_request_headers;

    RpUpstreamInfo* m_upstream_info;
    RpConnectionInfoProvider* m_downstream_connection_info_provider;
    RpFilterState* m_ancestor_filter_state;
    RpFilterStateLifeSpan_e m_life_span;

    RpDownstreamTiming m_downstream_timing;

    GHashTable* m_metadata;

    evhtp_res m_response_code;
    evhtp_proto m_protocol;

    guint32 m_response_flags;

    bool m_should_drain_connection : 1;
};

enum
{
    PROP_0, // Reserved.
    PROP_PROTOCOL,
    PROP_DOWNSTREAM_INFO_PROVIDER,
    PROP_ANCESTOR_FILTER_STATE,
    PROP_LIFE_SPAN,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void stream_info_iface_init(RpStreamInfoInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpStreamInfoImpl, rp_stream_info_impl, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpStreamInfoImpl)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_INFO, stream_info_iface_init)
)

#define PRIV(obj) \
    ((RpStreamInfoImplPrivate*)rp_stream_info_impl_get_instance_private(RP_STREAM_INFO_IMPL(obj)))

static inline GHashTable*
ensure_metadata(RpStreamInfoImplPrivate* me)
{
    NOISY_MSG_("(%p)", me);
    if (me->m_metadata)
    {
        NOISY_MSG_("pre-allocated metadata %p", me->m_metadata);
        return me->m_metadata;
    }
    me->m_metadata = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    NOISY_MSG_("allocated metadata %p", me->m_metadata);
    return me->m_metadata;
}

static GHashTable*
dynamic_metadata_i(RpStreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return ensure_metadata(PRIV(self));
}

static evhtp_proto
protocol_i(RpStreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_protocol;
}

static void
set_protocol_i(RpStreamInfo* self, evhtp_proto protocol)
{
    NOISY_MSG_("(%p, %d)", self, protocol);
    PRIV(self)->m_protocol = protocol;
}

static evhtp_headers_t*
get_request_headers_i(RpStreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_request_headers;
}

static void
set_request_headers_i(RpStreamInfo* self, evhtp_headers_t* request_headers)
{
    NOISY_MSG_("(%p, %p)", self, request_headers);
    PRIV(self)->m_request_headers = request_headers;
}

static void
set_response_flag_i(RpStreamInfo* self, RpCoreResponseFlag_e response_flag)
{
    NOISY_MSG_("(%p, %d)", self, response_flag);
    PRIV(self)->m_response_flags |= response_flag;
}

static RpDownstreamTiming*
downstream_timing_i(RpStreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return &PRIV(self)->m_downstream_timing;
}

static bool
should_drain_connection_upon_completion_i(RpStreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_should_drain_connection;
}

static void
set_should_drain_connection_upon_completion_i(RpStreamInfo* self, bool should_drain)
{
    NOISY_MSG_("(%p, %u)", self, should_drain);
    PRIV(self)->m_should_drain_connection = should_drain;
}

static void
set_upstream_info_i(RpStreamInfo* self, RpUpstreamInfo* info)
{
    NOISY_MSG_("(%p, %p)", self, info);
    PRIV(self)->m_upstream_info = info;
}

static RpUpstreamInfo*
upstream_info_i(RpStreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_upstream_info;
}

static RpClusterInfoConstSharedPtr
upstream_cluster_info_i(RpStreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_upstream_cluster_info;
}

static RpFilterState*
filter_state_i(RpStreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_FILTER_STATE(PRIV(self)->m_filter_state);
}

static void
set_response_code_i(RpStreamInfo* self, evhtp_res code)
{
    NOISY_MSG_("(%p, %d)", self, code);
    PRIV(self)->m_response_code = code;
}

static evhtp_res
response_code_i(RpStreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_response_code;
}

static RpRoute*
route_i(RpStreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
NOISY_MSG_("route %p", PRIV(self)->m_route);
    return PRIV(self)->m_route;
}

static void
set_dynamic_metadata_i(RpStreamInfo* self, const char* name, void* value)
{
    NOISY_MSG_("(%p, %p(%s), %p)", self, name, name, value);
    GHashTable* metadata = ensure_metadata(PRIV(self));
    if (!g_hash_table_insert(metadata, (gpointer)name, value))
    {
        LOGI("insert for key \"%s\" failed", name);
    }
}

static void
on_request_complete_i(RpStreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    //TODO...final_time_ = time_source_.monotonicTime();
}

static RpConnectionInfoProvider*
downstream_address_provider_i(RpStreamInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_downstream_connection_info_provider;
}

static void
set_upstream_cluster_info_i(RpStreamInfo* self, RpClusterInfoConstSharedPtr upstream_cluster_info)
{
    NOISY_MSG_("(%p, %p)", self, upstream_cluster_info);
    PRIV(self)->m_upstream_cluster_info = upstream_cluster_info;
}

static bool
has_response_flag_i(RpStreamInfo* self, RpCoreResponseFlag_e flag)
{
    NOISY_MSG_("(%p, %d)", self, flag);
    return (PRIV(self)->m_response_flags & flag);
}

static void
stream_info_iface_init(RpStreamInfoInterface* iface)
{
    LOGD("(%p)", iface);
    iface->protocol = protocol_i;
    iface->set_protocol = set_protocol_i;
    iface->get_request_headers = get_request_headers_i;
    iface->set_request_headers = set_request_headers_i;
    iface->set_response_flag = set_response_flag_i;
    iface->downstream_timing = downstream_timing_i;
    iface->should_drain_connection_upon_completion = should_drain_connection_upon_completion_i;
    iface->set_should_drain_connection_upon_completion = set_should_drain_connection_upon_completion_i;
    iface->set_upstream_info = set_upstream_info_i;
    iface->upstream_info = upstream_info_i;
    iface->upstream_cluster_info = upstream_cluster_info_i;
    iface->filter_state = filter_state_i;
    iface->set_response_code = set_response_code_i;
    iface->response_code = response_code_i;
    iface->route = route_i;
    iface->dynamic_metadata = dynamic_metadata_i;
    iface->set_dynamic_metadata = set_dynamic_metadata_i;
    iface->on_request_complete = on_request_complete_i;
    iface->downstream_address_provider = downstream_address_provider_i;
    iface->set_upstream_cluster_info = set_upstream_cluster_info_i;
    iface->has_response_flag = has_response_flag_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_PROTOCOL:
            g_value_set_int(value, PRIV(obj)->m_protocol);
            break;
        case PROP_DOWNSTREAM_INFO_PROVIDER:
            g_value_set_object(value, PRIV(obj)->m_downstream_connection_info_provider);
            break;
        case PROP_ANCESTOR_FILTER_STATE:
            g_value_set_object(value, PRIV(obj)->m_ancestor_filter_state);
            break;
        case PROP_LIFE_SPAN:
            g_value_set_int(value, PRIV(obj)->m_life_span);
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
        case PROP_PROTOCOL:
            PRIV(obj)->m_protocol = g_value_get_int(value);
            break;
        case PROP_DOWNSTREAM_INFO_PROVIDER:
            PRIV(obj)->m_downstream_connection_info_provider = g_value_get_object(value);
            break;
        case PROP_ANCESTOR_FILTER_STATE:
            PRIV(obj)->m_ancestor_filter_state = g_value_get_object(value);
            if (PRIV(obj)->m_ancestor_filter_state)
            {
                NOISY_MSG_("referencing ancestor filter state %p(%u)", PRIV(obj)->m_ancestor_filter_state, G_OBJECT(PRIV(obj)->m_ancestor_filter_state)->ref_count);
                g_object_ref(PRIV(obj)->m_ancestor_filter_state);
            }
            break;
        case PROP_LIFE_SPAN:
            PRIV(obj)->m_life_span = g_value_get_int(value);
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

    G_OBJECT_CLASS(rp_stream_info_impl_parent_class)->constructed(obj);

    RpStreamInfoImplPrivate* me = PRIV(obj);
    me->m_filter_state = rp_filter_state_impl_new(me->m_ancestor_filter_state, me->m_life_span);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpStreamInfoImplPrivate* me = PRIV(obj);
    g_clear_pointer(&me->m_metadata, g_hash_table_unref);
    g_clear_object(&me->m_ancestor_filter_state);
    g_clear_object(&me->m_filter_state);

    G_OBJECT_CLASS(rp_stream_info_impl_parent_class)->dispose(obj);
}

static void
rp_stream_info_impl_class_init(RpStreamInfoImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    obj_properties[PROP_PROTOCOL] = g_param_spec_int("protocol",
                                                    "Protocol",
                                                    "Protocol",
                                                    EVHTP_PROTO_INVALID,
                                                    EVHTP_PROTO_11,
                                                    EVHTP_PROTO_INVALID,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_DOWNSTREAM_INFO_PROVIDER] = g_param_spec_object("downstream-info-provider",
                                                    "Downstream info provider",
                                                    "Downstream ConnectionInfoProvider Instance",
                                                    RP_TYPE_CONNECTION_INFO_PROVIDER,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_ANCESTOR_FILTER_STATE] = g_param_spec_object("ancestor-filter-state",
                                                    "Ancestor filter state",
                                                    "Ancestor FilterState Instance",
                                                    RP_TYPE_FILTER_STATE,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_LIFE_SPAN] = g_param_spec_int("life-span",
                                                    "Life span",
                                                    "Life Span",
                                                    RpFilterStateLifeSpan_FilterChain,
                                                    RpFilterStateLifeSpan_TopSpan,
                                                    RpFilterStateLifeSpan_Connection,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_stream_info_impl_init(RpStreamInfoImpl* self)
{
    NOISY_MSG_("(%p)", self);

    RpStreamInfoImplPrivate* me = PRIV(self);
    me->m_protocol = EVHTP_PROTO_INVALID;
    me->m_request_headers = NULL;
    me->m_downstream_timing = rp_downstream_timing_ctor();
    me->m_should_drain_connection = false;
}

RpStreamInfoImpl*
rp_stream_info_impl_new(evhtp_proto protocol, RpConnectionInfoProvider* downstream_info_provider, RpFilterStateLifeSpan_e life_span, RpFilterState* ancestor_filter_state)
{
    LOGD("(%d, %p, %d, %p)", protocol, downstream_info_provider, life_span, ancestor_filter_state);
    return g_object_new(RP_TYPE_STREAM_INFO_IMPL,
                        "protocol", protocol,
                        "downstream-info-provider", downstream_info_provider,
                        "life-span", life_span,
                        "ancestor-filter-state", ancestor_filter_state,
                        NULL);
}

void
rp_stream_info_impl_set_route_(RpStreamInfoImpl* self, RpRoute* route)
{
    LOGD("(%p, %p)", self, route);
    g_return_if_fail(RP_IS_STREAM_INFO_IMPL(self));
    PRIV(self)->m_route = route;
}
