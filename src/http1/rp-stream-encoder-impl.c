/*
 * rp-stream-encoder-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_stream_encoder_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_stream_encoder_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-headers.h"
#include "rp-http-utility.h"
#include "rp-stream-encoder-impl.h"

typedef struct _RpStreamEncoderImplPrivate RpStreamEncoderImplPrivate;
struct _RpStreamEncoderImplPrivate {

    RpHttp1ConnectionImpl* m_connection;

    const char* m_details;

    RpCodecEventCallbacks* m_codec_callbacks;
    GSList/*<StreamCallbacks>*/* m_callbacks;

    guint32 m_read_disabled_calls;

    bool m_disable_chunk_encoding : 1;
    bool m_chunk_encoding : 1;
    bool m_connect_request : 1;
    bool m_is_tcp_tunneling : 1;
    bool m_is_response_to_head_request : 1;
    bool m_is_response_to_connect_request : 1;
};

enum
{
    PROP_0, // Reserved.
    PROP_CONNECTION,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void stream_reset_handler_iface_init(RpStreamResetHandlerInterface* iface);
static void stream_encoder_iface_init(RpStreamEncoderInterface* iface);
static void stream_iface_init(RpStreamInterface* iface);
static void http1_stream_encoder_options_iface_init(RpHttp1StreamEncoderOptionsInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpStreamEncoderImpl, rp_stream_encoder_impl, RP_TYPE_STREAM_CALLBACK_HELPER,
    G_ADD_PRIVATE(RpStreamEncoderImpl)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_RESET_HANDLER, stream_reset_handler_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM, stream_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_ENCODER, stream_encoder_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_HTTP1_STREAM_ENCODER_OPTIONS, http1_stream_encoder_options_iface_init)
)

#define PRIV(obj) \
    ((RpStreamEncoderImplPrivate*)rp_stream_encoder_impl_get_instance_private(RP_STREAM_ENCODER_IMPL(obj)))

static void
reset_stream_i(RpStreamResetHandler* self, RpStreamResetReason_e reason)
{
    NOISY_MSG_("(%p, %d)", self, reason);
    rp_http1_connection_impl_on_reset_stream_base(PRIV(self)->m_connection, reason);
}

static void
stream_reset_handler_iface_init(RpStreamResetHandlerInterface* iface)
{
    LOGD("(%p)", iface);
    iface->reset_stream = reset_stream_i;
}

static void
add_callbacks_i(RpStream* self, RpStreamCallbacks* callbacks)
{
    NOISY_MSG_("(%p)", callbacks);
    RpStreamEncoderImplPrivate* me = PRIV(self);
    me->m_callbacks = g_slist_append(me->m_callbacks, callbacks);
}

static guint32
buffer_limit_i(RpStream* self)
{
    NOISY_MSG_("(%p)", self);
    //TODO...return connection_.bufferLimit();
    return 0;
}

static void
read_disable_i(RpStream* self, bool disable)
{
    NOISY_MSG_("(%p, %u)", self, disable);
    RpStreamEncoderImplPrivate* me = PRIV(self);
    if (disable)
    {
        ++me->m_read_disabled_calls;
    }
    else if (me->m_read_disabled_calls != 0)
    {
        LOGD("%u does not equal zero!", me->m_read_disabled_calls);
    }
    else
    {
        --me->m_read_disabled_calls;
    }
    rp_http1_connection_impl_read_disable(me->m_connection, disable);
}

static RpCodecEventCallbacks*
register_codec_event_callbacks_i(RpStream* self, RpCodecEventCallbacks* codec_callbacks)
{
    NOISY_MSG_("(%p, %p)", self, codec_callbacks);
    RpStreamEncoderImplPrivate* me = PRIV(self);
    RpCodecEventCallbacks* codec_callbacks_ = me->m_codec_callbacks;
    me->m_codec_callbacks = codec_callbacks;
    return codec_callbacks_;
}

static void
remove_callbacks_i(RpStream* self, RpStreamCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    RpStreamEncoderImplPrivate* me = PRIV(self);
    me->m_callbacks = g_slist_remove(me->m_callbacks, callbacks);
}

static const char*
response_details_i(RpStream* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_details;
}

static void
set_flush_timeout_i(RpStream* self G_GNUC_UNUSED, guint32 timeout_ms G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %u)", self, timeout_ms);
}

static RpConnectionInfoProvider*
connection_info_provider_i(RpStream* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_network_connection_connection_info_provider(
                rp_http1_connection_impl_connection(PRIV(self)->m_connection));
}

static void
stream_iface_init(RpStreamInterface* iface)
{
    LOGD("(%p)", iface);
    iface->add_callbacks = add_callbacks_i;
    iface->buffer_limit = buffer_limit_i;
    iface->read_disable = read_disable_i;
    iface->register_codec_event_callbacks = register_codec_event_callbacks_i;
    iface->remove_callbacks = remove_callbacks_i;
    iface->response_details = response_details_i;
    iface->set_flush_timeout = set_flush_timeout_i;
    iface->connection_info_provider = connection_info_provider_i;
}

