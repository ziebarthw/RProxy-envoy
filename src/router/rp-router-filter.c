/*
 * rp-router-filter.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_router_filter_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_router_filter_NOISY)
#   define NOISY_MSG_ LOGD
#   define IF_NOISY_(x, ...) x##__VA_ARGS__
#else
#   define NOISY_MSG_(x, ...)
#   define IF_NOISY_(x, ...)
#endif

#include "event/rp-dispatcher-impl.h"
#include "router/rp-upstream-request.h"
#include "rp-http-conn-manager-impl.h"
#include "rp-http-conn-mgr-impl-active-stream.h"
#include "rp-filter-chain-factory-callbacks-impl.h"
#include "rp-filter-manager.h"
#include "rp-headers.h"
#include "rp-http-utility.h"
#include "rp-load-balancer.h"
#include "rp-per-host-upstream.h"
#include "rp-router.h"
#include "rp-router-filter.h"

#define RP_ROUTER_FILTER_CB(s) (RpRouterFilterCb*)s

typedef struct _RpRouterFilterCb RpRouterFilterCb;
struct _RpRouterFilterCb {
    RpFilterFactoryCb parent_instance;
    RpRouterFilterConfig* m_config;
    RpFactoryContext* m_context;
};

struct _RpRouterFilter {
    GObject parent_instance;

    RpRouterFilterConfig* m_config;
    RpGenericConnPoolFactory* m_generic_conn_pool_factory;

    RpStreamDecoderFilterCallbacks* m_callbacks;
    RpRoute* m_route;
    RpRouteEntry* m_route_entry;
    RpClusterInfoConstSharedPtr m_cluster;
//TODO...    void (*m_on_host_selected)(RpRouterFilter*, RpHostDescription*, const char*);
    GSList/*<UpstreamRequestPtr>*/* m_upstream_requests;
    RpUpstreamRequest* m_final_upstream_request;
    SHARED_PTR(RpClusterManager) m_cluster_manager;

    evhtp_headers_t* m_downstream_headers;
    evhtp_headers_t* m_downstream_trailers;

    modify_headers_cb m_modify_headers;

    gint64 m_downstream_request_complete_time;

    guint32 m_attempt_count;
    guint32 m_pending_retries;

    bool m_downstream_response_started : 1;
    bool m_downstream_end_stream : 1;
    bool m_is_retry : 1;
    bool m_include_attempt_count_in_request : 1;
    bool m_include_timeout_retry_header_in_request : 1;
    bool m_request_buffer_overflowed : 1;
    bool m_allow_multiplexed_upstream_half_close : 1;

};

static void stream_filter_base_iface_init(RpStreamFilterBaseInterface* iface);
static void stream_decoder_filter_iface_init(RpStreamDecoderFilterInterface* iface);
static void load_balancer_context_iface_init(RpLoadBalancerContextInterface* iface);
static void router_filter_interface_iface_init(RpRouterFilterInterfaceInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpRouterFilter, rp_router_filter, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_FILTER_BASE, stream_filter_base_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_DECODER_FILTER, stream_decoder_filter_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_LOAD_BALANCER_CONTEXT, load_balancer_context_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_ROUTER_FILTER_INTERFACE, router_filter_interface_iface_init)
)

static void
on_request_complete(RpRouterFilter* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_downstream_end_stream = true;
//    RpDispatcher* dispatcher = rp_stream_filter_callbacks_dispatcher(self->m_callbacks);
    self->m_downstream_request_complete_time = g_get_monotonic_time();
    if (self->m_upstream_requests)
    {
        NOISY_MSG_("%u upstream requests", g_slist_length(self->m_upstream_requests));
        //TODO...

    }
}

static void
default_modify_headers(evhtp_headers_t* headers G_GNUC_UNUSED, void* arg G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p)", headers, arg);
}

#define DISPATCHER(s) \
    rp_stream_filter_callbacks_dispatcher(RP_STREAM_FILTER_CALLBACKS((s)->m_callbacks))
#define STREAM_INFO(s) \
    rp_stream_filter_callbacks_stream_info(RP_STREAM_FILTER_CALLBACKS((s)->m_callbacks))

