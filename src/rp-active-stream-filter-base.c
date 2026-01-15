/*
 * rp-active-stream-filter-base.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_active_stream_filter_base_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_active_stream_filter_base_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-codec.h"
#include "rp-filter-manager.h"
#include "rp-active-stream-filter-base.h"

typedef struct _RpActiveStreamFilterBasePrivate RpActiveStreamFilterBasePrivate;
struct _RpActiveStreamFilterBasePrivate {

    RpFilterManager* m_parent;

    const struct RpFilterContext* m_filter_context;

    RpIterationState_e m_iteration_state;

    bool m_iterate_from_current_filter : 1;
    bool m_headers_continued : 1;
    bool m_end_stream : 1;
    bool m_processed_headers : 1;

};

enum
{
    PROP_0, // Reserved.
    PROP_PARENT,
    PROP_FILTER_CONTEXT,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void stream_filter_callbacks_iface_init(RpStreamFilterCallbacksInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpActiveStreamFilterBase, rp_active_stream_filter_base, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpActiveStreamFilterBase)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_FILTER_CALLBACKS, stream_filter_callbacks_iface_init)
)

#define PRIV(obj) \
    ((RpActiveStreamFilterBasePrivate*) rp_active_stream_filter_base_get_instance_private(RP_ACTIVE_STREAM_FILTER_BASE(obj)))

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_PARENT:
            g_value_set_object(value, PRIV(obj)->m_parent);
            break;
        case PROP_FILTER_CONTEXT:
            g_value_set_pointer(value, (gpointer)PRIV(obj)->m_filter_context);
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
        case PROP_PARENT:
            PRIV(obj)->m_parent = g_value_get_object(value);
            break;
        case PROP_FILTER_CONTEXT:
            PRIV(obj)->m_filter_context = g_value_get_pointer(value);
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
    G_OBJECT_CLASS(rp_active_stream_filter_base_parent_class)->dispose(obj);
}

static const char*
filter_config_name_i(RpStreamFilterCallbacks* self)
{
    return PRIV(self)->m_filter_context->config_name;
}

static evhtp_headers_t*
request_headers_i(RpStreamFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_filter_manager_callbacks_request_headers(
            rp_filter_manager_filter_manager_callbacks(PRIV(self)->m_parent));
}

static evhtp_headers_t*
response_headers_i(RpStreamFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_filter_manager_callbacks_response_headers(
            rp_filter_manager_filter_manager_callbacks(PRIV(self)->m_parent));
}

static void
reset_idle_timer_i(RpStreamFilterCallbacks* self)
{
    RpFilterManagerCallbacks* callbacks = rp_filter_manager_filter_manager_callbacks(PRIV(self)->m_parent);
    rp_filter_manager_callbacks_reset_idle_timer(callbacks);
}

static RpRoute*
get_route(RpActiveStreamFilterBasePrivate* me)
{
    NOISY_MSG_("(%p)", me);
    RpDownstreamStreamFilterCallbacks* callbacks = rp_filter_manager_callbacks_downstream_callbacks(
                                                        rp_filter_manager_filter_manager_callbacks(me->m_parent));
    if (callbacks)
    {
        NOISY_MSG_("callbacks %p", callbacks);
        return rp_downstream_stream_filter_callbacks_route(callbacks, NULL);
    }
    return rp_stream_info_route(rp_filter_manager_stream_info(me->m_parent));
}

static RpRoute*
route_i(RpStreamFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return get_route(PRIV(self));
}

static RpStreamInfo*
stream_info_i(RpStreamFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_filter_manager_stream_info(PRIV(self)->m_parent);
}

static guint64
stream_id_i(RpStreamFilterCallbacks* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return 0;
}

static RpNetworkConnection*
connection_i(RpStreamFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_filter_manager_connection(PRIV(self)->m_parent);
}

static RpDispatcher*
dispatcher_i(RpStreamFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_filter_manager_dispatcher(PRIV(self)->m_parent);
}

static RpClusterInfoConstSharedPtr
cluster_info_i(RpStreamFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_filter_manager_callbacks_cluster_info(
            rp_filter_manager_filter_manager_callbacks(PRIV(self)->m_parent));
}

static RpDownstreamStreamFilterCallbacks*
downstream_callbacks_i(RpStreamFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_filter_manager_callbacks_downstream_callbacks(
            rp_filter_manager_filter_manager_callbacks(PRIV(self)->m_parent));
}

static RpUpstreamStreamFilterCallbacks*
upstream_callbacks_i(RpStreamFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_filter_manager_callbacks_upstream_callbacks(
            rp_filter_manager_filter_manager_callbacks(PRIV(self)->m_parent));
}

static evhtp_headers_t*
response_trailers_i(RpStreamFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_filter_manager_callbacks_response_trailers(
            rp_filter_manager_filter_manager_callbacks(PRIV(self)->m_parent));
}

static void
reset_stream_i(RpStreamFilterCallbacks* self, RpStreamResetReason_e reset_reason, const char* transport_failure_reason)
{
    NOISY_MSG_("(%p, %d, %p(%s))", self, reset_reason, transport_failure_reason, transport_failure_reason);
    rp_filter_manager_reset_stream(PRIV(self)->m_parent, reset_reason, transport_failure_reason);
}

static void
stream_filter_callbacks_iface_init(RpStreamFilterCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->filter_config_name = filter_config_name_i;
    iface->request_headers = request_headers_i;
    iface->response_headers = response_headers_i;
    iface->reset_idle_timer = reset_idle_timer_i;
    iface->route = route_i;
    iface->stream_info = stream_info_i;
    iface->stream_id = stream_id_i;
    iface->connection = connection_i;
    iface->dispatcher = dispatcher_i;
    iface->upstream_callbacks = upstream_callbacks_i;
    iface->cluster_info = cluster_info_i;
    iface->downstream_callbacks = downstream_callbacks_i;
    iface->response_trailers = response_trailers_i;
    iface->reset_stream = reset_stream_i;
}

static void
rp_active_stream_filter_base_class_init(RpActiveStreamFilterBaseClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_PARENT] = g_param_spec_object("parent",
                                                    "Parent",
                                                    "Parent Filter Manager Instance",
                                                    RP_TYPE_FILTER_MANAGER,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_FILTER_CONTEXT] = g_param_spec_pointer("filter-context",
                                                    "Filter context",
                                                    "Filter Context",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_active_stream_filter_base_init(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    PRIV(self)->m_iterate_from_current_filter = false;
    PRIV(self)->m_iteration_state = RpIterationState_Continue;
}

bool
rp_active_stream_filter_base_iterate_from_current_filter(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_ACTIVE_STREAM_FILTER_BASE(self), false);
    return PRIV(self)->m_iterate_from_current_filter;
}

bool
rp_active_stream_filter_base_get_headers_continued(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_ACTIVE_STREAM_FILTER_BASE(self), false);
    return PRIV(self)->m_headers_continued;
}

void
rp_active_stream_filter_base_set_headers_continued(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_if_fail(RP_IS_ACTIVE_STREAM_FILTER_BASE(self));
    PRIV(self)->m_headers_continued = true;
}

bool
rp_active_stream_filter_base_can_iterate(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_ACTIVE_STREAM_FILTER_BASE(self), false);
    return PRIV(self)->m_iteration_state == RpIterationState_Continue;
}

void
rp_active_stream_filter_base_common_handle_buffer_data(RpActiveStreamFilterBase* self,
                                                        evbuf_t* provided_data)
{
    NOISY_MSG_("(%p, %p(%zu))", self, provided_data, evbuf_length(provided_data));
    g_return_if_fail(RP_IS_ACTIVE_STREAM_FILTER_BASE(self));
    g_return_if_fail(provided_data != NULL);
    evbuf_t* buffered_data = rp_active_stream_filter_base_buffered_data(self);
    if (buffered_data != provided_data)
    {
        if (!buffered_data)
        {
            NOISY_MSG_("creating buffered_data buffer");
            buffered_data = rp_active_stream_filter_base_create_buffer(self);
        }
        evbuffer_add_buffer(buffered_data, provided_data);
    }
}

static void
allow_iteration(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    g_assert(PRIV(self)->m_iteration_state != RpIterationState_Continue);
    PRIV(self)->m_iteration_state = RpIterationState_Continue;
}

void
rp_active_stream_filter_base_common_continue(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_if_fail(RP_IS_ACTIVE_STREAM_FILTER_BASE(self));
    if (!rp_active_stream_filter_base_can_continue(self))
    {
        NOISY_MSG_("returning");
        return;
    }

    RpActiveStreamFilterBasePrivate* me = PRIV(self);

    //TODO: ScopeTrackedObjectStack encapsulated_object;
    //...

    if (rp_active_stream_filter_base_stopped_all(self))
    {
        me->m_iterate_from_current_filter = true;
    }
    allow_iteration(self);

    //TODO: if (has1xxHeaders())...

    if (!me->m_headers_continued)
    {
        me->m_headers_continued = true;
        rp_active_stream_filter_base_do_headers(self,
            rp_active_stream_filter_base_observed_end_stream(self) &&
            !rp_active_stream_filter_base_buffered_data(self) &&
            !rp_active_stream_filter_base_has_trailers(self));
    }

    //TODO: doMetadata();

    bool had_trailers_before_data = rp_active_stream_filter_base_has_trailers(self);
    if (rp_active_stream_filter_base_buffered_data(self))
    {
        rp_active_stream_filter_base_do_data(self,
            rp_active_stream_filter_base_observed_end_stream(self) &&
            !had_trailers_before_data);
    }

    if (had_trailers_before_data)
    {
        rp_active_stream_filter_base_do_trailers(self);
    }

    me->m_iterate_from_current_filter = false;
}

bool
rp_active_stream_filter_base_common_handle_after_data_callback(RpActiveStreamFilterBase* self,
                                                                RpFilterDataStatus_e status,
                                                                evbuf_t* provided_data,
                                                                bool* buffer_was_streaming)
{
    NOISY_MSG_("(%p, %d, %p(%zu), %p)",
        self, status, provided_data, evbuf_length(provided_data), buffer_was_streaming);

    g_return_val_if_fail(RP_IS_ACTIVE_STREAM_FILTER_BASE(self), false);

    RpActiveStreamFilterBasePrivate* me = PRIV(self);

    if (status == RpFilterDataStatus_Continue)
    {
        NOISY_MSG_("RpFilterDataStatus_Continue");
        if (me->m_iteration_state == RpIterationState_StopSingleIteration)
        {
            rp_active_stream_filter_base_common_handle_buffer_data(self, provided_data);
            rp_active_stream_filter_base_common_continue(self);
            NOISY_MSG_("returning false");
            return false;
        }
        else
        {
            g_assert(me->m_headers_continued);
        }
    }
    else
    {
        me->m_iteration_state = RpIterationState_StopSingleIteration;
        if (status == RpFilterDataStatus_StopIterationAndBuffer ||
            status == RpFilterDataStatus_StopIterationAndWatermark)
        {
            NOISY_MSG_("%s", status == RpFilterDataStatus_StopIterationAndBuffer ? "RpFilterDataStatus_StopIterationAndBuffer" : "RpFilterDataStatus_StopIterationAndWatermark");
            *buffer_was_streaming = status == RpFilterDataStatus_StopIterationAndWatermark;
            rp_active_stream_filter_base_common_handle_buffer_data(self, provided_data);
        }
        else if (rp_active_stream_filter_base_observed_end_stream(self) &&
            !rp_active_stream_filter_base_has_trailers(self) &&
            !rp_active_stream_filter_base_buffered_data(self) &&
            !rp_filter_manager_destroyed(me->m_parent))
        {
            g_assert(me->m_end_stream);
            rp_active_stream_filter_base_create_buffer(self);
        }

        NOISY_MSG_("returning false");
        return false;
    }

    NOISY_MSG_("returning true");
    return true;
}

bool
rp_active_stream_filter_base_stopped_all(RpActiveStreamFilterBase* self)
{
    NOISY_MSG_("(%p)", self);
    g_return_val_if_fail(RP_IS_ACTIVE_STREAM_FILTER_BASE(self), false);
    return PRIV(self)->m_iteration_state == RpIterationState_StopAllBuffer ||
            PRIV(self)->m_iteration_state == RpIterationState_StopAllWatermark;
}

bool
rp_active_stream_filter_base_common_handle_after_headers_callback(RpActiveStreamFilterBase* self,
                                                                    RpFilterHeadersStatus_e status,
                                                                    bool* end_stream)
{
    NOISY_MSG_("(%p, %d, %p(%u))", self, status, end_stream, *end_stream);

    g_return_val_if_fail(RP_IS_ACTIVE_STREAM_FILTER_BASE(self), false/*????*/);
    g_return_val_if_fail(end_stream != NULL, false/*????*/);

    switch (status)
    {
        case RpFilterHeadersStatus_StopIteration:
            PRIV(self)->m_iteration_state = RpIterationState_StopSingleIteration;
            break;
        case RpFilterHeadersStatus_StopAllIterationAndBuffer:
            PRIV(self)->m_iteration_state = RpIterationState_StopAllBuffer;
            break;
        case RpFilterHeadersStatus_StopAllIterationAndWatermark:
            PRIV(self)->m_iteration_state = RpIterationState_StopAllWatermark;
            break;
        case RpFilterHeadersStatus_ContinueAndDontEndStream:
            *end_stream = false;
            PRIV(self)->m_headers_continued = true;
            break;
        case RpFilterHeadersStatus_Continue:
            PRIV(self)->m_headers_continued = true;
            break;
    }

