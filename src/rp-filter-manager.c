/*
 * rp-filter-manager.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_filter_manager_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_filter_manager_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-headers.h"
#include "rp-filter-factory.h"
#include "rp-filter-chain-factory-callbacks-impl.h"
#include "rp-http-utility.h"
#include "rp-filter-manager.h"

G_DEFINE_INTERFACE(RpFilterManagerCallbacks, rp_filter_manager_callbacks, G_TYPE_OBJECT)

static void
encode_headers_(RpFilterManagerCallbacks* self G_GNUC_UNUSED, evhtp_headers_t* response_headers G_GNUC_UNUSED, bool end_stream G_GNUC_UNUSED)
{
    g_debug("not implemented at line %d", __LINE__);
}

static void
encode_data_(RpFilterManagerCallbacks* self G_GNUC_UNUSED, evbuf_t* data G_GNUC_UNUSED, bool end_stream G_GNUC_UNUSED)
{
    g_debug("not implemented at line %d", __LINE__);
}

static void
set_response_headers_(RpFilterManagerCallbacks* self G_GNUC_UNUSED, evhtp_headers_t* response_headers G_GNUC_UNUSED)
{
    g_debug("not implemented at line %d", __LINE__);
}

static void
set_response_trailers_(RpFilterManagerCallbacks* self G_GNUC_UNUSED, evhtp_headers_t* response_trailers G_GNUC_UNUSED)
{
    g_debug("not implemented at line %d", __LINE__);
}

static void
charge_stats_(RpFilterManagerCallbacks* self G_GNUC_UNUSED, evhtp_headers_t* trailers G_GNUC_UNUSED)
{
}

static void
set_request_trailers_(RpFilterManagerCallbacks* self G_GNUC_UNUSED, evhtp_headers_t* request_trailers G_GNUC_UNUSED)
{
    g_debug("not implemented at line %d", __LINE__);
}

static evhtp_headers_t*
request_headers_(RpFilterManagerCallbacks* self G_GNUC_UNUSED)
{
    g_debug("not implemented at line %d", __LINE__);
    return NULL;
}

static evhtp_headers_t*
request_trailers_(RpFilterManagerCallbacks* self G_GNUC_UNUSED)
{
    g_debug("not implemented at line %d", __LINE__);
    return NULL;
}

static evhtp_headers_t*
response_headers_(RpFilterManagerCallbacks* self G_GNUC_UNUSED)
{
    g_debug("not implemented at line %d", __LINE__);
    return NULL;
}

static evhtp_headers_t*
response_trailers_(RpFilterManagerCallbacks* self G_GNUC_UNUSED)
{
    g_debug("not implemented at line %d", __LINE__);
    return NULL;
}

static void
end_stream_(RpFilterManagerCallbacks* self G_GNUC_UNUSED)
{
    g_debug("not implemented at line %d", __LINE__);
}

static void
disarm_request_timeout_(RpFilterManagerCallbacks* self G_GNUC_UNUSED)
{
    g_debug("not implemented at line %d", __LINE__);
}

static void
reset_idle_timer_(RpFilterManagerCallbacks* self G_GNUC_UNUSED)
{
    g_debug("not implemented at line %d", __LINE__);
}

static RpUpstreamStreamFilterCallbacks*
upstream_callbacks_(RpFilterManagerCallbacks* self G_GNUC_UNUSED)
{
    g_debug("not implemented at line %d", __LINE__);
    return NULL;
}

static RpDownstreamStreamFilterCallbacks*
downstream_callbacks_(RpFilterManagerCallbacks* self G_GNUC_UNUSED)
{
    g_debug("not implemented at line %d", __LINE__);
    return NULL;
}

static void
on_local_reply_(RpFilterManagerCallbacks* self G_GNUC_UNUSED, evhtp_res code G_GNUC_UNUSED)
{
    g_debug("not implemented at line %d", __LINE__);
}

static void
rp_filter_manager_callbacks_default_init(RpFilterManagerCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->encode_headers = encode_headers_;
    iface->encode_data = encode_data_;
    iface->end_stream = end_stream_;
    iface->on_local_reply = on_local_reply_;
    iface->request_headers = request_headers_;
    iface->request_trailers = request_trailers_;
    iface->response_headers = response_headers_;
    iface->response_trailers = response_trailers_;
    iface->disarm_request_timeout = disarm_request_timeout_;
    iface->reset_idle_timer = reset_idle_timer_;
    iface->set_response_headers = set_response_headers_;
    iface->set_response_trailers = set_response_trailers_;
    iface->set_request_trailers = set_request_trailers_;
    iface->charge_stats = charge_stats_;
    iface->downstream_callbacks = downstream_callbacks_;
    iface->upstream_callbacks = upstream_callbacks_;
}

void
rp_filter_manager_callbacks_encode_headers(RpFilterManagerCallbacks* self, evhtp_headers_t* response_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, response_headers, end_stream);
    g_return_if_fail(RP_IS_FILTER_MANAGER_CALLBACKS(self));
    RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->encode_headers(self,
                                                                response_headers,
                                                                end_stream);
}

void
rp_filter_manager_callbacks_encode_data(RpFilterManagerCallbacks* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    g_return_if_fail(RP_IS_FILTER_MANAGER_CALLBACKS(self));
    RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->encode_data(self, data, end_stream);
}

void
rp_filter_manager_callbacks_set_response_headers(RpFilterManagerCallbacks* self,
                                            evhtp_headers_t* response_headers)
{
    g_return_if_fail(RP_IS_FILTER_MANAGER_CALLBACKS(self));
    RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->set_response_headers(self, response_headers);
}

evhtp_headers_t*
rp_filter_manager_callbacks_request_headers(RpFilterManagerCallbacks* self)
{
    g_return_val_if_fail(RP_IS_FILTER_MANAGER_CALLBACKS(self), NULL);
    return RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->request_headers(self);
}

evhtp_headers_t*
rp_filter_manager_callbacks_response_headers(RpFilterManagerCallbacks* self)
{
    g_return_val_if_fail(RP_IS_FILTER_MANAGER_CALLBACKS(self), NULL);
    return RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->response_headers(self);
}

void
rp_filter_manager_callbacks_end_stream(RpFilterManagerCallbacks* self)
{
    g_return_if_fail(RP_IS_FILTER_MANAGER_CALLBACKS(self));
    RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->end_stream(self);
}

void
rp_filter_manager_callbacks_disarm_request_timeout(RpFilterManagerCallbacks* self)
{
    g_return_if_fail(RP_IS_FILTER_MANAGER_CALLBACKS(self));
    RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->disarm_request_timeout(self);
}

void
rp_filter_manager_callbacks_reset_idle_timer(RpFilterManagerCallbacks* self)
{
    g_return_if_fail(RP_IS_FILTER_MANAGER_CALLBACKS(self));
    RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->reset_idle_timer(self);
}

RpUpstreamStreamFilterCallbacks*
rp_filter_manager_callbacks_upstream_callbacks(RpFilterManagerCallbacks* self)
{
NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER_CALLBACKS(self), NULL);
    return RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->upstream_callbacks(self);
}

RpDownstreamStreamFilterCallbacks*
rp_filter_manager_callbacks_downstream_callbacks(RpFilterManagerCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER_CALLBACKS(self), NULL);
    return RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->downstream_callbacks(self);
}

void
rp_filter_manager_callbacks_on_local_reply(RpFilterManagerCallbacks* self, evhtp_res code)
{
    g_return_if_fail(RP_IS_FILTER_MANAGER_CALLBACKS(self));
    RP_FILTER_MANAGER_CALLBACKS_GET_IFACE(self)->on_local_reply(self, code);
}

struct RpFilterManagerState {
    guint32 m_filter_call_state;

    bool m_decoder_filter_chain_complete : 1;

    bool m_encoder_filter_chain_complete : 1;

    bool m_observed_decode_end_stream : 1;
    bool m_observed_encode_end_stream : 1;

    bool m_is_head_request : 1;
    bool m_non_100_response_headers_encoded : 1;
    bool m_under_on_local_reply : 1;
    bool m_decoder_filter_chain_aborted : 1;
    bool m_encoder_filter_chain_aborted : 1;
    bool m_saw_downstream_reset : 1;

    bool m_destroyed : 1;
    bool m_encoder_filters_streaming;
    bool m_decoder_filters_streaming;

    struct RpCreateChainResult m_create_chain_result;

    RpActiveStreamDecoderFilter* m_latest_data_decoding_filter;
    RpActiveStreamEncoderFilter* m_latest_data_encoding_filter;
};

struct RpFilterManagerState
rp_filter_manager_state_ctor(void)
{
    struct RpFilterManagerState self = {
        .m_filter_call_state = 0,
        .m_decoder_filter_chain_complete = false,
        .m_encoder_filter_chain_complete = false,
        .m_observed_decode_end_stream = false,
        .m_observed_encode_end_stream = false,
        .m_is_head_request = false,
        .m_under_on_local_reply = false,
        .m_decoder_filter_chain_aborted = false,
        .m_encoder_filter_chain_aborted = false,
        .m_encoder_filters_streaming = true,
        .m_decoder_filters_streaming = true,
        .m_destroyed = false,
        .m_create_chain_result = RpCreateChainResult_ctor(),
        .m_latest_data_decoding_filter = NULL,
        .m_latest_data_encoding_filter = NULL
    };
    return self;
}

typedef struct _RpFilterManagerPrivate RpFilterManagerPrivate;
struct _RpFilterManagerPrivate {

    RpFilterManagerCallbacks* m_filter_manager_callbacks;
    RpNetworkConnection* m_connection;
    RpDispatcher* m_dispatcher;

    GList* m_decoder_filters;
    GList* m_encoder_filters;
    GList* m_filters;
    GSList* m_watermark_callbacks;

    evbuf_t* m_buffered_response_data;
    evbuf_t* m_buffered_request_data;

    struct RpFilterManagerState m_state;

    guint32 m_buffer_limit;
    guint32 m_high_watermark_count;

    bool m_proxy_100_continue;
};

enum
{
    PROP_0, // Reserved.
    PROP_FILTER_MANAGER_CALLBACKS,
    PROP_CONNECTION,
    PROP_DISPATCHER,
    PROP_PROXY_100_CONTINUE,
    PROP_BUFFER_LIMIT,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void filter_chain_manager_iface_init(RpFilterChainManagerInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpFilterManager, rp_filter_manager, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpFilterManager)
    G_IMPLEMENT_INTERFACE(RP_TYPE_FILTER_CHAIN_MANAGER, filter_chain_manager_iface_init)
)

#define PRIV(obj) \
    ((RpFilterManagerPrivate*) rp_filter_manager_get_instance_private(RP_FILTER_MANAGER(obj)))

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_FILTER_MANAGER_CALLBACKS:
            g_value_set_object(value, PRIV(obj)->m_filter_manager_callbacks);
            break;
        case PROP_CONNECTION:
            g_value_set_object(value, PRIV(obj)->m_connection);
            break;
        case PROP_DISPATCHER:
            g_value_set_pointer(value, PRIV(obj)->m_dispatcher);
            break;
        case PROP_PROXY_100_CONTINUE:
            g_value_set_boolean(value, PRIV(obj)->m_proxy_100_continue);
            break;
        case PROP_BUFFER_LIMIT:
            g_value_set_uint(value, PRIV(obj)->m_buffer_limit);
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
        case PROP_FILTER_MANAGER_CALLBACKS:
            PRIV(obj)->m_filter_manager_callbacks = g_value_get_object(value);
            break;
        case PROP_CONNECTION:
            PRIV(obj)->m_connection = g_value_get_object(value);
            break;
        case PROP_DISPATCHER:
            PRIV(obj)->m_dispatcher = g_value_get_pointer(value);
            break;
        case PROP_PROXY_100_CONTINUE:
            PRIV(obj)->m_proxy_100_continue = g_value_get_boolean(value);
            break;
        case PROP_BUFFER_LIMIT:
            PRIV(obj)->m_buffer_limit = g_value_get_uint(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
dispose(GObject* object)
{
    NOISY_MSG_("(%p)", object);
    G_OBJECT_CLASS(rp_filter_manager_parent_class)->dispose(object);
}

static void
apply_filter_factory_cb_i(RpFilterChainManager* self, const struct RpFilterContext* context, RpFilterFactoryCb* factory)
{
    NOISY_MSG_("(%p, %p, %p(%p))", self, context, factory, factory->m_free_func);
    RpFilterManager* me = RP_FILTER_MANAGER(self);
    g_autoptr(RpFilterChainFactoryCallbacksImpl) callbacks = rp_filter_chain_factory_callbacks_impl_new(me, context);
    factory->m_cb(factory, RP_FILTER_CHAIN_FACTORY_CALLBACKS(callbacks));
}

static void
filter_chain_manager_iface_init(RpFilterChainManagerInterface* iface)
{
    LOGD("(%p)", iface);
    iface->apply_filter_factory_cb = apply_filter_factory_cb_i;
}

OVERRIDE RpFilterManagerCallbacks*
filter_manager_callbacks(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_filter_manager_callbacks;
}

static void
rp_filter_manager_class_init(RpFilterManagerClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    klass->filter_manager_callbacks = filter_manager_callbacks;

    obj_properties[PROP_FILTER_MANAGER_CALLBACKS] = g_param_spec_object("filter-manager-callbacks",
                                                    "Filter manager callbacks",
                                                    "Filter Manager Callbacks Instance",
                                                    RP_TYPE_FILTER_MANAGER_CALLBACKS,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_CONNECTION] = g_param_spec_object("connection",
                                                    "Connection",
                                                    "NetworkConnection Instance",
                                                    RP_TYPE_NETWORK_CONNECTION,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_DISPATCHER] = g_param_spec_pointer("dispatcher",
                                                    "Dispatcher",
                                                    "Dispatcher Instance (evhtp_t)",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_PROXY_100_CONTINUE] = g_param_spec_boolean("proxy-100-continue",
                                                    "Proxy 100 continue",
                                                    "Proxy 100 Continue Flag",
                                                    false,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_BUFFER_LIMIT] = g_param_spec_uint("buffer-limit",
                                                    "Buffer limit",
                                                    "Buffer Limit",
                                                    0,
                                                    G_MAXUINT32,
                                                    0,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_filter_manager_init(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    RpFilterManagerPrivate* me = PRIV(self);
    me->m_state = rp_filter_manager_state_ctor();
    me->m_buffer_limit = 0;
    me->m_high_watermark_count = 0;
}

static inline void
initialize_filter_chain(RpFilterManagerPrivate* me)
{
    NOISY_MSG_("(%p)", me);

    for (GList* itr = me->m_decoder_filters; itr; itr = itr->next)
    {
        rp_active_stream_decoder_set_entry(RP_ACTIVE_STREAM_DECODER_FILTER(itr->data), itr);
    }
    for (GList* itr = me->m_encoder_filters; itr; itr = itr->next)
    {
        rp_active_stream_encoder_set_entry(RP_ACTIVE_STREAM_ENCODER_FILTER(itr->data), itr);
    }
}

struct RpCreateChainResult
rp_filter_manager_create_filter_chain(RpFilterManager* self,
                                    RpFilterChainFactory* filter_chain_factory)
{
    LOGD("(%p, %p)", self, filter_chain_factory);

    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), RpCreateChainResult_ctor());

    RpFilterManagerPrivate* me = PRIV(self);
    if (RpCreateChainResult_created(&me->m_state.m_create_chain_result))
    {
        NOISY_MSG_("already created");
        return me->m_state.m_create_chain_result;
    }

    //TODO...

#if 0
    RpDownstreamStreamFilterCallbacks* downstream_callbacks =
        rp_filter_manager_callbacks_downstream_callbacks(me->m_filter_manager_callbacks);
#endif//0

    //TODO...

    RpUpgradeResult_e upgrade = RpUpgradeResult_UpgradeUnneeded;

    //TODO...
    bool created = rp_filter_chain_factory_create_filter_chain(filter_chain_factory, RP_FILTER_CHAIN_MANAGER(self));
    me->m_state.m_create_chain_result = RpCreateChainResult_ctor_(created, upgrade);
    initialize_filter_chain(me);
    return me->m_state.m_create_chain_result;
}

GList**
rp_filter_manager_get_decoder_filters(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), NULL);
    return &PRIV(self)->m_decoder_filters;
}

GList**
rp_filter_manager_get_encoder_filters(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), NULL);
    return &PRIV(self)->m_encoder_filters;
}

GList**
rp_filter_manager_get_filters(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), NULL);
    return &PRIV(self)->m_filters;
}

void
rp_filter_manager_on_stream_complete(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    for (GList* itr = PRIV(self)->m_filters; itr; itr = itr->next)
    {
        RpStreamFilterBase* filter = itr->data;
        rp_stream_filter_base_on_stream_complete(filter);
    }
}

void
rp_filter_manager_destroy_filters(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    RpFilterManagerPrivate* me = PRIV(self);
    me->m_state.m_destroyed = true;
    for (GList* itr = me->m_filters; itr; itr = itr->next)
    {
        RpStreamFilterBase* filter = itr->data;
        rp_stream_filter_base_on_destroy(filter);
    }
}

bool
rp_filter_manager_stop_decoder_filter_chain(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), false);
    return PRIV(self)->m_state.m_decoder_filter_chain_aborted;
}

bool
rp_filter_manager_stop_encoder_filter_chain(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), false);
    return PRIV(self)->m_state.m_encoder_filter_chain_aborted;
}

bool
rp_filter_manager_is_terminal_decoder_filter(RpFilterManager* self, RpActiveStreamDecoderFilter* filter)
{
    NOISY_MSG_("(%p, %p)", self, filter);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), false);
    RpFilterManagerPrivate* me = PRIV(self);
    return me->m_decoder_filters && g_list_last(me->m_decoder_filters)->data == (gpointer)filter;
}

evbuf_t*
rp_filter_manager_get_buffered_request_data(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), NULL);
    return PRIV(self)->m_buffered_request_data;
}

void
rp_filter_manager_set_buffered_request_data(RpFilterManager* self, evbuf_t* buffer)
{
    NOISY_MSG_("(%p, %p(%zu))", self, buffer, evbuffer_get_length(buffer));
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    PRIV(self)->m_buffered_request_data = buffer;
}

evbuf_t*
rp_filter_manager_get_buffered_response_data(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), NULL);
    return PRIV(self)->m_buffered_response_data;
}

void
rp_filter_manager_set_buffered_response_data(RpFilterManager* self, evbuf_t* buffer)
{
    NOISY_MSG_("(%p, %p(%zu))", self, buffer, evbuffer_get_length(buffer));
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    PRIV(self)->m_buffered_response_data = buffer;
}

static GList*
common_encode_prefix(RpFilterManager* self, RpActiveStreamEncoderFilter* filter, bool end_stream, RpFilterIterationStartState_e filter_iteration_start_state)
{
    NOISY_MSG_("(%p, %p, %u, %d)", self, filter, end_stream, filter_iteration_start_state);
    RpFilterManagerPrivate* me = PRIV(self);
    if (!filter)
    {
        if (end_stream)
        {
            me->m_state.m_observed_encode_end_stream = true;
            if (!rp_filter_manager_callbacks_is_half_close_enabled(me->m_filter_manager_callbacks))
            {
                me->m_state.m_decoder_filter_chain_aborted = true;
            }
        }
        return me->m_encoder_filters;
    }

    GList* entry = rp_active_stream_encoder_get_entry(filter);
    if (filter_iteration_start_state == RpFilterIterationStartState_CanStartFromCurrent &&
        rp_active_stream_filter_base_iterate_from_current_filter(RP_ACTIVE_STREAM_FILTER_BASE(filter)))
    {
        NOISY_MSG_("returning entry %p", entry);
        return entry;
    }
    NOISY_MSG_("returning entry->next %p", entry->next);
    return entry->next;
}

static GList*
common_decode_prefix(RpFilterManager* self, RpActiveStreamDecoderFilter* filter, RpFilterIterationStartState_e filter_iteration_start_state)
{
    NOISY_MSG_("(%p, %p, %d)", self, filter, filter_iteration_start_state);
    if (!filter)
    {
        NOISY_MSG_("begin");
        return PRIV(self)->m_decoder_filters;
    }
    if (filter_iteration_start_state == RpFilterIterationStartState_CanStartFromCurrent &&
        rp_active_stream_filter_base_iterate_from_current_filter(RP_ACTIVE_STREAM_FILTER_BASE(filter)))
    {
        NOISY_MSG_("current");
        return rp_active_stream_decoder_get_entry(filter);
    }
    return rp_active_stream_decoder_get_entry(filter)->next;
}

static inline bool
handle_data_if_stop_all(RpFilterManager* self, RpActiveStreamFilterBase* filter, evbuf_t* data, bool* filter_streaming)
{
    NOISY_MSG_("(%p, %p, %p(%zu), %p)", self, filter, data, data ? evbuffer_get_length(data) : 0, filter_streaming);
    if (rp_active_stream_filter_base_stopped_all(filter))
    {
        *filter_streaming =
            rp_active_stream_filter_base_get_iteration_state(filter) == RpIterationState_StopAllWatermark;
        rp_active_stream_filter_base_common_handle_buffer_data(filter, data);
        NOISY_MSG_("yep");
        return true;
    }
    NOISY_MSG_("nope");
    return false;
}

static inline RpActiveStreamFilterBase*
record_latest_data_filter(GList* current_filter, RpActiveStreamFilterBase* latest_filter, GList* filters)
{
    NOISY_MSG_("(%p, %p, %p)", current_filter, latest_filter, filters);

    if (latest_filter == NULL)
    {
        return RP_ACTIVE_STREAM_FILTER_BASE(current_filter->data);
    }

    if (current_filter != filters/*filters.begin()???*/ && latest_filter == RP_ACTIVE_STREAM_FILTER_BASE(current_filter->prev->data))
    {
        return RP_ACTIVE_STREAM_FILTER_BASE(current_filter->data);
    }

    return latest_filter;
}