static void
notify_encode_complete(RpStreamEncoderImplPrivate* me)
{
    NOISY_MSG_("(%p)", me);

    if (me->m_codec_callbacks)
    {
        rp_codec_event_callbacks_on_codec_encode_complete(me->m_codec_callbacks);
    }
    rp_http1_connection_impl_on_encode_complete(me->m_connection);
}

static void
flush_output(RpStreamEncoderImplPrivate* me, bool end_encode)
{
    NOISY_MSG_("(%p, %u)", me, end_encode);
    guint64 encoded_bytes G_GNUC_UNUSED = rp_http1_connection_impl_flush_output(me->m_connection, end_encode);
    //TODO...bytes_meter_->addWriteBytesSent(encoded_bytes);
}

static void
end_encode(RpStreamEncoderImplPrivate* me)
{
    NOISY_MSG_("(%p)", me);

    if (me->m_chunk_encoding)
    {
        NOISY_MSG_("terminating chunked body");
        rp_http1_connection_impl_buffer_add(me->m_connection, "0\r\n\r\n", 5);
    }

    flush_output(me, true);
    notify_encode_complete(me);
    if (me->m_connect_request || me->m_is_tcp_tunneling)
    {
        RpNetworkConnection* connection = rp_http1_connection_impl_connection(me->m_connection);
        rp_network_connection_close(connection, RpNetworkConnectionCloseType_FlushWrite);
    }
}

static void
encode_data_i(RpStreamEncoder* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuffer_get_length(data), end_stream);

    RpStreamEncoderImplPrivate* me = PRIV(self);
    size_t length = evbuffer_get_length(data);
    if (length > 0)
    {
        evbuf_t* output = rp_http1_connection_impl_buffer(me->m_connection);
        if (me->m_chunk_encoding)
        {
            evbuffer_add_printf(output, "%x\r\n", (unsigned)length);
        }

        evbuffer_add_buffer(output, data);

        if (me->m_chunk_encoding)
        {
            evbuffer_add(output, "\r\n", 2);
        }
    }

    if (end_stream)
    {
        end_encode(me);
    }
    else
    {
        flush_output(me, false);
    }
}

static RpStream*
get_stream_i(RpStreamEncoder* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_STREAM(self);
}

static void
stream_encoder_iface_init(RpStreamEncoderInterface* iface)
{
    LOGD("(%p)", iface);
    iface->encode_data = encode_data_i;
    iface->get_stream = get_stream_i;
}

static void
disable_chunk_encoding_i(RpHttp1StreamEncoderOptions* self)
{
    NOISY_MSG_("(%p)", self);
    PRIV(self)->m_disable_chunk_encoding = true;
}

static void
http1_stream_encoder_options_iface_init(RpHttp1StreamEncoderOptionsInterface* iface)
{
    LOGD("(%p)", iface);
    iface->disable_chunk_encoding = disable_chunk_encoding_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_CONNECTION:
            g_value_set_object(value, PRIV(obj)->m_connection);
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
        case PROP_CONNECTION:
            PRIV(obj)->m_connection = g_value_get_object(value);
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

    RpStreamEncoderImplPrivate* me = PRIV(obj);
    while (me->m_read_disabled_calls != 0)
    {
        read_disable_i(RP_STREAM(obj), false);
    }

    G_OBJECT_CLASS(rp_stream_encoder_impl_parent_class)->dispose(obj);
}