static void
reset_all(RpRouterFilter* self)
{
    NOISY_MSG_("(%p)", self);
    while (self->m_upstream_requests)
    {
        GSList* entry = g_slist_last(self->m_upstream_requests);
        RpUpstreamRequestPtr request_ptr = RP_UPSTREAM_REQUEST(entry->data);
        self->m_upstream_requests = g_slist_delete_link(self->m_upstream_requests, entry);
        rp_upstream_request_reset_stream(request_ptr);
        rp_dispatcher_deferred_delete(DISPATCHER(self), G_OBJECT(g_steal_pointer(&request_ptr)));
    }
}

static void
reset_other_upstreams(RpRouterFilter* self, RpUpstreamRequest* upstream_request)
{
    NOISY_MSG_("(%p, %p)", self, upstream_request);
    GSList* final_upstream_request;
    while (self->m_upstream_requests)
    {
        GSList* entry = g_slist_last(self->m_upstream_requests);
        RpUpstreamRequestPtr upstream_request_tmp = RP_UPSTREAM_REQUEST(entry->data);
        if (upstream_request_tmp != upstream_request)
        {
            self->m_upstream_requests = g_slist_delete_link(self->m_upstream_requests, entry);
            rp_upstream_request_reset_stream(upstream_request_tmp);
        }
        else
        {
            self->m_upstream_requests = g_slist_remove_link(self->m_upstream_requests, entry);
            final_upstream_request = entry;
        }
    }

    // Now put the final request back in the list.
    self->m_upstream_requests = g_slist_concat(self->m_upstream_requests, final_upstream_request);
}

static void
cleanup(RpRouterFilter* self)
{
    NOISY_MSG_("(%p)", self);
    g_assert(!self->m_upstream_requests);
//TODO...
}

static void
on_upstream_complete(RpRouterFilter* self, RpUpstreamRequest* upstream_request)
{
    NOISY_MSG_("(%p, %p)", self, upstream_request);
    if (!self->m_downstream_end_stream)
    {
        NOISY_MSG_("downstream not finished");
        if (self->m_allow_multiplexed_upstream_half_close)
        {
            NOISY_MSG_("returning");
            return;
        }
        rp_upstream_request_reset_stream(upstream_request);
    }
//    RpDispatcher* dispatcher = rp_stream_filter_callbacks_dispatcher(self->m_callbacks);
    IF_NOISY_(gint64 response_time = g_get_monotonic_time() - self->m_downstream_request_complete_time;)
    NOISY_MSG_("response time %zd usec", response_time);

    //TODO...

    self->m_upstream_requests = g_slist_remove(self->m_upstream_requests, upstream_request);
    NOISY_MSG_("%u upstream requests", g_slist_length(self->m_upstream_requests));
    rp_dispatcher_deferred_delete(DISPATCHER(self), G_OBJECT(upstream_request));
    cleanup(self);
}

static void
on_destroy_i(RpStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    RpRouterFilter* me = RP_ROUTER_FILTER(self);
    //TODO...

    reset_all(me);

    //TODO...

    cleanup(me);
}

static RpLocalErrorStatus_e
on_local_reply_i(RpStreamFilterBase* self G_GNUC_UNUSED, const struct RpStreamFilterBase_LocalReplyData* reply_data G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p)", self, reply_data);
    //TODO...

    //TODO...

    return RpLocalErrorStatus_Continue;
}

static void
stream_filter_base_iface_init(RpStreamFilterBaseInterface* iface)
{
    LOGD("(%p)", iface);
    iface->on_destroy = on_destroy_i;
    iface->on_local_reply = on_local_reply_i;
}

static UNIQUE_PTR(RpGenericConnPool)
create_conn_pool(RpRouterFilter* self, RpThreadLocalCluster* cluster, RpHostConstSharedPtr host)
{
    NOISY_MSG_("(%p, %p, %p)", self, cluster, host);
    if (!host)
    {
        LOGD("host is null");
        return NULL;
    }

    RpGenericConnPoolFactory* factory = self->m_generic_conn_pool_factory;
    RpUpstreamProtocol_e upstream_protocol = RpUpstreamProtocol_HTTP;
    RpResourcePriority_e priority = rp_route_entry_priority(self->m_route_entry);
    RpStreamInfo* stream_info = STREAM_INFO(self);
    evhtp_proto downstream_protocol = rp_stream_info_protocol(stream_info);
    if (/*TODO...route_entry_->connectConfig().has_value()*/true)
    {
        const char* method = evhtp_header_find(self->m_downstream_headers, RpHeaderValues.Method);
        if (/*TODO...Http::HeaderUtility::isConnectUdpRequest(*downstream_headers_)*/false)
        {
            NOISY_MSG_("udp placeholder");
        }
        else if (g_ascii_strcasecmp(method, RpHeaderValues.MethodValues.Connect) == 0 /*TODO...
            (rout_entry_->connectConfig()->allow_post()...)*/)
        {
            NOISY_MSG_("TCP");
            upstream_protocol = RpUpstreamProtocol_TCP;
        }
    }
    return rp_generic_conn_pool_factory_create_generic_conn_pool(factory, host, cluster, upstream_protocol, priority, downstream_protocol, RP_LOAD_BALANCER_CONTEXT(self));
}