static inline void
disarm_request_timeout(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    rp_filter_manager_callbacks_disarm_request_timeout(PRIV(self)->m_filter_manager_callbacks);
}

static void
maybe_continue_decoding(RpFilterManager* self, GList* continue_data_entry)
{
    NOISY_MSG_("(%p, %p)", self, continue_data_entry);
    if (continue_data_entry/*!= decoder_filters_.end()*/)
    {
        RpActiveStreamFilterBase* filter = continue_data_entry->data;
        rp_active_stream_filter_base_set_iteration_state(filter, RpIterationState_StopSingleIteration);
        rp_stream_decoder_filter_callbacks_continue_decoding(RP_STREAM_DECODER_FILTER_CALLBACKS(filter));
    }
}

static void
maybe_continue_encoding(RpFilterManager* self, GList* continue_data_entry)
{
    NOISY_MSG_("(%p, %p)", self, continue_data_entry);
    if (continue_data_entry)
    {
        RpActiveStreamFilterBase* filter = continue_data_entry->data;
        rp_active_stream_filter_base_set_iteration_state(filter, RpIterationState_StopSingleIteration);
        rp_stream_encoder_filter_callbacks_continue_encoding(RP_STREAM_ENCODER_FILTER_CALLBACKS(filter));
    }
}