//handleMetadataAfterHeadersCallback();

    if (rp_active_stream_filter_base_stopped_all(self) || status == RpFilterHeadersStatus_StopIteration)
    {
        NOISY_MSG_("returning false");
        return false;
    }
    NOISY_MSG_("returning true");
    return true;
}

bool
rp_active_stream_filter_base_common_handle_after_trailers_callback(RpActiveStreamFilterBase* self,
                                                RpFilterTrailerStatus_e status)
{
    NOISY_MSG_("(%p, %d)", self, status);

    g_return_val_if_fail(RP_IS_ACTIVE_STREAM_FILTER_BASE(self), false);

    RpActiveStreamFilterBasePrivate* me = PRIV(self);
    if (status == RpFilterTrailerStatus_Continue)
    {
        if (me->m_iteration_state == RpIterationState_StopSingleIteration)
        {
            rp_active_stream_filter_base_common_continue(self);
            return false;
        }
        else
        {
            g_assert(me->m_headers_continued);
        }
    }
    else if (status == RpFilterTrailerStatus_StopIteration)
    {
        if (rp_active_stream_filter_base_can_iterate(self))
        {
            me->m_iteration_state = RpIterationState_StopSingleIteration;
        }
        return false;
    }

    return true;
}

bool
rp_active_stream_filter_base_get_end_stream(RpActiveStreamFilterBase* self)
{
    g_return_val_if_fail(RP_IS_ACTIVE_STREAM_FILTER_BASE(self), true);
    return PRIV(self)->m_end_stream;
}