static RpFilterHeadersStatus_e
continue_decode_headers(RpRouterFilter* self, RpThreadLocalCluster* cluster, evhtp_headers_t* request_headers, bool end_stream,
                        modify_headers_cb modify_headers, bool* should_continue_decoding,
                        RpHostConstSharedPtr selected_host, const char* selected_host_details)
{
    NOISY_MSG_("(%p, %p, %p, %u, %p, %p, %p, %p(%s))",
        self, cluster, request_headers, end_stream, modify_headers, should_continue_decoding,
        selected_host, selected_host_details, selected_host_details);

    RpGenericConnPool* generic_conn_pool = create_conn_pool(self, cluster, selected_host);
    if (!generic_conn_pool)
    {
        NOISY_MSG_("sendNoHealthyUpstreamResponse()");
//TODO...sendNoHealthyUpstreamResponse(host_selection_details);
        return RpFilterHeadersStatus_StopIteration;
    }

    IF_NOISY_(RpHostDescription* host = rp_generic_conn_pool_host(generic_conn_pool);)
    NOISY_MSG_("host %p", host);

#if 0
    RpStreamFilterCallbacks* callbacks = RP_STREAM_FILTER_CALLBACKS(self->m_callbacks);
#endif//0
    RpStreamInfo* stream_info = STREAM_INFO(self);
    rp_route_entry_finalize_request_headers(self->m_route_entry, request_headers, stream_info, /*!config_->suppress_envoy_headers_*/false);

//TODO...FilterUtility::setUpstreamScheme()

    RpUpstreamRequestPtr upstream_request = rp_upstream_request_new(RP_ROUTER_FILTER_INTERFACE(self),
                                                                    generic_conn_pool,
                                                                    false,
                                                                    false,
                                                                    self->m_allow_multiplexed_upstream_half_close);
    self->m_upstream_requests = g_slist_prepend(self->m_upstream_requests, upstream_request);
    rp_upstream_request_accept_headers_from_router(upstream_request, end_stream);
    self->m_modify_headers = modify_headers;

    if (end_stream)
    {
        on_request_complete(self);
    }

    if (should_continue_decoding) *should_continue_decoding = true;

    return RpFilterHeadersStatus_StopIteration;
}

static RpFilterHeadersStatus_e
decode_headers_i(RpStreamDecoderFilter* self, evhtp_headers_t* request_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, request_headers, end_stream);
    RpRouterFilter* me = RP_ROUTER_FILTER(self);
    me->m_downstream_headers = request_headers;

    modify_headers_cb modify_headers = default_modify_headers;

    me->m_route = rp_stream_filter_callbacks_route(RP_STREAM_FILTER_CALLBACKS(me->m_callbacks));
    if (!me->m_route)
    {
        LOGD("no route match for URL(%s)", evhtp_header_find(request_headers, RpHeaderValues.Path));

        RpStreamInfo* stream_info = STREAM_INFO(me);
        rp_stream_info_set_response_flag(stream_info, RpCoreResponseFlag_NoRouteFound);
        rp_stream_decoder_filter_callbacks_send_local_reply(me->m_callbacks,
                                                            EVHTP_RES_NOTFOUND,
                                                            NULL,
                                                            modify_headers,
                                                            "route_not_found",
                                                            self);
        return RpFilterHeadersStatus_StopIteration;
    }

    RpDirectResponseEntry* direct_response = rp_route_direct_response_entry(me->m_route);
    if (direct_response)
    {
return RpFilterHeadersStatus_StopIteration;
    }

    me->m_route_entry = rp_route_route_entry(me->m_route);
    NOISY_MSG_("route_entry %p", me->m_route_entry);

    const char* cluster_name = rp_route_entry_cluster_name(me->m_route_entry);
    NOISY_MSG_("cluster_name \"%s\"", cluster_name);

    RpThreadLocalCluster* cluster = rp_cluster_manager_get_thread_local_cluster(me->m_cluster_manager, cluster_name);
    if (!cluster)
    {
        LOGD("unknown cluster \"%s\"", cluster_name);
        RpStreamInfo* stream_info = STREAM_INFO(me);
        rp_stream_info_set_response_flag(stream_info, RpCoreResponseFlag_NoClusterFound);
        rp_stream_decoder_filter_callbacks_send_local_reply(me->m_callbacks,
                                                            EVHTP_RES_NOTFOUND/*TODO...route_entry_->clusterNotFoundResponseCode()*/,
                                                            NULL,
                                                            modify_headers,
                                                            "cluster_not_found",
                                                            self);
        return RpFilterHeadersStatus_StopIteration;
    }
    me->m_cluster = rp_thread_local_cluster_info(cluster);

    RpHostSelectionResponse host_selection_response = rp_thread_local_cluster_choose_host(cluster, RP_LOAD_BALANCER_CONTEXT(self));
    if (!host_selection_response.m_cancelable /*|| TODO....*/)
    {
        return continue_decode_headers(me, cluster, request_headers, end_stream,
                                        modify_headers, NULL,
                                        host_selection_response.m_host, host_selection_response.m_details);
    }

