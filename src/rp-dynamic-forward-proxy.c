/*
 * rp-dynamic-forward-proxy.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_dynamic_forward_proxy_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_dynamic_forward_proxy_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <netinet/in.h>
#include "event/rp-dispatcher-impl.h"
#include "network/rp-transport-socket-factory-impl.h"
#include "upstream/rp-host-impl.h"
#include "rp-dfp-cluster-factory.h"
#include "rp-dfp-cluster-impl.h"
#include "rp-dfp-cluster-store.h"
#include "rp-filter-chain-factory-callbacks-impl.h"
#include "rp-headers.h"
#include "rp-http-conn-manager-impl.h"
#include "rp-http-conn-mgr-impl-active-stream.h"
#include "rp-state-filter.h"
#include "rp-static-cluster-impl.h"
#include "rp-http-utility.h"
#include "rp-dynamic-forward-proxy.h"

#define DECODER_CALLBACKS(s) \
    rp_pass_through_filter_decoder_callbacks_(RP_PASS_THROUGH_FILTER(s))
#define STREAM_FILTER_CALLBACKS(s) \
    RP_STREAM_FILTER_CALLBACKS(DECODER_CALLBACKS(s))
#define STREAM_INFO(s) \
    rp_stream_filter_callbacks_stream_info(STREAM_FILTER_CALLBACKS(s))
#define FILTER_STATE(s) rp_stream_info_filter_state(STREAM_INFO(s))
#define PARENT_STREAM_DECODER_FILTER_IFACE(s) \
    ((RpStreamDecoderFilterInterface*)g_type_interface_peek_parent(RP_STREAM_DECODER_FILTER_GET_IFACE(s)))
#define DISPATCHER(c) rp_filter_chain_factory_callbacks_dispatcher(c)
#define THR(c) rp_dispatcher_thr(RP_DISPATCHER_IMPL(DISPATCHER(c)))
#define RPROXY(c) ((rproxy_t*)evthr_get_aux(THR(c)))
#define CLUSTER_MANAGER(c) RPROXY(c)->m_cluster_manager
#define RP_DYNAMIC_FORWARD_PROXY_CB(s) (RpDynamicForwardProxyCb*)s

typedef struct _RpDynamicForwardProxyCb RpDynamicForwardProxyCb;
struct _RpDynamicForwardProxyCb {
    RpFilterFactoryCb parent_instance;
    RpDynamicForwardProxyCfg m_config;
};

struct _RpDynamicForwardProxy {
    RpPassThroughFilter parent_instance;

    RpDynamicForwardProxyCfg* m_config;
    RpClusterInfo* m_cluster_info;
    gpointer m_cluster_store; // Envoy: Extensions::Common::DynamicForwardProxy::DFPClusterStoreSharedPtr cluster_store_
};

static inline void stream_decoder_filter_iface_init(RpStreamDecoderFilterInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpDynamicForwardProxy, rp_dynamic_forward_proxy, RP_TYPE_PASS_THROUGH_FILTER,
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_DECODER_FILTER, stream_decoder_filter_iface_init)
)

static inline gchar*
create_cluster_name(const char* host, int port)
{
    NOISY_MSG_("(%p(%s), %d)", host, host, port);
    return g_strdup_printf("DFPCluster:%s:%u", host, port);
}

static bool
add_dynamic_cluster(RpDynamicForwardProxy* self, RpDfpCluster* cluster, const char* cluster_name, const char* host, int port/*, TODO...LoadCluster...*/)
{
    NOISY_MSG_("(%p, %p, %p(%s), %p(%s), %d)",
        self, cluster, cluster_name, cluster_name, host, host, port);
    RpClusterCfg sub_cluster = rp_dfp_cluster_create_sub_cluster_config(cluster, cluster_name, host, port);
    g_autoptr(RpDfpClusterFactory) factory = rp_dfp_cluster_factory_new();
    sub_cluster.factory = RP_CLUSTER_FACTORY(factory);

//TODO...this is more complicated than necessary

    if (rp_cluster_manager_add_or_update_cluster(self->m_config->m_cluster_manager, &sub_cluster, ""))
    {
        upstream_t* upstream = lztq_elem_data(lztq_first(sub_cluster.lb_endpoints));
        upstream_cfg_t* dscfg = upstream->config;
        g_autofree gchar* cluster_name = create_cluster_name(dscfg->name, dscfg->port);
        RpDfpCluster* new_cluster = rp_dfp_cluster_store_load(rp_dfp_cluster_store_get_instance(self->m_config->m_cluster_manager), cluster_name);
        rp_dfp_cluster_impl_set_disable_sub_cluster(RP_DFP_CLUSTER_IMPL(new_cluster), true);
        return true;
    }

    return false;
}