static void
maybe_end_decode(RpFilterManager* self, bool terminal_filter_decoded_end_stream)
{
    NOISY_MSG_("(%p, %u)", self, terminal_filter_decoded_end_stream);
    if (terminal_filter_decoded_end_stream)
    {
        RpFilterManagerPrivate* me = PRIV(self);
        me->m_state.m_decoder_filter_chain_complete = true;
        if (rp_filter_manager_callbacks_is_half_close_enabled(me->m_filter_manager_callbacks) &&
            !rp_filter_manager_stop_decoder_filter_chain(self))
        {
            NOISY_MSG_("checking close");
            rp_filter_manager_check_and_close_stream_if_fully_closed(self);
        }
    }
}

static void
maybe_end_encode(RpFilterManager* self, bool end_stream)
{
    NOISY_MSG_("(%p, %u)", self, end_stream);
    if (end_stream)
    {
        RpFilterManagerPrivate* me = PRIV(self);
        me->m_state.m_encoder_filter_chain_complete = true;
        if (rp_filter_manager_callbacks_is_half_close_enabled(me->m_filter_manager_callbacks))
        {
            NOISY_MSG_("calling rp_filter_manager_check_and_close_stream_if_fully_closed(%p)", self);
            rp_filter_manager_check_and_close_stream_if_fully_closed(self);
        }
        else
        {
            NOISY_MSG_("calling rp_filter_manager_callbacks_end_stream(%p)", me->m_filter_manager_callbacks);
            rp_filter_manager_callbacks_end_stream(me->m_filter_manager_callbacks);
        }
    }
}