//TODO...async host selection
return RpFilterHeadersStatus_StopIteration;
}

static RpFilterDataStatus_e
decode_data_i(RpStreamDecoderFilter* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)",
        self, data, data ? evbuffer_get_length(data) : 1, end_stream);
//TODO...

    RpRouterFilter* me = RP_ROUTER_FILTER(self);
    if (me->m_upstream_requests)
    {
        RpUpstreamRequest* upstream_request = RP_UPSTREAM_REQUEST(me->m_upstream_requests->data);
        rp_upstream_request_accept_data_from_router(upstream_request, data, end_stream);
    }
    else
    {
        cleanup(me);
        evbuf_t* body = evbuffer_new();
        char msg[] = "upstream is closed prematurely during decoding data from downstream";
        evbuffer_add(body, msg, sizeof(msg) - 1);
        rp_stream_decoder_filter_callbacks_send_local_reply(me->m_callbacks,
                                                            EVHTP_RES_SERVUNAVAIL,
                                                            body,
                                                            NULL/*modify_headers*/,
                                                            "early_upstream_reset",
                                                            self);
        return RpFilterDataStatus_StopIterationNoBuffer;
    }

    if (end_stream)
    {
        on_request_complete(me);
    }

    return RpFilterDataStatus_StopIterationNoBuffer;
}

static RpFilterTrailerStatus_e
decode_trailers_i(RpStreamDecoderFilter* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);

    RpRouterFilter* me = RP_ROUTER_FILTER(self);
    me->m_downstream_trailers = trailers;
    if (me->m_upstream_requests)
    {
        RpUpstreamRequest* upstream_request = RP_UPSTREAM_REQUEST(me->m_upstream_requests->data);
        rp_upstream_request_accept_trailers_from_router(upstream_request, trailers);
    }
    return RpFilterTrailerStatus_Continue;
}

static void
set_decoder_filter_callbacks_i(RpStreamDecoderFilter* self, RpStreamDecoderFilterCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    RP_ROUTER_FILTER(self)->m_callbacks = callbacks;
    //TODO...
}

