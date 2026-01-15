/*
 * rp-dynamic-forward-proxy.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_dynamic_forward_proxy_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_dynamic_forward_proxy_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <netinet/in.h>
#include "event/rp-dispatcher-impl.h"
#include "transport_sockets/rp-raw-buffer.h"
#include "upstream/rp-upstream-impl.h"
#include "dynamic_forward_proxy/rp-cluster.h"
#include "rp-cluster-store.h"
#include "rp-filter-chain-factory-callbacks-impl.h"
#include "rp-headers.h"
#include "rp-http-conn-manager-impl.h"
#include "rp-http-conn-mgr-impl-active-stream.h"
#include "rp-state-filter.h"
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
#define RP_DYNAMIC_FORWARD_PROXY_CB(s) (RpDynamicForwardProxyCb*)s

G_DEFINE_INTERFACE(RpLoadClusterEntryHandle, rp_load_cluster_entry_handle, G_TYPE_OBJECT)
static void
rp_load_cluster_entry_handle_default_init(RpLoadClusterEntryHandleInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpLoadClusterEntryCallbacks, rp_load_cluster_entry_callbacks, G_TYPE_OBJECT)
static void
rp_load_cluster_entry_callbacks_default_init(RpLoadClusterEntryCallbacksInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

static inline RpSingletonManager*
singleton_manager_from_factory_context(RpFactoryContext* context)
{
    return rp_common_factory_context_singleton_manager(RP_COMMON_FACTORY_CONTEXT(
        rp_generic_factory_context_server_factory_context(RP_GENERIC_FACTORY_CONTEXT(context))));
}

typedef struct _RpDynamicForwardProxyCb RpDynamicForwardProxyCb;
struct _RpDynamicForwardProxyCb {
    RpFilterFactoryCb parent_instance;
    RpProxyFilterConfigSharedPtr filter_config;
};

struct _RpDynamicForwardProxy {
    RpPassThroughFilter parent_instance;

    RpProxyFilterConfigSharedPtr m_config;
    RpClusterInfoConstSharedPtr m_cluster_info;
    RpLoadClusterEntryHandlePtr m_cluster_load_handle;
};

static void stream_decoder_filter_iface_init(RpStreamDecoderFilterInterface* iface);
static void load_cluster_entry_callbacks_iface_init(RpLoadClusterEntryCallbacksInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpDynamicForwardProxy, rp_dynamic_forward_proxy, RP_TYPE_PASS_THROUGH_FILTER,
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_DECODER_FILTER, stream_decoder_filter_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_LOAD_CLUSTER_ENTRY_CALLBACKS, load_cluster_entry_callbacks_iface_init)
)

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
    RpClusterManager* cluster_manager = rp_proxy_filter_config_cluster_manager(self->m_config);
    RpThreadLocalCluster* local_cluster = rp_cluster_manager_get_thread_local_cluster(cluster_manager, cluster_name->str);
    if (local_cluster && rp_dfp_cluster_touch(cluster, cluster_name->str))
    {
        LOGD("using the thread local cluster after touch success");
        return RpFilterHeadersStatus_Continue;
    }

    self->m_cluster_load_handle = rp_proxy_filter_config_add_dynamic_cluster(self->m_config,
                                                                    cluster,
                                                                    cluster_name->str,
                                                                    host,
                                                                    port,
                                                                    RP_LOAD_CLUSTER_ENTRY_CALLBACKS(self));
    if (!self->m_cluster_load_handle)
    {
        LOGE("sub clusters overflow");
        //TODO...this->decoder_callbacks_->sendLocalReply(...)
        return RpFilterHeadersStatus_StopIteration;
    }

    //TODO...

    LOGD("waiting to load cluster entry");
    return RpFilterHeadersStatus_Continue;
}

static RpFilterHeadersStatus_e
decode_headers_i(RpStreamDecoderFilter* self, evhtp_headers_t* request_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, request_headers, end_stream);
    RpRouteConstSharedPtr route = rp_stream_filter_callbacks_route(STREAM_FILTER_CALLBACKS(self));
    RpRouteEntry* route_entry;
    if (!route || !(route_entry = rp_route_route_entry(route)))
    {
        NOISY_MSG_("no route");
        return RpFilterHeadersStatus_Continue;
    }

    RpDynamicForwardProxy* me = RP_DYNAMIC_FORWARD_PROXY(self);
    RpClusterManager* cluster_manager = rp_proxy_filter_config_cluster_manager(me->m_config);
    RpThreadLocalCluster* cluster = rp_cluster_manager_get_thread_local_cluster(cluster_manager, rp_route_entry_cluster_name(route_entry));
    if (!cluster)
    {
        NOISY_MSG_("no cluster for \"%s\"", rp_route_entry_cluster_name(route_entry));
        return RpFilterHeadersStatus_Continue;
    }
    me->m_cluster_info = rp_thread_local_cluster_info(cluster);

    if (rp_cluster_info_type(me->m_cluster_info) != RpDiscoveryType_STRICT_DNS)
    {
        NOISY_MSG_("not strict dns");
        return RpFilterHeadersStatus_Continue;
    }

    guint16 default_port = 80;
    //TODO...if (cluster_info_->transportSocketMatcher(...)...)

    RpDfpClusterStoreSharedPtr cluster_store = rp_proxy_filter_config_cluster_store(me->m_config);
    RpDfpCluster* dfp_cluster = rp_dfp_cluster_store_load(cluster_store, rp_cluster_info_name(me->m_cluster_info));
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

static void
stream_decoder_filter_iface_init(RpStreamDecoderFilterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->decode_headers = decode_headers_i;
    iface->decode_complete = decode_complete_i;
    iface->decode_data = decode_data_i;
    iface->decode_trailers = decode_trailers_i;
    iface->set_decoder_filter_callbacks = set_decoder_filter_callbacks_i;
}

static void
on_load_cluster_complete_i(RpLoadClusterEntryCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    rp_stream_decoder_filter_callbacks_continue_decoding(DECODER_CALLBACKS(self));
}

static void
load_cluster_entry_callbacks_iface_init(RpLoadClusterEntryCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_load_cluster_complete = on_load_cluster_complete_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpDynamicForwardProxy* self = RP_DYNAMIC_FORWARD_PROXY(obj);
    g_clear_object(&self->m_config);

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
}

static inline RpDynamicForwardProxy*
dynamic_forward_proxy_new(RpProxyFilterConfigSharedPtr config)
{
    NOISY_MSG_("(%p)", config);
    RpDynamicForwardProxy* self = g_object_new(RP_TYPE_DYNAMIC_FORWARD_PROXY, NULL);
    self->m_config = g_object_ref(config);
    return self;
}

static void
filter_factory_cb(RpFilterFactoryCb* self, RpFilterChainFactoryCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);

    RpDynamicForwardProxyCb* me = RP_DYNAMIC_FORWARD_PROXY_CB(self);
    RpDynamicForwardProxy* filter = dynamic_forward_proxy_new(me->filter_config);
    rp_filter_chain_factory_callbacks_add_stream_decoder_filter(callbacks, RP_STREAM_DECODER_FILTER(filter));
}

static inline void
dynamic_forward_proxy_cb_dtor(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    RpDynamicForwardProxyCb* self = RP_DYNAMIC_FORWARD_PROXY_CB(arg);
    g_clear_object(&self->filter_config);
    g_clear_pointer(&self, g_free);
}

static inline RpDynamicForwardProxyCb
dynamic_forward_proxy_cb_ctor(RpProxyFilterConfigSharedPtr filter_config)
{
    NOISY_MSG_("(%p)", filter_config);
    RpDynamicForwardProxyCb self = {
        .parent_instance = rp_filter_factory_cb_ctor(filter_factory_cb, dynamic_forward_proxy_cb_dtor),
        .filter_config = filter_config
    };
    return self;
}

static inline RpDynamicForwardProxyCb*
dynamic_forward_proxy_cb_new(RpFactoryContext* context)
{
    NOISY_MSG_("(%p)", context);
    RpDynamicForwardProxyCb* self = g_new0(RpDynamicForwardProxyCb, 1);
    RpSingletonManager* singleton_manager = singleton_manager_from_factory_context(context);
    g_autoptr(RpDfpClusterStoreFactory) cluster_store_factory = rp_dfp_cluster_store_factory_new(singleton_manager);
    *self = dynamic_forward_proxy_cb_ctor(rp_proxy_filter_config_new(cluster_store_factory, context));
    return self;
}

RpFilterFactoryCb*
rp_dynamic_forward_proxy_create_filter_factory(RpFactoryContext* context)
{
    LOGD("(%p)", context);
    g_return_val_if_fail(RP_IS_FACTORY_CONTEXT(context), NULL);
    return RP_FILTER_FACTORY_CB(dynamic_forward_proxy_cb_new(context));
}
