/*
 * rp-codec-read-filter.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_codec_read_filter_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_codec_read_filter_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-codec-client.h"
#include "rp-net-connection.h"
#include "rp-codec-read-filter.h"

struct _RpCodecReadFilter {
    RpNetworkReadFilterBaseImpl parent_instance;

    RpCodecClient* m_parent;
};

static void network_read_filter_iface_init(RpNetworkReadFilterInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpCodecReadFilter, rp_codec_read_filter, RP_TYPE_NETWORK_READ_FILTER_BASE_IMPL,
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_READ_FILTER, network_read_filter_iface_init)
)

static RpNetworkFilterStatus_e
on_data_i(RpNetworkReadFilter* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuffer_get_length(data), end_stream);

    RpCodecReadFilter* me = RP_CODEC_READ_FILTER(self);
    rp_codec_client_on_data(me->m_parent, data);
    if (end_stream && rp_network_connection_is_half_close_enabled(RP_NETWORK_CONNECTION(me->m_parent)))
    {
        NOISY_MSG_("closing");
        rp_codec_client_close_(me->m_parent);
    }
    return RpNetworkFilterStatus_StopIteration;
}

static void
network_read_filter_iface_init(RpNetworkReadFilterInterface* iface)
{
    iface->on_data = on_data_i;
}

OVERRIDE void
dispose(GObject* object)
{
    LOGD("(%p)", object);
    G_OBJECT_CLASS(rp_codec_read_filter_parent_class)->dispose(object);
}

static void
rp_codec_read_filter_class_init(RpCodecReadFilterClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_codec_read_filter_init(RpCodecReadFilter* self G_GNUC_UNUSED)
{
    LOGD("(%p)", self);
}

RpCodecReadFilter*
rp_codec_read_filter_new(void* arg)
{
    LOGD("(%p)", arg);
    g_return_val_if_fail(RP_IS_CODEC_CLIENT(arg), NULL);
    RpCodecReadFilter* self = g_object_new(RP_TYPE_CODEC_READ_FILTER, NULL);
    self->m_parent = RP_CODEC_CLIENT(arg);
    return self;
}
