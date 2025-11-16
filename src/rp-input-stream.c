/*
 * Copyright/Licensing information
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_input_stream_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_input_stream_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rproxy.h"
#include "rp-input-stream.h"

struct _RpInputStream
{
    GInputStream parent_instance;

    const char* m_data;
    size_t m_len;
};

enum
{
    PROP_0, // Reserved.
    PROP_DATA,
    PROP_LEN,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

/*
 * forward definitions
 */
G_DEFINE_TYPE(RpInputStream, rp_input_stream, G_TYPE_INPUT_STREAM)

static inline int
read_internal(RpInputStream* self, void* buffer, gsize count)
{
    NOISY_MSG_("(%p, %p, %zu)", self, buffer, count);

    gsize nbytes = self->m_len > count ? count : self->m_len;
    memcpy(buffer, self->m_data, nbytes);
    self->m_len -= nbytes;
    self->m_data += nbytes;
    return nbytes;
}

// Called when a plain text input body is being processed.
static void
read_async_i(GInputStream* stream, void* buffer, gsize count, int io_priority, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    NOISY_MSG_("(%p, %p, %zd, %d, %p, %p, %p)",
        stream, buffer, count, io_priority, cancellable, callback, user_data);

    int bytes_read = read_internal(RP_INPUT_STREAM(stream), buffer, count);
    GTask* task = g_task_new(stream, cancellable, callback, user_data);
    g_task_return_int(task, bytes_read);
    g_object_unref(task);
}

// Called when an encoded (gzip, compressed, etc) is being processed.
static gssize
read_fn_i(GInputStream* stream, void* buffer, gsize count, GCancellable* cancellable, GError** error)
{
    LOGD("(%p, %p, %zu, %p, %p)", stream, buffer, count, cancellable, error);
    return read_internal(RP_INPUT_STREAM(stream), buffer, count);
}

static gssize
read_finish_i(GInputStream* stream, GAsyncResult* result, GError** error)
{
    NOISY_MSG_("(%p, %p, %p)", stream, result, error);
    g_return_val_if_fail(g_task_is_valid(result, stream), -1);
    return g_task_propagate_int(G_TASK(result), error);
}

static void
get_property_i(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_DATA:
            g_value_set_string(value, RP_INPUT_STREAM(obj)->m_data);
            break;
        case PROP_LEN:
            g_value_set_ulong(value, RP_INPUT_STREAM(obj)->m_len);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
set_property_i(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_DATA:
            RP_INPUT_STREAM(obj)->m_data = g_value_get_string(value);
            break;
        case PROP_LEN:
            RP_INPUT_STREAM(obj)->m_len = g_value_get_ulong(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
dispose_i(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_input_stream_parent_class)->dispose(obj);
}

static void
rp_input_stream_class_init(RpInputStreamClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property_i;
    object_class->set_property = set_property_i;
    object_class->dispose = dispose_i;

    obj_properties[PROP_DATA] = g_param_spec_string("data",
                                                    "Data",
                                                    "Data",
                                                    "",
                                                    G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);

    obj_properties[PROP_LEN] = g_param_spec_ulong("len",
                                                    "Len",
                                                    "Len",
                                                    0,
                                                    ULONG_MAX,
                                                    0,
                                                    G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);

    GInputStreamClass* input_stream_class = G_INPUT_STREAM_CLASS(klass);
    input_stream_class->read_async = read_async_i;
    input_stream_class->read_finish = read_finish_i;
    input_stream_class->read_fn = read_fn_i;
}

static void
rp_input_stream_init(RpInputStream* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpInputStream*
rp_input_stream_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_INPUT_STREAM, NULL);
}

void
rp_input_istream_set_data(RpInputStream* self, const char* data, gsize len)
{
    LOGD("(%p, %p, %zu)", self, data, len);
    g_return_if_fail(RP_IS_INPUT_STREAM(self));
    self->m_data = data;
    self->m_len = len;
}