static void
decode_headers(RpFilterManager* self, RpActiveStreamDecoderFilter* filter, evhtp_headers_t* request_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %p, %u)", self, filter, request_headers, end_stream);

    RpFilterManagerPrivate* me = PRIV(self);
    struct RpFilterManagerState* state = &me->m_state;

    GList* entry = common_decode_prefix(self, filter, RpFilterIterationStartState_AlwaysStartFromNext);
    GList* continue_data_entry = NULL/*decoder_filters_.end()????*/;
    bool terminal_filter_decoded_end_stream = false;

    for (; entry; entry = entry->next)
    {
        state->m_filter_call_state |= RpFilterCallState_DecodeHeaders;

        RpActiveStreamDecoderFilter* f = RP_ACTIVE_STREAM_DECODER_FILTER(entry->data);
        RpActiveStreamFilterBase* fb = RP_ACTIVE_STREAM_FILTER_BASE(f);
        bool end_stream_ = (end_stream && continue_data_entry == NULL/*decoder_filters_.end()???*/);
        rp_active_stream_filter_base_set_end_stream(fb, end_stream_);
        if (end_stream_)
        {
            state->m_filter_call_state |= RpFilterCallState_EndOfStream;
        }
        RpFilterHeadersStatus_e status = rp_active_stream_decoder_decode_headers(f, request_headers, end_stream_);
        state->m_filter_call_state &= ~RpFilterCallState_DecodeHeaders;
        if (end_stream_)
        {
            state->m_filter_call_state &= ~RpFilterCallState_EndOfStream;
        }
        if (state->m_decoder_filter_chain_aborted)
        {
//TODO: executeLocalReplyIfPrepared();
            status = RpFilterHeadersStatus_StopIteration;
        }

        rp_active_stream_filter_base_set_processed_headers(fb, true);

        bool continue_iteration = rp_active_stream_filter_base_common_handle_after_headers_callback(fb, status, &end_stream);

        if (rp_active_stream_filter_base_get_end_stream(fb))
        {
            rp_stream_decoder_filter_decode_complete(rp_active_stream_decoder_handle(f));
        }

        if (rp_filter_manager_stop_decoder_filter_chain(self) && entry->next)
        {
            NOISY_MSG_("returning");
            maybe_continue_decoding(self, continue_data_entry);
            return;
        }

//...

        if (!continue_iteration && entry->next/*std::next(entry) != decoder_filters_.end()???*/)
        {
            NOISY_MSG_("returning");
            maybe_continue_decoding(self, continue_data_entry);
            return;
        }

        if (end_stream && me->m_buffered_request_data && continue_data_entry == NULL/*decoder_filters_.end()???*/)
        {
            continue_data_entry = entry;
        }
#if 0
        bool no_body_was_injected = continue_data_entry == NULL/*decoder_filters_.end()???*/;
#endif//0
        terminal_filter_decoded_end_stream = (entry->next == NULL/*std::next(entry) == decoder_filters_.end()???*/ &&
            rp_active_stream_filter_base_get_end_stream(RP_ACTIVE_STREAM_FILTER_BASE(entry->data)));
    }

    maybe_continue_decoding(self, continue_data_entry);

    if (end_stream)
    {
        disarm_request_timeout(self);
    }
    maybe_end_decode(self, terminal_filter_decoded_end_stream);
}

