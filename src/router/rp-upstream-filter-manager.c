/*
 * rp-upstream-filter-manager.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_upstream_filter_manager_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_upstream_filter_manager_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "router/rp-router-filter-interface.h"
#include "router/rp-upstream-filter-manager.h"

#define UPSTREAM_REQUEST(s) RP_UPSTREAM_FILTER_MANAGER(self)->m_upstream_request
#define PARENT(s) rp_upstream_request_parent_(UPSTREAM_REQUEST(s))
#define STREAM_DECODER_FILTER_CALLBACKS(s) \
    rp_router_filter_interface_callbacks(PARENT(s))
#define STREAM_FILTER_CALLBAKS(s) \
    RP_STREAM_FILTER_CALLBACKS(STREAM_DECODER_FILTER_CALLBACKS(s))

struct _RpUpstreamFilterManager {
    RpFilterManager parent_instance;

    RpUpstreamRequest* m_upstream_request;
};

enum
{
    PROP_0, // Reserved.
    PROP_UPSTREAM_REQUEST,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_FINAL_TYPE(RpUpstreamFilterManager, rp_upstream_filter_manager, RP_TYPE_FILTER_MANAGER)

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p)", obj, prop_id, value, pspec);
    switch (prop_id)
    {
        case PROP_UPSTREAM_REQUEST:
            g_value_set_object(value, RP_UPSTREAM_FILTER_MANAGER(obj)->m_upstream_request);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p)", obj, prop_id, value, pspec);
    switch (prop_id)
    {
        case PROP_UPSTREAM_REQUEST:
            RP_UPSTREAM_FILTER_MANAGER(obj)->m_upstream_request = g_value_get_object(value);
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
    G_OBJECT_CLASS(rp_upstream_filter_manager_parent_class)->dispose(object);
}

OVERRIDE RpStreamInfo*
stream_info(RpFilterManager* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_stream_filter_callbacks_stream_info(STREAM_FILTER_CALLBAKS(self));
}

OVERRIDE void
send_local_reply(RpFilterManager* self, evhtp_res code, evbuf_t* body, modify_headers_cb modify_headers, const char* details, void* arg)
{
    NOISY_MSG_("(%p, %d, %p(%zu), %p, %p(%s), %p)",
        self, code, body, body ? evbuffer_get_length(body) : 0, modify_headers, details, details, arg);
    rp_filter_manager_set_decoder_filter_chain_aborted(self);
    rp_filter_manager_set_encoder_filter_chain_aborted(self);
    rp_filter_manager_set_encoder_filter_chain_complete(self);
    rp_filter_manager_set_observed_encode_end_stream(self);
    rp_stream_decoder_filter_callbacks_send_local_reply(STREAM_DECODER_FILTER_CALLBACKS(self), code, body, modify_headers, details, arg);
}

OVERRIDE void
execute_local_reply_if_prepared(RpFilterManager* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static void
filter_manager_class_init(RpFilterManagerClass* klass)
{
    LOGD("(%p)", klass);
    klass->stream_info = stream_info;
    klass->send_local_reply = send_local_reply;
    klass->execute_local_reply_if_prepared = execute_local_reply_if_prepared;
}

static void
rp_upstream_filter_manager_class_init(RpUpstreamFilterManagerClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    filter_manager_class_init(RP_FILTER_MANAGER_CLASS(klass));

    obj_properties[PROP_UPSTREAM_REQUEST] = g_param_spec_object("upstream-request",
                                                    "Upstream request",
                                                    "UpstreamRequest Instance",
                                                    RP_TYPE_UPSTREAM_REQUEST,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_upstream_filter_manager_init(RpUpstreamFilterManager* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpUpstreamFilterManager*
rp_upstream_filter_manager_new(RpFilterManagerCallbacks* filter_manager_callbacks,
                                                        RpDispatcher* dispatcher,
                                                        RpNetworkConnection* connection,
                                                        guint64 stream_id,
                                                        bool proxy_100_continue,
                                                        guint32 buffer_limit,
                                                        RpUpstreamRequest* request)
{
    LOGD("(%p, %p, %p, %lu, %u, %u, %p)",
        filter_manager_callbacks, dispatcher, connection, stream_id, proxy_100_continue,
        buffer_limit, request);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER_CALLBACKS(filter_manager_callbacks), NULL);
    g_return_val_if_fail(RP_IS_NETWORK_CONNECTION(connection), NULL);
    g_return_val_if_fail(RP_IS_UPSTREAM_REQUEST(request), NULL);
    return g_object_new(RP_TYPE_UPSTREAM_FILTER_MANAGER,
                        "filter-manager-callbacks", filter_manager_callbacks,
                        "dispatcher", dispatcher,
                        "connection", connection,
                        "proxy-100-continue", proxy_100_continue,
                        "buffer-limit", buffer_limit,
                        "upstream-request", request,
                        NULL);
}
