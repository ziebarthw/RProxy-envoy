/*
 * Copyright [2012] [Mandiant, inc]
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(request_NOISY) || defined(ALL_NOISY)) && !defined(NO_request_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "request.h"

static inline char*
href_new(evbuf_t* evbuf, evhtp_request_t* upstream_request, const char* hostname)
{
    NOISY_MSG_("(%p, %p, %p(%s))", evbuf, upstream_request, hostname, hostname);
    if (!util_uri_to_evbuffer(upstream_request, hostname, evbuf))
    {
        LOGE("alloc failed");
        return NULL;
    }
    char* href = g_strndup((gchar*)evbuffer_pullup(evbuf, -1), evbuffer_get_length(evbuf));
    evbuffer_drain(evbuf, -1);
    return href;
}

request_t*
request_new(rproxy_t* rproxy, rule_t* rule, evhtp_request_t* upstream_request, const char* hostname)
{
    LOGD("(%p, %p, %p, %p(%s))", rproxy, rule, upstream_request, hostname, hostname);

    g_return_val_if_fail(rproxy != NULL, NULL);
    g_return_val_if_fail(rule != NULL, NULL);
    g_return_val_if_fail(upstream_request != NULL, NULL);

    request_t* self = g_new0(request_t, 1);

    self->rproxy = rproxy;
    self->rule = rule;
    self->upstream_request = upstream_request;
    self->upstream_bev = evhtp_request_get_bev(upstream_request);
    self->ds_pending = true;

    self->m_scratch_buf = evbuffer_new();
    if (!self->m_scratch_buf)
    {
        LOGE("alloc failed");
        request_free(self);
        return NULL;
    }

    self->m_data_buffer = evbuffer_new();
    if (!self->m_data_buffer)
    {
        LOGE("alloc failed");
        request_free(self);
        return NULL;
    }

    self->m_href = href_new(self->m_scratch_buf, upstream_request, hostname);
    if (!self->m_href)
    {
        LOGE("alloc failed");
        request_free(self);
        return NULL;
    }

    self->parser = htparser_new();
    if (!self->parser)
    {
        LOGE("alloc failed");
        request_free(self);
        return NULL;
    }

    htparser_init(self->parser, htp_type_response);
    htparser_set_userdata(self->parser, self);

    LOGD("self %p, %zu bytes", self, sizeof(*self));
    return self;
}

void
request_free(request_t* self)
{
    LOGD("(%p)", self);

    g_return_if_fail(self != NULL);
    g_clear_pointer(&self->parser, free);
    g_clear_pointer(&self->pending_ev, event_free);
    g_clear_pointer(&self->m_scratch_buf, evbuffer_free);
    g_clear_pointer(&self->m_buffered_response_data, evbuffer_free);
    g_clear_pointer(&self->m_data_buffer, evbuffer_free);
    g_clear_pointer(&self->m_status_text, g_free);
    g_clear_object(&self->m_response_istream);
    g_slist_free(g_steal_pointer(&self->m_decoder_filters));
    g_slist_free(g_steal_pointer(&self->m_encoder_filters));
    g_slist_free_full(g_steal_pointer(&self->m_filters), (GDestroyNotify)rp_stream_filter_base_on_destroy);
    g_clear_pointer(&self, g_free);
}

static inline void
add_filter(request_t* self, RpStreamFilterBase* filter)
{
    LOGD("(%p, %p)", self, filter);
    if (!g_slist_find(self->m_filters, filter))
    {
        NOISY_MSG_("prepending %p", filter);
        self->m_filters = g_slist_prepend(self->m_filters, filter);
    }
}

void
request_add_decoder_filter(request_t* self, RpStreamDecoderFilter* decoder_filter)
{
    LOGD("(%p, %p)", self, decoder_filter);
    g_return_if_fail(self != NULL);
    g_return_if_fail(decoder_filter != NULL);
    self->m_decoder_filters = g_slist_append(self->m_decoder_filters, decoder_filter);
    add_filter(self, RP_STREAM_FILTER_BASE(decoder_filter));
}

void
request_add_encoder_filter(request_t* self, RpStreamEncoderFilter* encoder_filter)
{
    LOGD("(%p, %p)", self, encoder_filter);
    g_return_if_fail(self != NULL);
    g_return_if_fail(encoder_filter != NULL);
    // prepend() because encoders are run in reverse order of decoders.
    self->m_encoder_filters = g_slist_prepend(self->m_encoder_filters, encoder_filter);
    add_filter(self, RP_STREAM_FILTER_BASE(encoder_filter));
}

void
request_add_filter(request_t* self, RpPassThroughFilter* filter)
{
    LOGD("(%p, %p)", self, filter);
    g_return_if_fail(self != NULL);
    g_return_if_fail(filter != NULL);
    g_return_if_fail(RP_IS_PASS_THROUGH_FILTER(filter));
    request_add_decoder_filter(self, RP_STREAM_DECODER_FILTER(filter));
    request_add_encoder_filter(self, RP_STREAM_ENCODER_FILTER(filter));
}

void
request_decode_headers(request_t* self, evhtp_headers_t* request_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, request_headers, end_stream);

    for (GSList* itr = self->m_decoder_filters; itr; itr = itr->next)
    {
        RpStreamDecoderFilter* filter = itr->data;
        RpFilterHeadersStatus_e status = rp_stream_decoder_filter_decode_headers(filter, request_headers, end_stream);
        NOISY_MSG_("status %d", status);
    }
}

static inline evbuf_t*
ensure_buffered_request_data(request_t* request)
{
    NOISY_MSG_("(%p)", request);
    if (request->m_buffered_request_data)
    {
        NOISY_MSG_("pre-allocated buffered request data %p", request->m_buffered_request_data);
        return request->m_buffered_request_data;
    }
    request->m_buffered_request_data = evbuffer_new();
    if (!request->m_buffered_request_data)
    {
        LOGE("alloc failed");
        return NULL;
    }
    NOISY_MSG_("allocated buffered request data %p", request->m_buffered_request_data);
    return request->m_buffered_request_data;
}

void
request_decode_data(request_t* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, data, end_stream);

    for (GSList* itr = self->m_decoder_filters; itr; itr = itr->next)
    {
        RpStreamDecoderFilter* filter = itr->data;
        switch (rp_stream_decoder_filter_decode_data(filter, data, end_stream))
        {
            case RpFilterDataStatus_Continue:
                break;
            case RpFilterDataStatus_StopIterationNoBuffer:
                return;
            case RpFilterDataStatus_StopIterationAndBuffer:
                evbuf_t* evbuf = ensure_buffered_request_data(self);
                evbuffer_add_buffer(evbuf, data);
                evbuffer_drain(data, -1);
                return;
            default:
                break;
        }
    }
}

void
request_decode_complete(request_t* self)
{
    NOISY_MSG_("(%p)", self);

    for (GSList* itr = self->m_decoder_filters; itr; itr = itr->next)
    {
        RpStreamDecoderFilter* filter = itr->data;
        rp_stream_decoder_filter_decode_complete(filter);
    }
}

void
request_encode_headers(request_t* self, evhtp_headers_t* response_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, response_headers, end_stream);

    for (GSList* itr = self->m_encoder_filters; itr; itr = itr->next)
    {
        RpStreamEncoderFilter* filter = itr->data;
        RpFilterHeadersStatus_e status = rp_stream_encoder_filter_encode_headers(filter, response_headers, end_stream);
        NOISY_MSG_("status %d", status);
    }
}

static inline evbuf_t*
ensure_buffered_response_data(request_t* request)
{
    NOISY_MSG_("(%p)", request);
    if (request->m_buffered_response_data)
    {
        NOISY_MSG_("pre-allocated buffered response data %p", request->m_buffered_response_data);
        return request->m_buffered_response_data;
    }
    request->m_buffered_response_data = evbuffer_new();
    if (!request->m_buffered_response_data)
    {
        LOGE("alloc failed");
        return NULL;
    }
    NOISY_MSG_("allocated buffered response data %p", request->m_buffered_response_data);
    return request->m_buffered_response_data;
}

void
request_encode_data(request_t* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, data, end_stream);

    for (GSList* itr = self->m_encoder_filters; itr; itr = itr->next)
    {
        RpStreamEncoderFilter* filter = itr->data;
        switch (rp_stream_encoder_filter_encode_data(filter, data, end_stream))
        {
            case RpFilterDataStatus_Continue:
                break;
            case RpFilterDataStatus_StopIterationNoBuffer:
                return;
            case RpFilterDataStatus_StopIterationAndBuffer:
                evbuf_t* evbuf = ensure_buffered_response_data(self);
                evbuffer_add_buffer(evbuf, data);
                evbuffer_drain(data, -1);
                return;
            default:
                break;
        }
    }
}

void
request_encode_complete(request_t* self)
{
    NOISY_MSG_("(%p)", self);

    for (GSList* itr = self->m_encoder_filters; itr; itr = itr->next)
    {
        RpStreamEncoderFilter* filter = itr->data;
        rp_stream_encoder_filter_encode_complete(filter);
    }
}

void
request_on_stream_complete(request_t* self)
{
    NOISY_MSG_("(%p)", self);

    for (GSList* itr = self->m_encoder_filters; itr; itr = itr->next)
    {
        RpStreamFilterBase* filter = itr->data;
        rp_stream_filter_base_on_stream_complete(filter);
    }
}

static inline int
request__set_hook_(request_hooks_t** hooks, request_hook_type type, request_hook cb, void* arg)
{
    LOGD("(%p(%p), %d, %p, %p)", hooks, *hooks, type, cb, arg);
    if (!*hooks && !(*hooks = calloc(sizeof(**hooks), 1)))
    {
        return -1;
    }
    switch (type)
    {
        case hook_on_upstream_headers:
            (*hooks)->on_upstream_headers     = cb;
            (*hooks)->on_upstream_headers_arg = arg;
            break;
        case hook_on_upstream_body:
            (*hooks)->on_upstream_body     = cb;
            (*hooks)->on_upstream_body_arg = arg;
            break;
        case hook_on_upstream_write:
            (*hooks)->on_upstream_write     = cb;
            (*hooks)->on_upstream_write_arg = arg;
            break;
        case hook_on_upstream_fini:
            (*hooks)->on_upstream_fini     = cb;
            (*hooks)->on_upstream_fini_arg = arg;
            break;
        case hook_on_response_start:
            (*hooks)->on_response_start     = cb;
            (*hooks)->on_response_start_arg = arg;
            break;
        case hook_on_downstream_headers:
            (*hooks)->on_downstream_headers     = (request_hook_status(*)(request_t*, evhtp_res, void*))cb;
            (*hooks)->on_downstream_headers_arg = arg;
            break;
        case hook_on_downstream_body:
            (*hooks)->on_downstream_body     = (request_hook_status(*)(request_t*, const char*, size_t, void*))cb;
            (*hooks)->on_downstream_body_arg = arg;
            break;
        case hook_on_downstream_fini:
            (*hooks)->on_downstream_fini     = cb;
            (*hooks)->on_downstream_fini_arg = arg;
            break;
        default:
            return -1;
    }
    return 0;
}

int
request_set_hook(request_t* self, request_hook_type type, request_hook cb, void* arg)
{
    LOGD("(%p, %d, %p, %p)", self, type, cb, arg);
    return self ? request__set_hook_(&self->m_hooks, type, cb, arg) : -1;
}