static void
decode_data(RpFilterManager* self, RpActiveStreamDecoderFilter* filter, evbuf_t* data,
            bool end_stream, RpFilterIterationStartState_e filter_iteration_start_state)
{
    NOISY_MSG_("(%p, %p, %p(%zu), %u, %d)",
        self, filter, data, evbuffer_get_length(data), end_stream, filter_iteration_start_state);
    //TODO...ScopeTrackerScopeState scope(this, dispatcher_);
    RpFilterManagerPrivate* me = PRIV(self);
    RpFilterManagerCallbacks* filter_manager_callbacks = me->m_filter_manager_callbacks;
    struct RpFilterManagerState* state = &PRIV(self)->m_state;
    rp_filter_manager_callbacks_reset_idle_timer(filter_manager_callbacks);

    if (rp_filter_manager_stop_decoder_filter_chain(self))
    {
        NOISY_MSG_("returning");
        return;
    }

    GList* trailers_added_entry = NULL;
    bool trailers_exists_at_start = rp_filter_manager_callbacks_request_trailers(filter_manager_callbacks) != NULL/*.has_value()???*/;
    GList* entry = common_decode_prefix(self, filter, filter_iteration_start_state);
    bool terminal_filter_decoded_end_stream = false;

    for (; entry; entry = entry->next)
    {
        RpActiveStreamDecoderFilter* f = RP_ACTIVE_STREAM_DECODER_FILTER(entry->data);
        RpActiveStreamFilterBase* fb = RP_ACTIVE_STREAM_FILTER_BASE(f);

        //TODO????ENVOY_EXECUTION_SCOPE(...)
        if (handle_data_if_stop_all(self, fb, data, &state->m_decoder_filters_streaming))
        {
            NOISY_MSG_("returning");
            return;
        }
        if (rp_active_stream_filter_base_get_end_stream(fb))
        {
            NOISY_MSG_("returning");
            return;
        }

        if (end_stream)
        {
            NOISY_MSG_("end stream");
            state->m_filter_call_state |= RpFilterCallState_EndOfStream;
        }

        state->m_latest_data_decoding_filter = RP_ACTIVE_STREAM_DECODER_FILTER(
            record_latest_data_filter(entry, RP_ACTIVE_STREAM_FILTER_BASE(state->m_latest_data_decoding_filter), me->m_decoder_filters)
        );

        state->m_filter_call_state |= RpFilterCallState_DecodeData;
        bool end_stream_ = (end_stream && !rp_filter_manager_callbacks_request_trailers(filter_manager_callbacks));
        rp_active_stream_filter_base_set_end_stream(fb, end_stream_);

        RpStreamDecoderFilter* handle = rp_active_stream_decoder_handle(f);
        RpFilterDataStatus_e status = rp_stream_decoder_filter_decode_data(handle, data, end_stream_);
        if (end_stream_)
        {
            rp_stream_decoder_filter_decode_complete(handle);
        }
        state->m_filter_call_state &= ~RpFilterCallState_DecodeData;
        if (end_stream)
        {
            state->m_filter_call_state &= ~RpFilterCallState_EndOfStream;
        }
        if (state->m_decoder_filter_chain_aborted)
        {
            //TODO: executeLocalReplyIfPrepared();
            return;
        }

        //TODO: processNewlyAddedMetadata();

        if (!trailers_exists_at_start &&
            rp_filter_manager_callbacks_request_trailers(filter_manager_callbacks) &&
            trailers_added_entry == NULL/*decoder_filters_.end()???*/)
        {
            end_stream = false;
            trailers_added_entry = entry;
        }

        terminal_filter_decoded_end_stream = end_stream && !entry->next/*std::next(entry) == decoder_filters_.end()*/;

        if (!rp_active_stream_filter_base_common_handle_after_data_callback(fb, status, data, &state->m_decoder_filters_streaming) &&
            entry->next/*std::next(entry) != decoder_filters_.end()*/)
        {
            break;
        }
    }

    if (trailers_added_entry/*!=decoder_filters_.end()*/)
    {
        rp_stream_decoder_filter_decode_trailers(RP_STREAM_DECODER_FILTER(self),
            rp_filter_manager_callbacks_request_trailers(filter_manager_callbacks));
    }

    if (end_stream)
    {
        disarm_request_timeout(self);
    }
    maybe_end_decode(self, terminal_filter_decoded_end_stream);
}

static void
decode_trailers(RpFilterManager* self, RpActiveStreamDecoderFilter* filter, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p, %p)", self, filter, trailers);

    if (rp_filter_manager_stop_decoder_filter_chain(self))
    {
        return;
    }

    struct RpFilterManagerState* state = &PRIV(self)->m_state;
    GList* entry = common_decode_prefix(self, filter, RpFilterIterationStartState_CanStartFromCurrent);
    bool terminal_filter_reached = false;

    for (; entry; entry = entry->next)
    {
        RpActiveStreamDecoderFilter* f = RP_ACTIVE_STREAM_DECODER_FILTER(entry->data);
        RpActiveStreamFilterBase* fb = RP_ACTIVE_STREAM_FILTER_BASE(f);
        if (rp_active_stream_filter_base_stopped_all(fb))
        {
            NOISY_MSG_("returning");
            return;
        }
        state->m_filter_call_state |= RpFilterCallState_DecodeTrailers;
        RpStreamDecoderFilter* handle = rp_active_stream_decoder_handle(f);
        RpFilterTrailerStatus_e status = rp_stream_decoder_filter_decode_trailers(handle, trailers);
        rp_active_stream_filter_base_set_end_stream(fb, true);
        state->m_filter_call_state &= ~RpFilterCallState_DecodeTrailers;
        if (state->m_decoder_filter_chain_aborted)
        {
            //TODO: executeLocalReplyIfPrepared();
            status = RpFilterTrailerStatus_StopIteration;
        }

        //TODO: processNewlyAddedMetadata();
        terminal_filter_reached = entry->next == NULL/*std::next(entry) == decoder_filters_.end()*/;

        if (!rp_active_stream_filter_base_common_handle_after_trailers_callback(fb, status))
        {
            NOISY_MSG_("returning");
            return;
        }
    }

    disarm_request_timeout(self);
    maybe_end_decode(self, terminal_filter_reached);
}

static void
encode_headers(RpFilterManager* self, RpActiveStreamEncoderFilter* filter, evhtp_headers_t* response_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %p, %u)", self, filter, response_headers, end_stream);

    RpFilterManagerPrivate* me = PRIV(self);
    RpFilterManagerCallbacks* filter_manager_callbacks = me->m_filter_manager_callbacks;
    struct RpFilterManagerState* state = &me->m_state;
    rp_filter_manager_callbacks_reset_idle_timer(filter_manager_callbacks);
    disarm_request_timeout(self);

    GList* entry = common_encode_prefix(self, filter, end_stream, RpFilterIterationStartState_AlwaysStartFromNext);
    GList* continue_data_entry = NULL;

    for (; entry; entry = entry->next)
    {
        RpActiveStreamEncoderFilter* f = RP_ACTIVE_STREAM_ENCODER_FILTER(entry->data);
        RpActiveStreamFilterBase* fb = RP_ACTIVE_STREAM_FILTER_BASE(f);
        state->m_filter_call_state |= RpFilterCallState_EncodeHeaders;
        bool end_stream_ = (end_stream && !continue_data_entry);
        rp_active_stream_filter_base_set_end_stream(fb, end_stream_);
        if (end_stream_)
        {
            state->m_filter_call_state |= RpFilterCallState_EndOfStream;
        }
        RpStreamEncoderFilter* handle = rp_active_stream_encoder_handle(f);
        RpFilterHeadersStatus_e status = rp_stream_encoder_filter_encode_headers(handle, response_headers, end_stream_);
        if (state->m_encoder_filter_chain_aborted)
        {
            status = RpFilterHeadersStatus_StopIteration;
        }

        state->m_filter_call_state &= ~RpFilterCallState_EncodeHeaders;
        if (end_stream_)
        {
            state->m_filter_call_state &= ~RpFilterCallState_EndOfStream;
        }

        rp_active_stream_filter_base_set_processed_headers(fb, true);

        bool continue_iteration = rp_active_stream_filter_base_common_handle_after_headers_callback(fb, status, &end_stream);

        if (end_stream_)
        {
            rp_stream_encoder_filter_encode_complete(handle);
        }

        if (!continue_iteration)
        {
            if (!end_stream_)
            {
                maybe_continue_encoding(self, continue_data_entry);
            }
            return;
        }

        if (end_stream && me->m_buffered_response_data && !continue_data_entry)
        {
            continue_data_entry = entry;
        }
    }

    //TODO: status = HeaderUtility::checkRequiredResponseHeaders(headers)...

    bool modified_end_stream = (end_stream && !continue_data_entry);
    state->m_non_100_response_headers_encoded = true;
