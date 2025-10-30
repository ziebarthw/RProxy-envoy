/*
 * rp-client-socket-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_client_socket_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_client_socket_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include "network/rp-client-socket-impl.h"

struct _RpClientSocketImpl {
    RpConnectionSocketImpl parent_instance;

};

G_DEFINE_FINAL_TYPE(RpClientSocketImpl, rp_client_socket_impl, RP_TYPE_CONNECTION_SOCKET_IMPL)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_client_socket_impl_parent_class)->dispose(obj);
}

static void
rp_client_socket_impl_class_init(RpClientSocketImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_client_socket_impl_init(RpClientSocketImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpClientSocketImpl*
rp_client_socket_impl_new(RpIoHandle* io_handle, struct sockaddr* remote_address)
{
    LOGD("(%p, %p)", io_handle, remote_address);
    g_return_val_if_fail(RP_IS_IO_HANDLE(io_handle), NULL);
    g_return_val_if_fail(remote_address != NULL, NULL);
    return g_object_new(RP_TYPE_CLIENT_SOCKET_IMPL,
                        "io-handle", io_handle,
                        "remote-address", remote_address,
                        NULL);
}