static void
decode_complete_i(RpStreamDecoderFilter* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static void
stream_decoder_filter_iface_init(RpStreamDecoderFilterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->decode_headers = decode_headers_i;
    iface->decode_data = decode_data_i;
    iface->decode_trailers = decode_trailers_i;
    iface->set_decoder_filter_callbacks = set_decoder_filter_callbacks_i;
    iface->decode_complete = decode_complete_i;
}

static RpNetworkConnection*
downstream_connection_i(RpLoadBalancerContext* self)
{
    NOISY_MSG_("(%p)", self);
    RpRouterFilter* me = RP_ROUTER_FILTER(self);
    return rp_stream_filter_callbacks_connection(RP_STREAM_FILTER_CALLBACKS(me->m_callbacks));
}

static evhtp_headers_t*
load_balancer_context_downstream_headers_i(RpLoadBalancerContext* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_ROUTER_FILTER(self)->m_downstream_headers;
}

static RpStreamInfo*
request_stream_info_i(RpLoadBalancerContext* self)
{
    NOISY_MSG_("(%p)", self);
    RpRouterFilter* me = RP_ROUTER_FILTER(self);
    return STREAM_INFO(me);
}

static bool
should_select_another_host_i(RpLoadBalancerContext* self, RpHost* host)
{
    NOISY_MSG_("(%p, %p)", self, host);
    RpRouterFilter* me = RP_ROUTER_FILTER(self);
    if (!me->m_is_retry)
    {
        return false;
    }

    //TODO...retry_state_->should...
return false;
}

static guint32
host_selection_retry_count_i(RpLoadBalancerContext* self)
{
    NOISY_MSG_("(%p)", self);
    RpRouterFilter* me = RP_ROUTER_FILTER(self);
    if (!me->m_is_retry)
    {
        return 1;
    }
    //TODO...retry_state_->...
    return 1;
}

static RpOverrideHost
override_host_to_select_i(RpLoadBalancerContext* self)
{
    NOISY_MSG_("(%p)", self);
    RpRouterFilter* me = RP_ROUTER_FILTER(self);
    if (me->m_is_retry)
    {
        NOISY_MSG_("returning empty result");
        return RpOverrideHost_make(NULL, false);
    }
NOISY_MSG_("callbacks %p", me->m_callbacks);
    return rp_stream_decoder_filter_callbacks_upstream_override_host(me->m_callbacks);
}

static void
load_balancer_context_iface_init(RpLoadBalancerContextInterface* iface)
{
    LOGD("(%p)", iface);
    iface->downstream_connection = downstream_connection_i;
    iface->downstream_headers = load_balancer_context_downstream_headers_i;
    iface->request_stream_info = request_stream_info_i;
    iface->should_select_another_host = should_select_another_host_i;
    iface->host_selection_retry_count = host_selection_retry_count_i;
    iface->override_host_to_select = override_host_to_select_i;
}

static RpStreamDecoderFilterCallbacks*
callbacks_i(RpRouterFilterInterface* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_ROUTER_FILTER(self)->m_callbacks;
}

static RpClusterInfoConstSharedPtr
cluster_i(RpRouterFilterInterface* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_ROUTER_FILTER(self)->m_cluster;
}

static RpRouterFilterConfig*
config_i(RpRouterFilterInterface* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_ROUTER_FILTER(self)->m_config;
}

static evhtp_headers_t*
downstream_headers_i(RpRouterFilterInterface* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_ROUTER_FILTER(self)->m_downstream_headers;
}

static evhtp_headers_t*
downstream_trailers_i(RpRouterFilterInterface* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_ROUTER_FILTER(self)->m_downstream_trailers;
}

static bool
downstream_response_started_i(RpRouterFilterInterface* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_ROUTER_FILTER(self)->m_downstream_response_started;
}

static bool
downstream_end_stream_i(RpRouterFilterInterface* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_ROUTER_FILTER(self)->m_downstream_end_stream;
}

static guint32
attempt_count_i(RpRouterFilterInterface* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_ROUTER_FILTER(self)->m_attempt_count;
}

static void
on_upstream_host_selected_i(RpRouterFilterInterface* self G_GNUC_UNUSED, RpHostDescription* host G_GNUC_UNUSED, bool pool_success)
{
    NOISY_MSG_("(%p, %p, %u)", self, host, pool_success);
    //TODO...

    if (!pool_success)
    {
        NOISY_MSG_("not success");
        return;
    }

    //TODO...vcluster...
}

static void
on_upstream_1xx_headers_i(RpRouterFilterInterface* self, evhtp_headers_t* response_headers, RpUpstreamRequest* upstream_request)
{
    NOISY_MSG_("(%p, %p, %p)", self, response_headers, upstream_request);
    RpRouterFilter* me = RP_ROUTER_FILTER(self);
#if 0
    evhtp_res response_code = http_utility_get_response_status(response_headers);
#endif//0

    me->m_downstream_response_started = true;
    me->m_final_upstream_request = upstream_request;
    reset_other_upstreams(me, upstream_request);

    //TODO...rety_state_.reset();

    rp_stream_decoder_filter_callbacks_encode_1xx_headers(me->m_callbacks, response_headers);
}

static void
on_upstream_headers_i(RpRouterFilterInterface* self, guint64 response_code, evhtp_headers_t* response_headers,
                        RpUpstreamRequest* upstream_request, bool end_stream)
{
    NOISY_MSG_("(%p, %lu, %p, %p, %u)", self, response_code, response_headers, upstream_request, end_stream);
    RpRouterFilter* me = RP_ROUTER_FILTER(self);

    me->m_modify_headers(response_headers, me);

    //TODO...retry stuff...

    //TODO...internal redirect stuff...

    //TODO...kill upstream in flight if bad response...

    //TODO...reset rety state

    RpStreamInfo* stream_info = rp_stream_filter_callbacks_stream_info(RP_STREAM_FILTER_CALLBACKS(me->m_callbacks));
    rp_stream_info_set_response_code(stream_info, response_code);

    rp_response_entry_finalize_response_headers(RP_RESPONSE_ENTRY(me->m_route_entry), response_headers, stream_info);

    me->m_downstream_response_started = true;
    me->m_final_upstream_request = upstream_request;
    rp_stream_info_set_upstream_info(stream_info,
        rp_stream_info_upstream_info(
            rp_upstream_request_stream_info(upstream_request)));
    reset_other_upstreams(me, upstream_request);
    if (end_stream)
    {
        on_upstream_complete(me, upstream_request);
    }

    rp_stream_decoder_filter_callbacks_encode_headers(me->m_callbacks, response_headers, end_stream, "via_upstream");
}

static void
on_upstream_trailers_i(RpRouterFilterInterface* self, evhtp_headers_t* trailers, RpUpstreamRequest* upstream_request)
{
    NOISY_MSG_("(%p, %p, %p)", self, trailers, upstream_request);
    RpRouterFilter* me = RP_ROUTER_FILTER(self);
    on_upstream_complete(me, upstream_request);
    rp_stream_decoder_filter_callbacks_encode_trailers(me->m_callbacks, trailers);
}

static void
on_upstream_data_i(RpRouterFilterInterface* self, evbuf_t* data, RpUpstreamRequest* upstream_request, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %p, %u)", self, data, data ? evbuffer_get_length(data) : 0, upstream_request, end_stream);
    RpRouterFilter* me = RP_ROUTER_FILTER(self);
    g_assert(g_slist_length(me->m_upstream_requests) == 1);
    if (end_stream)
    {
        NOISY_MSG_("calling on_upstream_complete(%p, %p)", me, upstream_request);
        on_upstream_complete(me, upstream_request);
    }

    rp_stream_decoder_filter_callbacks_encode_data(me->m_callbacks, data, end_stream);
}