NOISY_MSG_("calling rp_filter_manager_callbacks_encode_headers(%p, %p, %u)", filter_manager_callbacks, response_headers, modified_end_stream);
    rp_filter_manager_callbacks_encode_headers(filter_manager_callbacks, response_headers, modified_end_stream);
    if (state->m_saw_downstream_reset)
    {
        return;
    }
    maybe_end_encode(self, modified_end_stream);

    if (!modified_end_stream)
    {
        maybe_continue_encoding(self, continue_data_entry);
    }
}

static void
encode_trailers(RpFilterManager* self, RpActiveStreamEncoderFilter* filter, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p, %p)", self, filter, trailers);

    RpFilterManagerPrivate* me = PRIV(self);
    RpFilterManagerCallbacks* filter_manager_callbacks = me->m_filter_manager_callbacks;
    struct RpFilterManagerState* state = &me->m_state;
    rp_filter_manager_callbacks_reset_idle_timer(filter_manager_callbacks);

    GList* entry = common_encode_prefix(self, filter, true, RpFilterIterationStartState_CanStartFromCurrent);
    for (; entry; entry = entry->next)
    {
        //TODO: ENVOY_EXECUTION_SCOPE(...)
        RpActiveStreamEncoderFilter* f = entry->data;
        RpActiveStreamFilterBase* fb = RP_ACTIVE_STREAM_FILTER_BASE(f);
        if (rp_active_stream_filter_base_stopped_all(fb))
        {
            NOISY_MSG_("returning");
            return;
        }
        state->m_filter_call_state |= RpFilterCallState_EncodeTrailers;
        RpStreamEncoderFilter* handle = rp_active_stream_encoder_handle(f);
        RpFilterTrailerStatus_e status = rp_stream_encoder_filter_encode_trailers(handle, trailers);
        rp_stream_encoder_filter_encode_complete(handle);
        rp_active_stream_filter_base_set_end_stream(fb, true);
        state->m_filter_call_state &= ~RpFilterCallState_EncodeTrailers;
        if (!rp_active_stream_filter_base_common_handle_after_trailers_callback(fb, status))
        {
            return;
        }
    }

    rp_filter_manager_callbacks_encode_trailers(filter_manager_callbacks, trailers);
    if (state->m_saw_downstream_reset)
    {
        return;
    }
    maybe_end_encode(self, true);
}

static void
encode_data(RpFilterManager* self, RpActiveStreamEncoderFilter* filter, evbuf_t* data, bool end_stream, RpFilterIterationStartState_e filter_iteration_start_state)
{
    NOISY_MSG_("(%p, %p, %p(%zu), %u, %d)",
        self, filter, data, data ? evbuffer_get_length(data) : 0, end_stream, filter_iteration_start_state);

    RpFilterManagerPrivate* me = PRIV(self);
    RpFilterManagerCallbacks* filter_manager_callbacks = me->m_filter_manager_callbacks;
    struct RpFilterManagerState* state = &me->m_state;
    rp_filter_manager_callbacks_reset_idle_timer(filter_manager_callbacks);

    GList* entry = common_encode_prefix(self, filter, end_stream, filter_iteration_start_state);
    GList* trailers_added_entry = NULL;

    bool trailers_exists_at_start = rp_filter_manager_callbacks_response_trailers(filter_manager_callbacks) != NULL;
    for (; entry; entry = entry->next)
    {
        RpActiveStreamEncoderFilter* f = RP_ACTIVE_STREAM_ENCODER_FILTER(entry->data);
        RpActiveStreamFilterBase* fb = RP_ACTIVE_STREAM_FILTER_BASE(f);
        //TODO: ENVOY_EXECUTION_SCOPE(...)
        if (handle_data_if_stop_all(self, fb, data, &state->m_encoder_filters_streaming))
        {
            NOISY_MSG_("returning");
            return;
        }
        if (rp_active_stream_filter_base_get_end_stream(fb))
        {
            NOISY_MSG_("returning");
            return;
        }

        state->m_filter_call_state |= RpFilterCallState_EncodeData;
        if (end_stream)
        {
            state->m_filter_call_state |= RpFilterCallState_EndOfStream;
        }

        state->m_latest_data_encoding_filter = RP_ACTIVE_STREAM_ENCODER_FILTER(
            record_latest_data_filter(entry, RP_ACTIVE_STREAM_FILTER_BASE(state->m_latest_data_encoding_filter), me->m_encoder_filters)
        );

        bool end_stream_ = end_stream && !rp_filter_manager_callbacks_response_trailers(filter_manager_callbacks);
        rp_active_stream_filter_base_set_end_stream(fb, end_stream_);
        RpStreamEncoderFilter* handle = rp_active_stream_encoder_handle(f);
        RpFilterDataStatus_e status = rp_stream_encoder_filter_encode_data(handle, data, end_stream_);
        if (state->m_encoder_filter_chain_aborted)
        {
            status = RpFilterDataStatus_StopIterationNoBuffer;
        }
        if (end_stream_)
        {
            rp_stream_encoder_filter_encode_complete(handle);
        }
        state->m_filter_call_state &= ~RpFilterCallState_EncodeData;
        if (end_stream)
        {
            state->m_filter_call_state &= ~RpFilterCallState_EndOfStream;
        }

        if (!trailers_exists_at_start &&
            rp_filter_manager_callbacks_response_trailers(filter_manager_callbacks) &&
            !trailers_added_entry)
        {
            trailers_added_entry = entry;
        }

        if (!rp_active_stream_filter_base_common_handle_after_data_callback(fb, status, data, &state->m_encoder_filters_streaming))
        {
            NOISY_MSG_("returning");
            return;
        }
    }

    bool modified_end_stream = end_stream && !trailers_added_entry;
    rp_filter_manager_callbacks_encode_data(filter_manager_callbacks, data, modified_end_stream);
    if (state->m_saw_downstream_reset)
    {
        return;
    }
    maybe_end_encode(self, modified_end_stream);

    if (trailers_added_entry)
    {
        encode_trailers(self, trailers_added_entry->data, rp_filter_manager_callbacks_response_trailers(filter_manager_callbacks));
    }
}

evhtp_headers_t*
rp_filter_manager_add_encoded_trailers(RpFilterManager* self)
{
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), NULL);
    RpFilterManagerPrivate* me = PRIV(self);
    rp_filter_manager_callbacks_set_response_trailers(me->m_filter_manager_callbacks, evhtp_headers_new());
    return rp_filter_manager_callbacks_response_trailers(me->m_filter_manager_callbacks);
}

