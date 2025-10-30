/*
 * rp-fixed-read-buffer-source.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_fixed_read_buffer_source_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_fixed_read_buffer_source_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rproxy.h"
#include "rp-fixed-read-buffer-source.h"

struct _RpFixedReadBufferSource {
    GObject parent_instance;

    evbuf_t* m_data;
    bool m_end_stream;
};

static void read_buffer_source_iface_init(RpReadBufferSourceInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpFixedReadBufferSource, rp_fixed_read_buffer_source, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_READ_BUFFER_SOURCE, read_buffer_source_iface_init)
)

static struct RpStreamBuffer
get_read_buffer_i(RpReadBufferSource* self)
{
    RpFixedReadBufferSource* me = RP_FIXED_READ_BUFFER_SOURCE(self);
    return rp_stream_buffer_ctor(me->m_data, me->m_end_stream);
}

static void
read_buffer_source_iface_init(RpReadBufferSourceInterface* iface)
{
    iface->get_read_buffer = get_read_buffer_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_fixed_read_buffer_source_parent_class)->dispose(obj);
}

static void
rp_fixed_read_buffer_source_class_init(RpFixedReadBufferSourceClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_fixed_read_buffer_source_init(RpFixedReadBufferSource* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpFixedReadBufferSource*
rp_fixed_read_buffer_source_new(evbuf_t* data, bool end_stream)
{
    LOGD("(%p(%zu), %u)", data, data ? evbuffer_get_length(data) : 0, end_stream);
    RpFixedReadBufferSource* self = g_object_new(RP_TYPE_FIXED_READ_BUFFER_SOURCE, NULL);
    self->m_data = data;
    self->m_end_stream = end_stream;
    return self;
}