static bool
maybe_retry_reset(RpRouterFilter* self, RpStreamResetReason_e reset_reason, RpUpstreamRequest* upstream_request/*, TODO...TimeoutRetry...*/)
{
    NOISY_MSG_("(%p, %d, %p)", self, reset_reason, upstream_request);
    //TODO...if (downstream_response_started_ || ...)
return false;
}

static void
awaiting_headers_f(RpUpstreamRequest* upstream_request, guint32* count)
{
    NOISY_MSG_("(%p, %p)", upstream_request, count);
    if (rp_upstream_request_awaiting_headers(upstream_request)) \
        ++(*count);
}

static guint32
num_requests_awaiting_headers(RpRouterFilter* self)
{
    NOISY_MSG_("(%p)", self);
    guint32 count = 0;
    g_slist_foreach(self->m_upstream_requests, (GFunc)awaiting_headers_f, &count);
    return count;
}

static void
on_upstream_abort(RpRouterFilter* self, evhtp_res code, RpCoreResponseFlag_e response_flags, evbuf_t* body, bool dropped, const char* details)
{
    NOISY_MSG_("(%p, %d, %d, %p, %u, %p(%s))",
        self, code, response_flags, body, dropped, details, details);
    rp_stream_decoder_filter_callbacks_send_local_reply(self->m_callbacks, code, body, /*TODO...modify_headers_*/ NULL, details, self);
}