static RpFilterHeadersStatus_e
load_dynamic_cluster(RpDynamicForwardProxy* self, RpDfpCluster* cluster, evhtp_headers_t* request_headers, guint16 default_port)
{
    NOISY_MSG_("(%p, %p, %p, %u)", self, cluster, request_headers, default_port);

    RpAuthorityAttributes host_attributes = http_utility_parse_authority(evhtp_header_find(request_headers, RpHeaderValues.HostLegacy));
    char* host = g_strndup(host_attributes.m_host.m_data, host_attributes.m_host.m_length);
    guint16 port = host_attributes.m_port ? host_attributes.m_port : default_port;

    //TODO...applyFilterStateOverrides(...)

    rp_filter_state_set_data(FILTER_STATE(self),
                                dynamic_host_key,
                                host,
                                RpFilterStateStateType_ReadOnly,
                                RpFilterStateLifeSpan_Request);
    rp_filter_state_set_data(FILTER_STATE(self),
                                dynamic_port_key,
                                g_memdup2(&port, sizeof(port)),
                                RpFilterStateStateType_ReadOnly,
                                RpFilterStateLifeSpan_Request);

    g_autoptr(GString) cluster_name = g_string_new("DFPCluster:");
    g_string_append_printf(cluster_name, "%s:%u", host, port);
    RpThreadLocalCluster* local_cluster = rp_cluster_manager_get_thread_local_cluster(self->m_config->m_cluster_manager, cluster_name->str);
    if (local_cluster && rp_dfp_cluster_touch(cluster, cluster_name->str))
    {
        LOGD("using the thread local cluster after touch success");
        return RpFilterHeadersStatus_Continue;
    }

    if (!add_dynamic_cluster(self, cluster, cluster_name->str, host, port))
    {
        LOGE("sub clusters overflow");
        //TODO...this->decoder_callbacks_->sendLocalReply(...)
        return RpFilterHeadersStatus_StopIteration;
    }

    //TODO...

    return RpFilterHeadersStatus_Continue;
}

static RpFilterHeadersStatus_e
decode_headers_i(RpStreamDecoderFilter* self, evhtp_headers_t* request_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, request_headers, end_stream);
    RpRoute* route = rp_stream_filter_callbacks_route(STREAM_FILTER_CALLBACKS(self));
    RpRouteEntry* route_entry;
    if (!route || !(route_entry = rp_route_route_entry(route)))
    {
        NOISY_MSG_("no route");
        return RpFilterHeadersStatus_Continue;
    }

    RpDynamicForwardProxy* me = RP_DYNAMIC_FORWARD_PROXY(self);
    RpThreadLocalCluster* cluster = rp_cluster_manager_get_thread_local_cluster(me->m_config->m_cluster_manager, rp_route_entry_cluster_name(route_entry));
    if (!cluster)
    {
        NOISY_MSG_("no cluster for \"%s\"", rp_route_entry_cluster_name(route_entry));
        return RpFilterHeadersStatus_Continue;
    }
    me->m_cluster_info = rp_cluster_info(RP_CLUSTER(cluster));

    if (rp_cluster_info_type(me->m_cluster_info) != RpDiscoveryType_STRICT_DNS)
    {
        NOISY_MSG_("not strict dns");
        return RpFilterHeadersStatus_Continue;
    }

    if (!me->m_cluster_store)
    {
        NOISY_MSG_("allocating cluster store");
        me->m_cluster_store = rp_dfp_cluster_store_get_instance(me->m_config->m_cluster_manager);
    }

    guint16 default_port = 80;
    //TODO...if (cluster_info_->transportSocketMatcher(...)...)

    RpDfpCluster* dfp_cluster = rp_dfp_cluster_store_load(me->m_cluster_store,
                                                            rp_cluster_info_name(me->m_cluster_info));
    if (!dfp_cluster)
    {
        LOGD("dynamic forward cluster is gone");
        //TODO...this->decoder_callbacks_->sendLocalReply(....)
        return RpFilterHeadersStatus_StopIteration;
    }

    if (rp_dfp_cluster_enable_sub_cluster(dfp_cluster))
    {
        NOISY_MSG_("loading dynamic cluster");
        return load_dynamic_cluster(me, dfp_cluster, request_headers, default_port);
    }

    return RpFilterHeadersStatus_Continue;
}

