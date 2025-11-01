/*
 * rp-accepted-socket-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_accepted_socket_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_accepted_socket_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "network/rp-accepted-socket-impl.h"

struct _RpAcceptedSocketImpl {
    RpConnectionSocketImpl parent_instance;

    UNIQUE_PTR(evhtp_connection_t) m_conn;

//TODO...accepted_socket_count...
};

G_DEFINE_FINAL_TYPE(RpAcceptedSocketImpl, rp_accepted_socket_impl, RP_TYPE_CONNECTION_SOCKET_IMPL)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpAcceptedSocketImpl* self = RP_ACCEPTED_SOCKET_IMPL(obj);
    g_clear_pointer(&self->m_conn, evhtp_connection_free);

    G_OBJECT_CLASS(rp_accepted_socket_impl_parent_class)->dispose(obj);
}

static void
rp_accepted_socket_impl_class_init(RpAcceptedSocketImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_accepted_socket_impl_init(RpAcceptedSocketImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline struct sockaddr_storage
get_remote_address(int sockfd)
{
    struct sockaddr_storage sockaddr;
    socklen_t len = sizeof(sockaddr);
    getpeername(sockfd, (struct sockaddr*)&sockaddr, &len);
    return sockaddr;
}

RpAcceptedSocketImpl*
rp_accepted_socket_impl_new(RpIoHandle* io_handle, UNIQUE_PTR(evhtp_connection_t) conn)
{
    LOGD("(%p, %p)", io_handle, conn);
    g_return_val_if_fail(RP_IS_IO_HANDLE(io_handle), NULL);
    g_return_val_if_fail(conn != NULL, NULL);

    struct sockaddr_storage remote_address = get_remote_address(conn->sock);

    RpAcceptedSocketImpl* self = g_object_new(RP_TYPE_ACCEPTED_SOCKET_IMPL,
                                                "io-handle", io_handle,
                                                "local-address", conn->saddr,
                                                "remote-address", &remote_address,
                                                NULL);
    self->m_conn = conn;
    return self;
}