void
rp_filter_manager_add_encoded_data(RpFilterManager* self, RpActiveStreamEncoderFilter* filter, evbuf_t* data, bool streaming)
{
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));

    struct RpFilterManagerState* state = &PRIV(self)->m_state;
    if (state->m_filter_call_state == 0 ||
        (state->m_filter_call_state & RpFilterCallState_EncodeHeaders) ||
        (state->m_filter_call_state & RpFilterCallState_EncodeData) ||
        ((state->m_filter_call_state & RpFilterCallState_EncodeTrailers) && !rp_active_stream_filter_base_can_iterate(RP_ACTIVE_STREAM_FILTER_BASE(filter))))
    {
        state->m_encoder_filters_streaming = streaming;
        rp_active_stream_filter_base_common_handle_buffer_data(RP_ACTIVE_STREAM_FILTER_BASE(filter), data);
    }
    else if (state->m_filter_call_state & RpFilterCallState_EncodeTrailers)
    {
        rp_filter_manager_encode_data_(self, filter, data, false, RpFilterIterationStartState_AlwaysStartFromNext);
    }
    else
    {
        //TODO: sendLocalReply(...)...
    }
}

bool
rp_filter_manager_destroyed(RpFilterManager* self)
{
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), true);
    return PRIV(self)->m_state.m_destroyed;
}

void
rp_filter_manager_decode_data_(RpFilterManager* self, RpActiveStreamDecoderFilter* filter, evbuf_t* data,
                                bool end_stream, RpFilterIterationStartState_e filter_iteration_start_state)
{
    NOISY_MSG_("(%p, %p, %p(%zu), %u, %d)",
        self, filter, data, evbuffer_get_length(data), end_stream, filter_iteration_start_state);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    decode_data(self, filter, data, end_stream, filter_iteration_start_state);
}

void
rp_filter_manager_decode_data(RpFilterManager* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuffer_get_length(data), end_stream);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    PRIV(self)->m_state.m_observed_decode_end_stream = end_stream;
    decode_data(self, NULL, data, end_stream, RpFilterIterationStartState_CanStartFromCurrent);
}

void
rp_filter_manager_decode_trailers_(RpFilterManager* self, RpActiveStreamDecoderFilter* filter, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p, %p)", self, filter, trailers);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    decode_trailers(self, filter, trailers);
}

void
rp_filter_manager_decode_trailers(RpFilterManager* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    decode_trailers(self, NULL, trailers);
}

void
rp_filter_manager_add_decoded_data(RpFilterManager* self, RpActiveStreamDecoderFilter* filter, evbuf_t* data, bool streaming)
{
    NOISY_MSG_("(%p, %p, %p(%zu), %u)", self, filter, data, evbuffer_get_length(data), streaming);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    g_return_if_fail(RP_IS_ACTIVE_STREAM_DECODER_FILTER(filter));
    g_return_if_fail(data != NULL);
    RpFilterManagerPrivate* me = PRIV(self);
    struct RpFilterManagerState* state = &me->m_state;
    if (state->m_filter_call_state == 0 ||
        (state->m_filter_call_state & RpFilterCallState_DecodeHeaders) ||
        (state->m_filter_call_state & RpFilterCallState_DecodeData) ||
        ((state->m_filter_call_state & RpFilterCallState_DecodeTrailers) && !rp_active_stream_filter_base_can_iterate(RP_ACTIVE_STREAM_FILTER_BASE(filter))))
    {
        state->m_decoder_filters_streaming = streaming;
        rp_active_stream_filter_base_common_handle_buffer_data(RP_ACTIVE_STREAM_FILTER_BASE(filter), data);
    }
    else if (state->m_filter_call_state & RpFilterCallState_DecodeTrailers)
    {
        rp_filter_manager_decode_data_(self, filter, data, false, RpFilterIterationStartState_AlwaysStartFromNext);
    }
    else
    {
        LOGD("invalid request data");

    }
}

void
rp_filter_manager_decode_headers_(RpFilterManager* self, RpActiveStreamDecoderFilter* filter, evhtp_headers_t* request_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %p, %u)", self, filter, request_headers, end_stream);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    g_return_if_fail(request_headers != NULL);
    decode_headers(self, filter, request_headers, end_stream);
}

void
rp_filter_manager_decode_headers(RpFilterManager* self, evhtp_headers_t* request_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, request_headers, end_stream);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    g_return_if_fail(request_headers != NULL);
    PRIV(self)->m_state.m_observed_decode_end_stream = end_stream;
    decode_headers(self, NULL, request_headers, end_stream);
}

void
rp_filter_manager_encode_headers_(RpFilterManager* self, RpActiveStreamEncoderFilter* filter, evhtp_headers_t* response_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %p, %u)", self, filter, response_headers, end_stream);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    g_return_if_fail(response_headers != NULL);
    encode_headers(self, filter, response_headers, end_stream);
}

void
rp_filter_manager_encode_headers(RpFilterManager* self, evhtp_headers_t* response_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, response_headers, end_stream);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    g_return_if_fail(response_headers != NULL);
    PRIV(self)->m_state.m_observed_encode_end_stream = end_stream;
    encode_headers(self, NULL, response_headers, end_stream);
}

void
rp_filter_manager_encode_data_(RpFilterManager* self, RpActiveStreamEncoderFilter* filter, evbuf_t* data,
                                bool end_stream, RpFilterIterationStartState_e filter_iteration_start_state)
{
    NOISY_MSG_("(%p, %p, %p(%zu), %u, %d)",
        self, filter, data, evbuffer_get_length(data), end_stream, filter_iteration_start_state);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    encode_data(self, filter, data, end_stream, filter_iteration_start_state);
}

void
rp_filter_manager_encode_data(RpFilterManager* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuffer_get_length(data), end_stream);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    PRIV(self)->m_state.m_observed_encode_end_stream = end_stream;
    encode_data(self, NULL, data, end_stream, RpFilterIterationStartState_CanStartFromCurrent);
}

void
rp_filter_manager_encode_trailers_(RpFilterManager* self, RpActiveStreamEncoderFilter* filter, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p, %p)", self, filter, trailers);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    encode_trailers(self, filter, trailers);
}

bool
rp_filter_manager_get_decoder_filter_chain_complete(RpFilterManager* self)
{
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), false);
    return PRIV(self)->m_state.m_decoder_filter_chain_complete;
}

void
rp_filter_manager_set_decoder_filter_chain_aborted(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    PRIV(self)->m_state.m_decoder_filter_chain_aborted = true;
}

void
rp_filter_manager_set_encoder_filter_chain_aborted(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    PRIV(self)->m_state.m_encoder_filter_chain_aborted = true;
}

void
rp_filter_manager_set_encoder_filter_chain_complete(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    PRIV(self)->m_state.m_encoder_filter_chain_complete = true;
}

bool
rp_filter_manager_encoder_filter_chain_complete(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), true);
    return PRIV(self)->m_state.m_encoder_filter_chain_complete;
}

void
rp_filter_manager_set_observed_encode_end_stream(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    PRIV(self)->m_state.m_observed_encode_end_stream = true;
}

bool
rp_filter_manager_get_observed_encode_end_stream(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), false);
    return PRIV(self)->m_state.m_observed_encode_end_stream;
}

