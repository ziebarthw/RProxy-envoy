/*
 * rp-downstream-raw-buffer.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_downstream_raw_buffer_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_downstream_raw_buffer_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "network/rp-raw-buffer-socket.h"
#include "transport_sockets/rp-raw-buffer.h"

struct _RpDownstreamRawBufferSocketFactory {
    GObject parent_instance;

};

static void downstream_transport_socket_config_factory_iface_init(RpDownstreamTransportSocketConfigFactoryInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpDownstreamRawBufferSocketFactory, rp_downstream_raw_buffer_socket_factory, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_DOWNSTREAM_TRANSPORT_SOCKET_CONFIG_FACTORY, downstream_transport_socket_config_factory_iface_init)
)

static RpDownstreamTransportSocketFactoryPtr
create_transport_socket_factory_i(RpDownstreamTransportSocketConfigFactory* self, gpointer config, RpTransportSocketFactoryContext* context G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %p, %p)", self, config, context);
    server_cfg_t* server_cfg = config;
    return RP_DOWNSTREAM_TRANSPORT_SOCKET_FACTORY(
            rp_raw_buffer_socket_factory_create_downstream_factory(server_cfg));
}

static void
downstream_transport_socket_config_factory_iface_init(RpDownstreamTransportSocketConfigFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_transport_socket_factory = create_transport_socket_factory_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_downstream_raw_buffer_socket_factory_parent_class)->dispose(obj);
}

static void
rp_downstream_raw_buffer_socket_factory_class_init(RpDownstreamRawBufferSocketFactoryClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_downstream_raw_buffer_socket_factory_init(RpDownstreamRawBufferSocketFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpDownstreamRawBufferSocketFactory*
rp_downstream_raw_buffer_socket_factory_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_DOWNSTREAM_RAW_BUFFER_SOCKET_FACTORY, NULL);
}