static void
rp_stream_encoder_impl_class_init(RpStreamEncoderImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_CONNECTION] = g_param_spec_object("connection",
                                                    "Connection",
                                                    "ConnectionImpl Instance",
                                                    RP_TYPE_HTTP1_CONNECTION_IMPL,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_stream_encoder_impl_init(RpStreamEncoderImpl* self)
{
    NOISY_MSG_("(%p)", self);

    RpStreamEncoderImplPrivate* me = PRIV(self);
    me->m_chunk_encoding = true;
    me->m_connect_request = false;
    me->m_is_tcp_tunneling = false;
    me->m_is_response_to_head_request = false;
    me->m_is_response_to_connect_request = false;
}

void
rp_stream_encoder_impl_encode_headers_base(RpStreamEncoderImpl* self, evhtp_headers_t* headers, evhtp_res status, bool end_stream, bool bodiless_request)
{
    LOGD("(%p, %p, %d, %u, %u)", self, headers, status, end_stream, bodiless_request);

    g_return_if_fail(RP_IS_STREAM_ENCODER_IMPL(self));
    g_return_if_fail(headers != NULL);

    RpStreamEncoderImplPrivate* me = PRIV(self);
    size_t content_length_len = strlen(RpHeaderValues.ContentLength);
    bool saw_content_length = false;
    evbuf_t* output = rp_http1_connection_impl_buffer(me->m_connection);
    for (evhtp_header_t* header=TAILQ_FIRST(headers); header; header = TAILQ_NEXT(header, next))
    {
        const char* key_to_use = header->key;
        size_t key_size_to_use = header->klen;
        // Translate :authority -> host so that upper layers to not need to deal
        // with this.
        if (key_size_to_use > 1 && key_to_use[0] == ':' && key_to_use[1] == 'a' &&
            !evhtp_header_find(headers, RpHeaderValues.HostLegacy)/*TODO: not part of the envoy code; don't know how they avoid dup host: headers(?)*/)
        {
            key_to_use = RpHeaderValues.HostLegacy;
            key_size_to_use = strlen(key_to_use);
        }

        // Skip all headers starting with ':' that make it here.
        if (key_to_use[0] == ':')
        {
            continue;
        }

        if (!saw_content_length &&
            key_size_to_use == content_length_len &&
            g_ascii_strncasecmp(key_to_use, RpHeaderValues.ContentLength, key_size_to_use) == 0)
        {
            saw_content_length = true;
        }

        evbuffer_add(output, key_to_use, key_size_to_use);
        evbuffer_add(output, ": ", 2);
        evbuffer_add(output, header->val, header->vlen);
        evbuffer_add(output, "\r\n", 2);
    }

    if (saw_content_length || me->m_disable_chunk_encoding)
    {
        me->m_chunk_encoding = false;
    }
    else
    {
        if (status && (status < 200 || status == 204))
        {
            me->m_chunk_encoding = false;
        }
        else if (status == 304)
        {
            me->m_chunk_encoding = false;
        }
        else if (end_stream && !me->m_is_response_to_head_request)
        {
            if (!status || (status >= 200 && status != 204))
            {
                if (!bodiless_request)
                {
                    evbuffer_add(output, RpHeaderValues.ContentLength, content_length_len);
                    evbuffer_add(output, ": 0\r\n", 5);
                }
            }
            me->m_chunk_encoding = false;
        }
        else if (rp_http_connection_protocol(RP_HTTP_CONNECTION(me->m_connection)) == EVHTP_PROTO_10)
        {
            me->m_chunk_encoding = false;
        }
        else
        {
            if (!me->m_is_response_to_connect_request)
            {
                evbuffer_add(output, RpHeaderValues.TransferEncoding, strlen(RpHeaderValues.TransferEncoding));
                evbuffer_add(output, ": ", 2);
                evbuffer_add(output, RpHeaderValues.TransferCodingValues.Chunked, strlen(RpHeaderValues.TransferCodingValues.Chunked));
                evbuffer_add(output, "\r\n", 2);
            }
            me->m_chunk_encoding = !http_utility_is_upgrade(headers) &&
                                    !me->m_is_response_to_head_request &&
                                    !me->m_is_response_to_connect_request;
        }
    }

    evbuffer_add(output, "\r\n", 2);

    if (end_stream)
    {
        end_encode(me);
    }
    else
    {
        flush_output(me, false);
    }
}

void
rp_stream_encoder_impl_encode_trailers_base(RpStreamEncoderImpl* self, evhtp_headers_t* trailers)
{
    LOGD("(%p, %p)", self, trailers);
    g_return_if_fail(RP_IS_STREAM_ENCODER_IMPL(self));
    g_return_if_fail(trailers != NULL);
    RpStreamEncoderImplPrivate* me = PRIV(self);
    if (!rp_http1_connection_impl_enable_trailers(me->m_connection))
    {
        end_encode(me);
        return;
    }
    //TODO:....
}

void
rp_stream_encoder_impl_set_connection_request(RpStreamEncoderImpl* self, bool v)
{
    LOGD("(%p, %u)", self, v);
    g_return_if_fail(RP_IS_STREAM_ENCODER_IMPL(self));
    PRIV(self)->m_connect_request = v;
}

void
rp_stream_encoder_impl_set_is_response_to_connect_request(RpStreamEncoderImpl* self, bool v)
{
    LOGD("(%p, %u)", self, v);
    g_return_if_fail(RP_IS_STREAM_ENCODER_IMPL(self));
    PRIV(self)->m_is_response_to_connect_request = v;
}

void
rp_stream_encoder_impl_set_is_response_to_head_request(RpStreamEncoderImpl* self, bool v)
{
    LOGD("(%p, %u)", self, v);
    g_return_if_fail(RP_IS_STREAM_ENCODER_IMPL(self));
    PRIV(self)->m_is_response_to_head_request = v;
}

bool
rp_stream_encoder_impl_connect_request_(RpStreamEncoderImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_STREAM_ENCODER_IMPL(self), false);
    return PRIV(self)->m_connect_request;
}

void
rp_stream_encoder_impl_set_is_tcp_tunneling(RpStreamEncoderImpl* self, bool v)
{
    LOGD("(%p, %u)", self, v);
    g_return_if_fail(RP_IS_STREAM_ENCODER_IMPL(self));
    PRIV(self)->m_is_tcp_tunneling = v;
}