bool
rp_filter_manager_get_is_head_request(RpFilterManager* self)
{
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), false);
    return PRIV(self)->m_state.m_is_head_request;
}

guint32
rp_filter_manager_get_filter_call_state(RpFilterManager* self)
{
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), 0);
    return PRIV(self)->m_state.m_filter_call_state;
}

void
rp_filter_manager_set_under_on_local_reply(RpFilterManager* self, bool val)
{
    NOISY_MSG_("(%p, %u)", self, val);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    PRIV(self)->m_state.m_under_on_local_reply = val;
}

bool
rp_filter_manager_get_non_100_response_headers_encoded(RpFilterManager* self)
{
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), false);
    return PRIV(self)->m_state.m_non_100_response_headers_encoded;
}

void
rp_filter_manager_set_non_100_response_headers_encoded(RpFilterManager* self, bool val)
{
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    PRIV(self)->m_state.m_non_100_response_headers_encoded = val;
}

void
rp_filter_manager_set_encoder_filters_streaming(RpFilterManager* self, bool val)
{
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    PRIV(self)->m_state.m_encoder_filters_streaming = val;
}

void
rp_filter_manager_maybe_end_encode_(RpFilterManager* self, bool end_stream)
{
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    maybe_end_decode(self, end_stream);
}

void
rp_filter_manager_request_headers_initialized(RpFilterManager* self)
{
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    RpFilterManagerPrivate* me = PRIV(self);
    evhtp_headers_t* headers = rp_filter_manager_callbacks_request_headers(me->m_filter_manager_callbacks);
    const char* method_value = evhtp_header_find(headers, RpHeaderValues.Method);
    if (method_value && g_ascii_strcasecmp(method_value, RpHeaderValues.MethodValues.Head) == 0)
    {
        me->m_state.m_is_head_request = true;
    }
    //TODO...state.is_grpc_request_ = ...
}

void
rp_filter_manager_set_local_complete(RpFilterManager* self)
{
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    struct RpFilterManagerState* state = &PRIV(self)->m_state;
    state->m_observed_encode_end_stream = true;
    state->m_decoder_filter_chain_aborted = true;
}

bool
rp_filter_manager_decoder_observed_end_stream(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), true);
    return PRIV(self)->m_state.m_observed_decode_end_stream;
}

static inline bool
http_code_utility_is1xx(evhtp_res code)
{
    return code >= 100 && code < 200;
}

static inline bool
http_code_utility_is2xx(evhtp_res code)
{
    return code >= 200 && code < 300;
}

void
rp_filter_manager_check_and_close_stream_if_fully_closed(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);

    RpFilterManagerPrivate* me = PRIV(self);
    struct RpFilterManagerState* state = &me->m_state;
    evhtp_headers_t* response_headers = rp_filter_manager_callbacks_response_headers(me->m_filter_manager_callbacks);
    if (response_headers)
    {
        evhtp_res response_status = http_utility_get_response_status(response_headers);
        bool error_response = !(http_code_utility_is2xx(response_status) || http_code_utility_is1xx(response_status));
        if (error_response && !state->m_decoder_filter_chain_complete)
        {
            state->m_decoder_filter_chain_aborted = true;
        }
    }

    bool downstream_client_sent_end_stream = rp_filter_manager_decoder_observed_end_stream(self);
    bool decoder_filter_chain_paused =
        !state->m_decoder_filter_chain_complete && !state->m_decoder_filter_chain_aborted;
    if (state->m_encoder_filter_chain_complete &&
        downstream_client_sent_end_stream &&
        decoder_filter_chain_paused)
    {
        state->m_decoder_filter_chain_aborted = true;
    }

    if (state->m_encoder_filter_chain_complete &&
        (state->m_decoder_filter_chain_complete || state->m_decoder_filter_chain_aborted))
    {
        NOISY_MSG_("closing stream");
        rp_filter_manager_callbacks_end_stream(me->m_filter_manager_callbacks);
    }
}

void
rp_filter_manager_on_downstream_reset(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    PRIV(self)->m_state.m_saw_downstream_reset = true;
}

bool
rp_filter_manager_saw_downstream_reset(RpFilterManager* self)
{
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), false);
    return PRIV(self)->m_state.m_saw_downstream_reset;
}

void
rp_filter_manager_send_go_away_and_close(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    RpFilterManagerPrivate* me = PRIV(self);
    struct RpFilterManagerState* state = &me->m_state;
    if (state->m_filter_call_state & RpFilterCallState_IsDecodingMask)
    {
        NOISY_MSG_("decoding aborted");
        state->m_decoder_filter_chain_aborted = true;
    }
    else if (state->m_filter_call_state & RpFilterCallState_IsEncodingMask)
    {
        NOISY_MSG_("encoding aborted");
        state->m_encoder_filter_chain_aborted = true;
    }
    rp_filter_manager_callbacks_send_go_away_and_close(me->m_filter_manager_callbacks);
}

RpNetworkConnection*
rp_filter_manager_connection(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), NULL);
    return PRIV(self)->m_connection;
}

RpDispatcher*
rp_filter_manager_dispatcher(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), NULL);
    return PRIV(self)->m_dispatcher;
}

guint32
rp_filter_manager_buffer_limit_(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), 0);
    return PRIV(self)->m_buffer_limit;
}

GSList**
rp_filter_manager_watermark_callbacks_(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), NULL);
    return &PRIV(self)->m_watermark_callbacks;
}

guint32
rp_filter_manager_high_watermark_count_(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(self), 0);
    return PRIV(self)->m_high_watermark_count;
}

void
rp_filter_manager_call_high_watermark_callbacks(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    RpFilterManagerPrivate* me = PRIV(self);
    ++me->m_high_watermark_count;
    for (GSList* entry = me->m_watermark_callbacks; entry; entry = entry->next)
    {
        RpDownstreamWatermarkCallbacks* callbacks = RP_DOWNSTREAM_WATERMARK_CALLBACKS(entry->data);
        rp_downstream_watermark_callbacks_on_above_write_buffer_high_watermark(callbacks);
    }
}

void
rp_filter_manager_call_low_watermark_callbacks(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    RpFilterManagerPrivate* me = PRIV(self);
    --me->m_high_watermark_count;
    for (GSList* entry = me->m_watermark_callbacks; entry; entry = entry->next)
    {
        RpDownstreamWatermarkCallbacks* callbacks = RP_DOWNSTREAM_WATERMARK_CALLBACKS(entry->data);
        rp_downstream_watermark_callbacks_on_below_write_buffer_low_watermark(callbacks);
    }
}

void
rp_filter_manager_reset_stream(RpFilterManager* self, RpStreamResetReason_e reason, const char* transport_failure_reason)
{
    LOGD("(%p, %d, %p(%s))", self, reason, transport_failure_reason, transport_failure_reason);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    RpFilterManagerPrivate* me = PRIV(self);
    if (me->m_state.m_filter_call_state & RpFilterCallState_IsDecodingMask)
    {
        me->m_state.m_decoder_filter_chain_aborted = true;
    }
    else if (me->m_state.m_filter_call_state & RpFilterCallState_IsEncodingMask)
    {
        me->m_state.m_encoder_filter_chain_aborted = true;
    }

    rp_filter_manager_callbacks_reset_stream(me->m_filter_manager_callbacks, reason, transport_failure_reason);
}
