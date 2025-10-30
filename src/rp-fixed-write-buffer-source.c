/*
 * rp-fixed-write-buffer-source.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_fixed_write_buffer_source_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_fixed_write_buffer_source_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rproxy.h"
#include "rp-fixed-write-buffer-source.h"

struct _RpFixedWriteBufferSource {
    GObject parent_instance;

    evbuf_t* m_data;
    bool m_end_stream;
};

static void write_buffer_source_iface_init(RpWriteBufferSourceInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpFixedWriteBufferSource, rp_fixed_write_buffer_source, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_WRITE_BUFFER_SOURCE, write_buffer_source_iface_init)
)

static struct RpStreamBuffer
get_write_buffer_i(RpWriteBufferSource* self)
{
    RpFixedWriteBufferSource* me = RP_FIXED_WRITE_BUFFER_SOURCE(self);
    return rp_stream_buffer_ctor(me->m_data, me->m_end_stream);
}

static void
write_buffer_source_iface_init(RpWriteBufferSourceInterface* iface)
{
    iface->get_write_buffer = get_write_buffer_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_fixed_write_buffer_source_parent_class)->dispose(obj);
}

static void
rp_fixed_write_buffer_source_class_init(RpFixedWriteBufferSourceClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_fixed_write_buffer_source_init(RpFixedWriteBufferSource* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpFixedWriteBufferSource*
rp_fixed_write_buffer_source_new(evbuf_t* data, bool end_stream)
{
    LOGD("(%p(%zu), %u)", data, data ? evbuffer_get_length(data) : 0, end_stream);
    RpFixedWriteBufferSource* self = g_object_new(RP_TYPE_FIXED_WRITE_BUFFER_SOURCE, NULL);
    self->m_data = data;
    self->m_end_stream = end_stream;
    return self;
}