void
rp_active_stream_filter_base_set_end_stream(RpActiveStreamFilterBase* self, bool end_stream)
{
    g_return_if_fail(RP_IS_ACTIVE_STREAM_FILTER_BASE(self));
    PRIV(self)->m_end_stream = end_stream;
}

void
rp_active_stream_filter_base_set_processed_headers(RpActiveStreamFilterBase* self, bool processed_headers)
{
    g_return_if_fail(RP_IS_ACTIVE_STREAM_FILTER_BASE(self));
    PRIV(self)->m_processed_headers = processed_headers;
}

RpIterationState_e
rp_active_stream_filter_base_get_iteration_state(RpActiveStreamFilterBase* self)
{
    g_return_val_if_fail(RP_IS_ACTIVE_STREAM_FILTER_BASE(self), RpIterationState_Continue);
    return PRIV(self)->m_iteration_state;
}

void
rp_active_stream_filter_base_set_iteration_state(RpActiveStreamFilterBase* self,
                                                    RpIterationState_e iteration_state)
{
    g_return_if_fail(RP_IS_ACTIVE_STREAM_FILTER_BASE(self));
    PRIV(self)->m_iteration_state = iteration_state;
}

void
rp_active_stream_filter_base_send_local_reply(RpActiveStreamFilterBase* self, evhtp_res code, evbuf_t* body,
                                                modify_headers_cb modify_headers, const char* details, void* arg)
{
    LOGD("(%p, %d, %p(%zu), %p, %p, %p)",
        self, code, body, evbuf_length(body), modify_headers, details, arg);
    //TODO:...
    rp_filter_manager_send_local_reply(PRIV(self)->m_parent,
                                        code,
                                        body,
                                        modify_headers,
                                        details,
                                        arg);
}