static inline RpCoreResponseFlag_e
stream_request_reason_to_response_flag(RpStreamResetReason_e reset_reason)
{
    NOISY_MSG_("(%d)", reset_reason);
    switch (reset_reason)
    {
        case RpStreamResetReason_LocalConnectionFailure:
        case RpStreamResetReason_RemoteConnectionFailure:
        case RpStreamResetReason_ConnectionTimeout:
            return RpCoreResponseFlag_UpstreamConnectionFailure;
        case RpStreamResetReason_ConnectionTermination:
            return RpCoreResponseFlag_UpstreamConnectionTermination;
        case RpStreamResetReason_LocalReset:
        case RpStreamResetReason_LocalRefusedStreamReset:
        case RpStreamResetReason_Http1PrematureUpstreamHalfClose:
            return RpCoreResponseFlag_LocalReset;
        case RpStreamResetReason_Overflow:
            return RpCoreResponseFlag_UpstreamOverflow;
        case RpStreamResetReason_RemoteReset:
        case RpStreamResetReason_RemoteRefusedStreamReset:
        case RpStreamResetReason_ConnectError:
            return RpCoreResponseFlag_UpstreamRemoteReset;
        case RpStreamResetReason_ProtocolError:
            return RpCoreResponseFlag_UpstreamProtocolError;
        case RpStreamResetReason_OverloadManager:
default:
            return RpCoreResponseFlag_OverloadManager;
    }
}

static void
on_upstream_reset_i(RpRouterFilterInterface* self, RpStreamResetReason_e reset_reason, const char* transport_failure_reason, RpUpstreamRequest* upstream_request)
{
    NOISY_MSG_("(%p, %d, %p(%s), %p)", self, reset_reason, transport_failure_reason, transport_failure_reason, upstream_request);

    bool dropped = reset_reason == RpStreamResetReason_Overflow;
    if (!dropped)
    {
        //TODO...updateOutlierDetection(...)
    }

    RpRouterFilter* me = RP_ROUTER_FILTER(self);
    if (maybe_retry_reset(me, reset_reason, upstream_request))
    {
        NOISY_MSG_("returning");
        return;
    }

    evhtp_res error_code = (reset_reason == RpStreamResetReason_ProtocolError) ? EVHTP_RES_BADGATEWAY : EVHTP_RES_SERVUNAVAIL;
    //TODO...chargeUpstreamAbort(...);
    me->m_upstream_requests = g_slist_remove(me->m_upstream_requests, upstream_request);
    rp_dispatcher_deferred_delete(
        rp_stream_filter_callbacks_dispatcher(RP_STREAM_FILTER_CALLBACKS(me->m_callbacks)), G_OBJECT(g_steal_pointer(&upstream_request)));

    if (num_requests_awaiting_headers(me) > 0 || me->m_pending_retries > 0)
    {
        NOISY_MSG_("returning");
        return;
    }

    RpCoreResponseFlag_e response_flags = stream_request_reason_to_response_flag(reset_reason);

    //TODO...body = ...;
    on_upstream_abort(me, error_code, response_flags, /*TODO...body*/NULL, dropped, /*TODO...details*/NULL);
}

static void
router_filter_interface_iface_init(RpRouterFilterInterfaceInterface* iface)
{
    LOGD("(%p)", iface);
    iface->callbacks = callbacks_i;
    iface->cluster = cluster_i;
    iface->config = config_i;
    iface->downstream_headers = downstream_headers_i;
    iface->downstream_trailers = downstream_trailers_i;
    iface->downstream_response_started = downstream_response_started_i;
    iface->downstream_end_stream = downstream_end_stream_i;
    iface->attempt_count = attempt_count_i;
    iface->on_upstream_host_selected = on_upstream_host_selected_i;
    iface->on_upstream_1xx_headers = on_upstream_1xx_headers_i;
    iface->on_upstream_headers = on_upstream_headers_i;
    iface->on_upstream_trailers = on_upstream_trailers_i;
    iface->on_upstream_data = on_upstream_data_i;
    iface->on_upstream_reset = on_upstream_reset_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpRouterFilter* self = RP_ROUTER_FILTER(obj);
    g_clear_object(&self->m_cluster_manager);

    G_OBJECT_CLASS(rp_router_filter_parent_class)->dispose(obj);
}

