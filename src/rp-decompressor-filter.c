/*
 * rp-decompressor-filter.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_decompressor_filter_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_decompressor_filter_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include "rproxy.h"
#include "rp-headers.h"
#include "rp-input-stream.h"
#include "rp-decompressor-filter.h"
#include "rp-state-filter.h"

#define ENCODER_FILTER_CALLBACKS(s) \
    rp_pass_through_filter_encoder_callbacks_(RP_PASS_THROUGH_FILTER(s))
#define STREAM_FILTER_CALLBACKS(s) \
    RP_STREAM_FILTER_CALLBACKS(ENCODER_FILTER_CALLBACKS(s))
#define STREAM_INFO(s) \
    rp_stream_filter_callbacks_stream_info(STREAM_FILTER_CALLBACKS(s))
#define FILTER_STATE(s) rp_stream_info_filter_state(STREAM_INFO(s))
#define REWRITE_URLS(s) \
    rp_filter_state_get_data(FILTER_STATE(s), rewrite_urls_key)

typedef struct RpDetails * RpDetails;
struct RpDetails {
    evbuf_t* m_output_buffer;
    GInputStream* m_istream;
    enc_type m_enc_type;
    bool m_transparent : 1;
    bool m_finished : 1;
};

struct _RpDecompressorFilter {
    RpPassThroughFilter parent_instance;

    struct RpDetails m_encode_details;
};

static void stream_encoder_filter_iface_init(RpStreamEncoderFilterInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpDecompressorFilter, rp_decompressor_filter, RP_TYPE_PASS_THROUGH_FILTER,
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_ENCODER_FILTER, stream_encoder_filter_iface_init)
)

static inline void
details_dispose(RpDetails details)
{
    g_clear_object(&details->m_istream);
    g_clear_pointer(&details->m_output_buffer, evbuffer_free);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpDecompressorFilter* me = RP_DECOMPRESSOR_FILTER(obj);
    details_dispose(&me->m_encode_details);

    G_OBJECT_CLASS(rp_decompressor_filter_parent_class)->dispose(obj);
}

static GInputStream*
create_istream(evhtp_headers_t* headers, enc_type type)
{
    NOISY_MSG_("(%p, %d)", headers, type);

    g_autoptr(RpInputStream) base = rp_input_stream_new();
    if (!base)
    {
        LOGE("alloc failed");
        return NULL;
    }

    g_autoptr(GInputStream) istream = util_create_input_stream(G_INPUT_STREAM(base), type);
    if (!istream)
    {
        LOGE("alloc failed");
        return NULL;
    }

if (type != enc_type_none)
{
    NOISY_MSG_("removing content-encoding header");
    evhtp_header_t* header = evhtp_headers_find_header(headers, "Content-encoding");
    evhtp_header_rm_and_free(headers, header);
}

    NOISY_MSG_("allocated input stream %p", istream);
    return g_steal_pointer(&istream);
}

static inline const char*
input_buffer_data(evbuf_t* input_buffer)
{
    NOISY_MSG_("(%p)", input_buffer);
    const char* data = input_buffer ?
                        (const char*)evbuffer_pullup(input_buffer, -1) : NULL;
    return data ? data : "";
}

static inline gsize
input_buffer_len(evbuf_t* input_buffer)
{
    NOISY_MSG_("(%p)", input_buffer);
    return input_buffer ? evbuffer_get_length(input_buffer) : 0;
}

static void
decompress(RpDecompressorFilter* self, RpDetails details, evbuf_t* input_buffer)
{
#   define BUF_SIZE (1024 * 32)

    NOISY_MSG_("(%p, %p, %p(%zu))", self, input_buffer, details, evbuffer_get_length(input_buffer));

    GConverterInputStream* istream = G_CONVERTER_INPUT_STREAM(details->m_istream);
    GConverter* converter = g_converter_input_stream_get_converter(istream);
    evbuf_t* output_buffer = details->m_output_buffer;
    const char* data = input_buffer_data(input_buffer);
    const char* startp = data; // Used to calculate bytes removed below.
    gsize len = input_buffer_len(input_buffer);
    struct evbuffer_iovec iovec[1];
    gsize bytes_read;
    gsize bytes_written;
    GConverterResult res;

    do
    {
        int n = evbuffer_reserve_space(output_buffer, BUF_SIZE, iovec, 1);
        NOISY_MSG_("%d extents, %p, %zu(%zu) bytes", n, iovec[0].iov_base, iovec[0].iov_len, evbuffer_get_length(output_buffer));

        g_autoptr(GError) err = NULL;
        res = g_converter_convert(converter, data, len, iovec[0].iov_base, iovec[0].iov_len, 0, &bytes_read, &bytes_written, &err);

        NOISY_MSG_("%zu bytes read, %zu bytes written", bytes_read, bytes_written);

        // Commit the bytes written (if any) to the output evbuffer.
        iovec[0].iov_len = bytes_written;
        evbuffer_commit_space(output_buffer, iovec, n);

        if (bytes_read || bytes_written)
        {
            data += bytes_read;
            len -= bytes_read;
        }
        else
        {
            NOISY_MSG_("res %d", res);
            switch (res)
            {
                case G_CONVERTER_CONVERTED:
                    NOISY_MSG_("converted; bytes_read %zu, bytes_written %zu", bytes_read, bytes_written);
                    break;
                case G_CONVERTER_ERROR:
                    if (err->code == G_IO_ERROR_PARTIAL_INPUT)
                        NOISY_MSG_("need more input data");
                    else
                        NOISY_MSG_("error %d(%s)", err->code, err->message);
                    break;
                case G_CONVERTER_FINISHED:
                    NOISY_MSG_("finished");
                    details->m_finished = true;
                    break;
                case G_CONVERTER_FLUSHED:
                    NOISY_MSG_("flushed");
                    break;
                default:
                    NOISY_MSG_("unknown");
                    break;
            }
        }
    }
    while (bytes_read || bytes_written);

    NOISY_MSG_("decoded %zu bytes", evbuffer_get_length(output_buffer));

    gsize bytes_removed = data - startp;
    NOISY_MSG_("%zu bytes removed from input buffer", bytes_removed);

    evbuffer_drain(input_buffer, bytes_removed);

#   undef BUF_SIZE
}

static inline enc_type
get_enc_type(evhtp_headers_t* headers, bool rewriting_urls)
{
    NOISY_MSG_("(%p, %u)", headers, rewriting_urls);
    // Only try decompressing if we are rewriting uls in content.
    if (!rewriting_urls)
    {
        NOISY_MSG_("none");
        return enc_type_none;
    }

    NOISY_MSG_("checking content-encoding");
    return util_get_enc_type(evhtp_header_find(headers, RpCustomHeaderValues.ContentEncoding));
}

static void
details_init(RpDetails me, evhtp_headers_t* headers, enc_type enc_type, bool rewriting_urls)
{
    NOISY_MSG_("(%p, %p, %u)", me, headers, rewriting_urls);
    me->m_enc_type = enc_type;
    if (enc_type != enc_type_none)
    {
        me->m_istream = create_istream(headers, me->m_enc_type);
        me->m_output_buffer = evbuffer_new();
        me->m_transparent = !rewriting_urls;
        me->m_finished = me->m_transparent;
    }
}

static RpFilterDataStatus_e
decompress_data_common(RpDecompressorFilter* self, RpDetails details, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %p(%zu), %u)", self, details, data, data ? evbuffer_get_length(data) : 0, end_stream);
    if (details->m_enc_type != enc_type_none)
    {
        decompress(self, details, data);
        if (!details->m_finished)
        {
            NOISY_MSG_("not finished");
            return RpFilterDataStatus_StopIterationNoBuffer;
//return RpFilterDataStatus_StopIterationAndBuffer;
        }
        // Reset "finished" flag in case there are more chunks.
        details->m_finished = details->m_transparent;
        // Move data from output buffer to the in/out |data| buffer.
        evbuffer_add_buffer(data, details->m_output_buffer);
    }
    return RpFilterDataStatus_Continue;
}

static RpFilterHeadersStatus_e
encode_headers_i(RpStreamEncoderFilter* self, evhtp_headers_t* response_headers, bool end_stream)
{
    NOISY_MSG_("(%p, %p, %u)", self, response_headers, end_stream);

    if (end_stream)
    {
        NOISY_MSG_("headers only response; nothing to do");
        return RpFilterHeadersStatus_Continue;
    }

    RpDecompressorFilter* me = RP_DECOMPRESSOR_FILTER(self);
    bool rewriting_urls = REWRITE_URLS(self) != NULL;
    enc_type enc_type = get_enc_type(response_headers, rewriting_urls);

    details_init(&me->m_encode_details, response_headers, enc_type, rewriting_urls);
    return RpFilterHeadersStatus_Continue;
}

static RpFilterDataStatus_e
encode_data_i(RpStreamEncoderFilter* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);

    RpDecompressorFilter* me = RP_DECOMPRESSOR_FILTER(self);
    return decompress_data_common(me, &me->m_encode_details, data, end_stream);
}

#define PARENT_STREAM_ENCODER_FILTER_IFACE(s) \
    ((RpStreamEncoderFilterInterface*)g_type_interface_peek_parent(RP_STREAM_ENCODER_FILTER_GET_IFACE(self)))

static void
encode_complete_i(RpStreamEncoderFilter* self)
{
    NOISY_MSG_("(%p)", self);
    PARENT_STREAM_ENCODER_FILTER_IFACE(self)->encode_complete(self);
}

static RpFilterTrailerStatus_e
encode_trailers_i(RpStreamEncoderFilter* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    return PARENT_STREAM_ENCODER_FILTER_IFACE(self)->encode_trailers(self, trailers);
}

static void
set_encoder_filter_callbacks_i(RpStreamEncoderFilter* self, RpStreamEncoderFilterCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    PARENT_STREAM_ENCODER_FILTER_IFACE(self)->set_encoder_filter_callbacks(self, callbacks);
}

static void
stream_encoder_filter_iface_init(RpStreamEncoderFilterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->encode_headers = encode_headers_i;
    iface->encode_data = encode_data_i;
    iface->encode_complete = encode_complete_i;
    iface->encode_trailers = encode_trailers_i;
    iface->set_encoder_filter_callbacks = set_encoder_filter_callbacks_i;
}

static void
rp_decompressor_filter_class_init(RpDecompressorFilterClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_decompressor_filter_init(RpDecompressorFilter* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpDecompressorFilter*
decompressor_filter_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_DECOMPRESSOR_FILTER, NULL);
}

static void
filter_factory_cb(RpFilterFactoryCb* self G_GNUC_UNUSED, RpFilterChainFactoryCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);

    RpDecompressorFilter* filter = decompressor_filter_new();
    rp_filter_chain_factory_callbacks_add_stream_encoder_filter(callbacks, RP_STREAM_ENCODER_FILTER(filter));
}

RpFilterFactoryCb*
rp_decompressor_filter_create_filter_factory(RpFactoryContext* context)
{
    LOGD("(%p)", context);
    g_return_val_if_fail(RP_IS_FACTORY_CONTEXT(context), NULL);
    return rp_filter_factory_cb_new(filter_factory_cb, g_free);
}