static void
decode_complete_i(RpStreamDecoderFilter* self)
{
    NOISY_MSG_("(%p)", self);
    PARENT_STREAM_DECODER_FILTER_IFACE(self)->decode_complete(self);
}

static RpFilterDataStatus_e
decode_data_i(RpStreamDecoderFilter* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    return PARENT_STREAM_DECODER_FILTER_IFACE(self)->decode_data(self, data, end_stream);
}

static RpFilterTrailerStatus_e
decode_trailers_i(RpStreamDecoderFilter* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    return PARENT_STREAM_DECODER_FILTER_IFACE(self)->decode_trailers(self, trailers);
}

static void
set_decoder_filter_callbacks_i(RpStreamDecoderFilter* self, RpStreamDecoderFilterCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    PARENT_STREAM_DECODER_FILTER_IFACE(self)->set_decoder_filter_callbacks(self, callbacks);
}

static inline void
stream_decoder_filter_iface_init(RpStreamDecoderFilterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->decode_headers = decode_headers_i;
    iface->decode_complete = decode_complete_i;
    iface->decode_data = decode_data_i;
    iface->decode_trailers = decode_trailers_i;
    iface->set_decoder_filter_callbacks = set_decoder_filter_callbacks_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpDynamicForwardProxy* self = RP_DYNAMIC_FORWARD_PROXY(obj);
    // Shared singleton store is process-lifetime; do not free here.
    self->m_cluster_store = NULL;
    g_clear_object(&self->m_config->m_cluster_manager);

    G_OBJECT_CLASS(rp_dynamic_forward_proxy_parent_class)->dispose(obj);
}

static void
rp_dynamic_forward_proxy_class_init(RpDynamicForwardProxyClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_dynamic_forward_proxy_init(RpDynamicForwardProxy* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    // Initialize optional members
    self->m_cluster_store = NULL;
}

static inline RpDynamicForwardProxy*
dynamic_forward_proxy_new(RpDynamicForwardProxyCfg* config)
{
    NOISY_MSG_("(%p)", config);
    RpDynamicForwardProxy* self = g_object_new(RP_TYPE_DYNAMIC_FORWARD_PROXY, NULL);
    self->m_config = config;
    return self;
}

static void
filter_factory_cb(RpFilterFactoryCb* self, RpFilterChainFactoryCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);

    RpDynamicForwardProxyCb* me = RP_DYNAMIC_FORWARD_PROXY_CB(self);
    RpDynamicForwardProxy* filter = dynamic_forward_proxy_new(&me->m_config);
    me->m_config.m_cluster_manager = CLUSTER_MANAGER(callbacks);
    rp_filter_chain_factory_callbacks_add_stream_decoder_filter(callbacks, RP_STREAM_DECODER_FILTER(filter));
}

static inline RpDynamicForwardProxyCb
dynamic_forward_proxy_cb_ctor(RpDynamicForwardProxyCfg* proto_config)
{
    NOISY_MSG_("(%p)", proto_config);
    RpDynamicForwardProxyCb self = {
        .parent_instance = rp_filter_factory_cb_ctor(filter_factory_cb, g_free),
        .m_config = *proto_config
    };
    return self;
}

static inline RpDynamicForwardProxyCb*
dynamic_forward_proxy_cb_new(RpDynamicForwardProxyCfg* proto_config, RpFactoryContext* context G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p)", proto_config, context);
    RpDynamicForwardProxyCb* self = g_new0(RpDynamicForwardProxyCb, 1);
    *self = dynamic_forward_proxy_cb_ctor(proto_config);
    return self;
}

RpFilterFactoryCb*
rp_dynamic_forward_proxy_create_filter_factory(RpFactoryContext* context)
{
    LOGD("(%p)", context);
    g_return_val_if_fail(RP_IS_FACTORY_CONTEXT(context), NULL);
    RpDynamicForwardProxyCfg proto_config = rp_dynamic_forward_proxy_cfg_ctor(NULL);
    return RP_FILTER_FACTORY_CB(dynamic_forward_proxy_cb_new(&proto_config, context));
}