static void
rp_router_filter_class_init(RpRouterFilterClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_router_filter_init(RpRouterFilter* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_attempt_count = 1;
    self->m_pending_retries = 0;
}

static inline RpRouterFilter*
router_filter_new(RpRouterFilterConfig* config, RpGenericConnPoolFactory* factory, RpClusterManager* cluster_manager)
{
    LOGD("(%p, %p, %p)", config, factory, cluster_manager);
    g_return_val_if_fail(RP_IS_ROUTER_FILTER_CONFIG(config), NULL);
    g_return_val_if_fail(RP_IS_CLUSTER_MANAGER(cluster_manager), NULL);
    RpRouterFilter* self = g_object_new(RP_TYPE_ROUTER_FILTER, NULL);
    self->m_config = config;
    self->m_generic_conn_pool_factory = factory;
    self->m_cluster_manager = g_object_ref(cluster_manager);
    return self;
}

static void
filter_factory_cb(RpFilterFactoryCb* self, RpFilterChainFactoryCallbacks* callbacks)
{
    extern RpPerHostGenericConnPoolFactory* default_conn_pool_factory;//REVISIT...

    NOISY_MSG_("(%p, %p)", self, callbacks);

    RpRouterFilterCb* me = RP_ROUTER_FILTER_CB(self);
    RpServerFactoryContext* server_factory_context = rp_generic_factory_context_server_factory_context(RP_GENERIC_FACTORY_CONTEXT(me->m_context));
    RpClusterManager* cluster_manager = rp_common_factory_context_cluster_manager(RP_COMMON_FACTORY_CONTEXT(server_factory_context));
    RpRouterFilter* filter = router_filter_new(me->m_config,
                                                RP_GENERIC_CONN_POOL_FACTORY(default_conn_pool_factory),
                                                cluster_manager);
    rp_filter_chain_factory_callbacks_add_stream_decoder_filter(callbacks, RP_STREAM_DECODER_FILTER(filter));
}

static void
destroy_router_filter_cb(RpRouterFilterCb* self)
{
    NOISY_MSG_("(%p)", self);
    g_clear_object(&self->m_config);
    g_free(self);
}

static inline RpRouterFilterCb
router_filter_cb_ctor(RpRouterFilterConfig* config, RpFactoryContext* context)
{
    NOISY_MSG_("(%p, %p)", config, context);
    RpRouterFilterCb self = {
        .parent_instance = rp_filter_factory_cb_ctor(filter_factory_cb, (GDestroyNotify)destroy_router_filter_cb),
        .m_config = config,
        .m_context = context
    };
    return self;
}

static inline RpRouterFilterCb*
router_filter_cb_new(RpRouterFilterConfig* config, RpFactoryContext* context)
{
    NOISY_MSG_("(%p, %p)", config, context);
    RpRouterFilterCb* self = g_new0(RpRouterFilterCb, 1);
    *self = router_filter_cb_ctor(config, context);
    return self;
}

RpFilterFactoryCb*
rp_router_filter_create_filter_factory(RpRouterCfg* proto_config, RpFactoryContext* context)
{
    LOGD("(%p, %p)", proto_config, context);
    g_return_val_if_fail(proto_config != NULL, NULL);
    g_return_val_if_fail(RP_IS_FACTORY_CONTEXT(context), NULL);
    return (RpFilterFactoryCb*)router_filter_cb_new(rp_router_filter_config_new(proto_config, context), context);
}

RpCoreResponseFlag_e
rp_router_filter_stream_reset_reason_to_response_flag(RpStreamResetReason_e reset_reason)
{
    LOGD("(%d)", reset_reason);
    switch (reset_reason)
    {
        case RpStreamResetReason_LocalConnectionFailure:
        case RpStreamResetReason_RemoteConnectionFailure:
        case RpStreamResetReason_ConnectionTimeout:
            return RpCoreResponseFlag_UpstreamConnectionFailure;
        case RpStreamResetReason_ConnectionTermination:
            return RpCoreResponseFlag_UpstreamConnectionTermination;
        case RpStreamResetReason_LocalReset:
        case RpStreamResetReason_LocalRefusedStreamReset:
        case RpStreamResetReason_Http1PrematureUpstreamHalfClose:
            return RpCoreResponseFlag_LocalReset;
        case RpStreamResetReason_Overflow:
            return RpCoreResponseFlag_UpstreamOverflow;
        case RpStreamResetReason_RemoteReset:
        case RpStreamResetReason_RemoteRefusedStreamReset:
        case RpStreamResetReason_ConnectError:
            return RpCoreResponseFlag_UpstreamRemoteReset;
        case RpStreamResetReason_ProtocolError:
            return RpCoreResponseFlag_UpstreamProtocolError;
        case RpStreamResetReason_OverloadManager:
            return RpCoreResponseFlag_OverloadManager;
        default:
            break;
    }
    return RpStreamResetReason_LocalReset;
}

GSList*
rp_router_filter_upstream_requests(RpRouterFilter* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_ROUTER_FILTER(self), NULL);
    return self->m_upstream_requests;
}
