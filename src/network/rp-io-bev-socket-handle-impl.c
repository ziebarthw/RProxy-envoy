/*
 * rp-io-bev-socket-handle-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_io_bev_socket_handle_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_io_bev_socket_handle_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-dispatcher.h"
#include "rp-http-utility.h"
#include "event/rp-schedulable-cb-impl.h"
#include "network/rp-io-bev-socket-handle-impl.h"

#define SOCKFD(s) \
    (RP_IO_BEV_SOCKET_HANDLE_IMPL(s)->m_bev ? bufferevent_getfd(RP_IO_BEV_SOCKET_HANDLE_IMPL(s)->m_bev) : -1)

typedef struct _RpIoBevSocketHandleImpl RpIoBevSocketHandleImpl;
struct _RpIoBevSocketHandleImpl {
    GObject parent_instance;

    UNIQUE_PTR(evbev_t) m_bev;
    evdns_base_t* m_dns_base;

    RpDispatcher* m_dispatcher;
    UNIQUE_PTR(RpSchedulableCallback) m_activation_cb;

    RpFileReadyCb m_cb;
    gpointer m_arg;

    struct sockaddr_storage m_local_addr;
    struct sockaddr_storage m_peer_addr;

    guint32 m_injected_activation_events;
    guint32 m_enabled_events;

    RpHandleType_e m_type;

    bool m_was_connected : 1;
    bool m_initialized : 1;
};

static void file_event_merge_injected_events_and_run_cb(RpIoBevSocketHandleImpl* self, guint32 events);
static void io_handle_iface_init(RpIoHandleInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpIoBevSocketHandleImpl, rp_io_bev_socket_handle_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_IO_HANDLE, io_handle_iface_init)
)

static void
readcb(evbev_t* bev, void* arg)
{
    NOISY_MSG_("(%p(fd %d), %p)", bev, bufferevent_getfd(bev), arg);

    RpIoBevSocketHandleImpl* self = RP_IO_BEV_SOCKET_HANDLE_IMPL(arg);
NOISY_MSG_("%zu bytes read from fd %d", evbuffer_get_length(bufferevent_get_input(bev)), bufferevent_getfd(bev));
NOISY_MSG_("\n\"%.*s\"", (int)evbuffer_get_length(bufferevent_get_input(bev)), (gchar*)evbuffer_pullup(bufferevent_get_input(bev), -1));
    file_event_merge_injected_events_and_run_cb(self, RpFileReadyType_Read);
}

static void
writecb(evbev_t* bev G_GNUC_UNUSED, void* arg G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p(fd %d), %p)", bev, bufferevent_getfd(bev), arg);

//    RpIoBevSocketHandleImpl* self = RP_IO_BEV_SOCKET_HANDLE_IMPL(arg);
//    file_event_merge_injected_events_and_run_cb(self, RpFileReadyType_Write);
NOISY_MSG_("%zu bytes in output buffer for fd %d", evbuffer_get_length(bufferevent_get_output(bev)), bufferevent_getfd(bev));
}

static inline void
do_bufferevent_error(evbev_t* bev)
{
    NOISY_MSG_("(%p)", bev);

    int dns_err = bufferevent_socket_get_dns_error(bev);
    if (dns_err)
    {
        LOGE("DNS error %d(%s)", dns_err, evutil_gai_strerror(dns_err));
        return;
    }

    int errcode = EVUTIL_SOCKET_ERROR();
    if (errcode)
    {
        const char* errmsg = evutil_socket_error_to_string(errcode);
        LOGE("Socket error %d(%s)", errcode, errmsg);
    }

    int fd = bufferevent_getfd(bev);
    if (fd != -1)
    {
        int so_error = 0;
        socklen_t len = sizeof(so_error);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len) == 0 && so_error != 0)
        {
            LOGE("Socket option error %d(%s)", so_error, evutil_socket_error_to_string(so_error));
        }
    }
}

static void
eventcb(evbev_t* bev, short events, void* arg)
{
    NOISY_MSG_("(%p(fd %d), %x, %p)", bev, bufferevent_getfd(bev), events, arg);

    int sockfd = bufferevent_getfd(bev); // Grab sockfd in case bev is destroyed before end of func.
    RpIoBevSocketHandleImpl* self = RP_IO_BEV_SOCKET_HANDLE_IMPL(arg);

    if ((events & BEV_EVENT_EOF) || (events & BEV_EVENT_ERROR))
    {
        if (events & BEV_EVENT_ERROR)
        {
            do_bufferevent_error(bev);
        }
        else
        {
            NOISY_MSG_("EOF on fd %d", sockfd);
        }
        file_event_merge_injected_events_and_run_cb(self, RpFileReadyType_Closed);
    }
    else if (events & BEV_EVENT_TIMEOUT)
    {
        NOISY_MSG_("timeout on fd %d", sockfd);
        file_event_merge_injected_events_and_run_cb(self, RpFileReadyType_Closed);
    }
    else if (events & BEV_EVENT_CONNECTED)
    {
        LOGD("connected on fd %d", sockfd);
        self->m_was_connected = true;
        if (self->m_type == RpHandleType_Connecting)
        {
            NOISY_MSG_("enabling read, write, event on fd %d, %zu bytes available to write", sockfd, evbuffer_get_length(bufferevent_get_output(bev)));
            bufferevent_setcb(bev, readcb, writecb, eventcb, self);
            if (self->m_enabled_events & RpFileReadyType_Write)
            {
                file_event_merge_injected_events_and_run_cb(self, RpFileReadyType_Write);
            }
        }
    }
}

static inline void
set_enabled_events(RpIoBevSocketHandleImpl* self, guint32 enabled_events)
{
    NOISY_MSG_("(%p(fd %d), %u)", self, SOCKFD(self), enabled_events);
    self->m_enabled_events = enabled_events;
}

static inline guint32
get_enabled_events(RpIoBevSocketHandleImpl* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
    return self->m_enabled_events;
}

static void
file_event_assign_events(RpIoBevSocketHandleImpl* self, guint32 events)
{
    NOISY_MSG_("(%p(fd %d), %u)", self, SOCKFD(self), events);

    bufferevent_data_cb readcb_ = NULL;
    bufferevent_data_cb writecb_ = NULL;

    self->m_enabled_events = events;
    if (events & RpFileReadyType_Read)
    {
        NOISY_MSG_("enabling read on fd %d", SOCKFD(self));
        readcb_ = readcb;
    }
    if (events & RpFileReadyType_Write)
    {
        NOISY_MSG_("enabling write on fd %d", SOCKFD(self));
        writecb_ = writecb;
    }

    bufferevent_setcb(self->m_bev, readcb_, writecb_, eventcb, self);

    if (events & RpFileReadyType_Closed)
    {
        NOISY_MSG_("enabling close on fd %d", SOCKFD(self));
        bufferevent_trigger_event(self->m_bev, BEV_EVENT_EOF, 0);
    }
}

static void
file_event_activate(RpIoBevSocketHandleImpl* self, guint32 events)
{
    NOISY_MSG_("(%p(fd %d), %u)", self, SOCKFD(self), events);

    if (self->m_injected_activation_events == 0)
    {
        g_assert(!rp_schedulable_callback_enabled(self->m_activation_cb));
        rp_schedulable_callback_schedule_callback_next_iteration(self->m_activation_cb);
    }
    g_assert(rp_schedulable_callback_enabled(self->m_activation_cb));

    self->m_injected_activation_events |= events;
}

static void
file_event_update_events(RpIoBevSocketHandleImpl* self, guint32 events)
{
    NOISY_MSG_("(%p(fd %d), %u)", self, SOCKFD(self), events);
    if (events == self->m_enabled_events)
    {
        NOISY_MSG_("nothing to do");
        return;
    }
    bufferevent_setcb(self->m_bev, NULL, NULL, eventcb, self);
    file_event_assign_events(self, events);
}

static void
file_event_set_enabled(RpIoBevSocketHandleImpl* self, guint32 events)
{
    NOISY_MSG_("(%p(fd %d), %u)", self, SOCKFD(self), events);
    if (self->m_injected_activation_events != 0)
    {
        self->m_injected_activation_events = 0;
        rp_schedulable_callback_cancel(self->m_activation_cb);
    }
    file_event_update_events(self, events);
}

static void
file_event_merge_injected_events_and_run_cb(RpIoBevSocketHandleImpl* self, guint32 events)
{
    NOISY_MSG_("(%p(fd %d), %u)", self, SOCKFD(self), events);
    if (self->m_injected_activation_events != 0)
    {
        events |= self->m_injected_activation_events;
        self->m_injected_activation_events = 0;
        rp_schedulable_callback_cancel(self->m_activation_cb);
    }

    self->m_cb(self->m_arg, events);
}

static void
activate_file_events_i(RpIoHandle* self, guint32 events)
{
    NOISY_MSG_("(%p(fd %d), %u)", self, SOCKFD(self), events);
    RpIoBevSocketHandleImpl* me = RP_IO_BEV_SOCKET_HANDLE_IMPL(self);
    if (me->m_initialized)
    {
        file_event_activate(RP_IO_BEV_SOCKET_HANDLE_IMPL(self), events);
    }
    else
    {
        LOGI("null file_event_");
    }
}

static void
freecb(evbev_t* bev, void* arg)
{
    NOISY_MSG_("(%p(fd %d), %p)", bev, bufferevent_getfd(bev), arg);

    if (evbuffer_get_length(bufferevent_get_output(bev)) == 0)
    {
        RpIoBevSocketHandleImpl* self = RP_IO_BEV_SOCKET_HANDLE_IMPL(arg);
        g_clear_pointer(&self->m_bev, bufferevent_free);
        g_object_unref(self);
    }
}

static void
close_i(RpIoHandle* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
    RpIoBevSocketHandleImpl* me = RP_IO_BEV_SOCKET_HANDLE_IMPL(self);
    if (me->m_bev && evbuffer_get_length(bufferevent_get_output(me->m_bev)) > 0)
    {
        NOISY_MSG_("still writing %zu bytes to fd %d",
            evbuffer_get_length(bufferevent_get_output(me->m_bev)), SOCKFD(self));
        // freecb() will be called when bytes have been written to the
        // underlying socket or ssl object.
        bufferevent_setcb(me->m_bev, NULL, freecb, NULL, self);
    }
    else
    {
        g_clear_pointer(&me->m_bev, bufferevent_free);
        g_object_unref(self);
    }
}

static inline int
sockaddr_port(struct sockaddr* address)
{
    NOISY_MSG_("(%p)", address);
    switch (address->sa_family)
    {
        case AF_INET:
            return ntohs(((struct sockaddr_in*)address)->sin_port);
        case AF_INET6:
            return ntohs(((struct sockaddr_in6*)address)->sin6_port);
        default:
            return -1;
    }
}

static inline sa_family_t
sockaddr_family(struct sockaddr* address)
{
    NOISY_MSG_("(%p)", address);
NOISY_MSG_("sa family %d(%u), port %d", address->sa_family, address->sa_family == AF_UNSPEC, sockaddr_port(address));
    return address ? address->sa_family : AF_UNSPEC;
}

static inline guint16
default_port(RpIoBevSocketHandleImpl* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
    return bufferevent_openssl_get_ssl(self->m_bev) ? 443 : 80;
}

static inline int
socket_connect_hostname(RpIoBevSocketHandleImpl* self, const char* requested_server_name)
{
    NOISY_MSG_("(%p(fd %d), %p(%s))", self, SOCKFD(self), requested_server_name, requested_server_name);
    RpAuthorityAttributes attrs = http_utility_parse_authority(requested_server_name);
    guint16 port = attrs.m_port ? attrs.m_port : default_port(self);
    g_autofree gchar* hostname = g_strndup(attrs.m_host.m_data, attrs.m_host.m_length);
    return bufferevent_socket_connect_hostname(self->m_bev, self->m_dns_base, AF_UNSPEC, hostname, port);
}

static int
connect_internal(RpIoBevSocketHandleImpl* self, struct sockaddr* address, const char* requested_server_name)
{
    NOISY_MSG_("(%p(fd %d), %p, %p(%s))",
        self, SOCKFD(self), address, requested_server_name, requested_server_name);
    return sockaddr_family(address) != AF_UNSPEC ?
            bufferevent_socket_connect(self->m_bev, address, sizeof(*address)) :
            socket_connect_hostname(self, requested_server_name);

}

static int
connect_i(RpIoHandle* self, struct sockaddr* address, const char* requested_server_name)
{
    NOISY_MSG_("(%p(fd %d), %p, %p(%s))",
        self, SOCKFD(self), address, requested_server_name, requested_server_name);
    RpIoBevSocketHandleImpl* me = RP_IO_BEV_SOCKET_HANDLE_IMPL(self);
    me->m_type = RpHandleType_Connecting;
    bufferevent_setcb(me->m_bev, NULL, NULL, eventcb, self);
    return connect_internal(me, address, requested_server_name);
}

static void
enable_file_events_i(RpIoHandle* self, guint32 events)
{
    NOISY_MSG_("(%p(fd %d), %u)", self, SOCKFD(self), events);
    RpIoBevSocketHandleImpl* me = RP_IO_BEV_SOCKET_HANDLE_IMPL(self);
    if (me->m_initialized)
    {
        file_event_set_enabled(RP_IO_BEV_SOCKET_HANDLE_IMPL(self), events);
    }
    else
    {
        LOGI("null file_event_");
    }
}

static void
activation_cb(RpSchedulableCallback* self G_GNUC_UNUSED, gpointer arg)
{
    NOISY_MSG_("(%p, %p(fd %d))", self, arg, SOCKFD(arg));
    RpIoBevSocketHandleImpl* me = RP_IO_BEV_SOCKET_HANDLE_IMPL(arg);
g_assert(me->m_injected_activation_events != 0);
    file_event_merge_injected_events_and_run_cb(me, 0);
}

static void
initialize_file_event_i(RpIoHandle* self, RpDispatcher* dispatcher, RpFileReadyCb cb, gpointer arg, guint32 events)
{
    NOISY_MSG_("(%p(fd %d), %p, %p, %p, %u)", self, SOCKFD(self), dispatcher, cb, arg, events);
    RpIoBevSocketHandleImpl* me = RP_IO_BEV_SOCKET_HANDLE_IMPL(self);
    me->m_enabled_events = events;
    me->m_dispatcher = dispatcher;
    me->m_cb = cb;
    me->m_arg = arg;

    g_clear_object(&me->m_activation_cb);
    me->m_activation_cb = rp_dispatcher_create_schedulable_callback(dispatcher, activation_cb, me);
    me->m_initialized = true;
    file_event_assign_events(me, events);
}

static const char*
interface_name_i(RpIoHandle* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
//TODO...
    return "";
}

static inline bool
is_open(RpIoBevSocketHandleImpl* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
NOISY_MSG_("returning %s for fd %d", self->m_bev && bufferevent_getfd(self->m_bev) != -1 ? "true" : "false", SOCKFD(self));
    return self->m_bev && bufferevent_getfd(self->m_bev) != -1;
}

static bool
is_open_i(RpIoHandle* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
    return is_open(RP_IO_BEV_SOCKET_HANDLE_IMPL(self));
}

static struct sockaddr*
local_address_i(RpIoHandle* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
    RpIoBevSocketHandleImpl* me = RP_IO_BEV_SOCKET_HANDLE_IMPL(self);
    socklen_t ss_len = sizeof(me->m_local_addr);
    memset(&me->m_local_addr, 0, ss_len);
    if (!is_open(me))
    {
        LOGE("socket is closed");
    }
    else if (getsockname(bufferevent_getfd(me->m_bev), (struct sockaddr*)&me->m_local_addr, &ss_len) != 0)
    {
        int err = errno;
        LOGE("getsockname() failed %d(%s) on fd %d", err, g_strerror(err), SOCKFD(self));
    }
    return (struct sockaddr*)&me->m_local_addr;
}

static struct sockaddr*
peer_address_i(RpIoHandle* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
    RpIoBevSocketHandleImpl* me = RP_IO_BEV_SOCKET_HANDLE_IMPL(self);
    socklen_t ss_len = sizeof(me->m_peer_addr);
    memset(&me->m_peer_addr, 0, ss_len);
    if (!is_open(me))
    {
        LOGE("socket is closed");
    }
    else if (getpeername(bufferevent_getfd(me->m_bev), (struct sockaddr*)&me->m_peer_addr, &ss_len) != 0)
    {
        int err = errno;
        LOGE("getsockname() failed %d(%s) on fd %d", err, g_strerror(err), SOCKFD(self));
    }
    return (struct sockaddr*)&me->m_peer_addr;
}

static void
reset_file_events_i(RpIoHandle* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
    RP_IO_BEV_SOCKET_HANDLE_IMPL(self)->m_initialized = false;
}

static inline short
disable_events_from_how(int how)
{
    switch (how)
    {
        case SHUT_RD:
            return EV_READ;
        case SHUT_WR:
            return EV_WRITE;
        default:
            return EV_READ|EV_WRITE;
    }
}

static void
shutdown_i(RpIoHandle* self, int how)
{
    NOISY_MSG_("(%p(fd %d), %d)", self, SOCKFD(self), how);
    evbev_t* bev = RP_IO_BEV_SOCKET_HANDLE_IMPL(self)->m_bev;
    shutdown(bufferevent_getfd(bev), how);
    bufferevent_disable(bev, disable_events_from_how(how));
}

static bool
was_connected_i(RpIoHandle* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
    return RP_IO_BEV_SOCKET_HANDLE_IMPL(self)->m_was_connected;
}

static int
write_i(RpIoHandle* self, evbuf_t* buffer)
{
    NOISY_MSG_("(%p(fd %d), %p(%zu))", self, SOCKFD(self), buffer, evbuf_length(buffer));
    if (!buffer)
    {
        LOGI("buffer is null on fd %d", SOCKFD(self));
        return 0;
    }
    return bufferevent_write_buffer(RP_IO_BEV_SOCKET_HANDLE_IMPL(self)->m_bev, buffer);
}

static int
read_i(RpIoHandle* self, evbuf_t* buffer)
{
    NOISY_MSG_("(%p(fd %d), %p)", self, SOCKFD(self), buffer);
    if (!buffer)
    {
        LOGI("buffer is null on fd %d", SOCKFD(self));
        return -1;
    }
    RpIoBevSocketHandleImpl* me = RP_IO_BEV_SOCKET_HANDLE_IMPL(self);
    size_t start_len = evbuffer_get_length(buffer);
    if (bufferevent_read_buffer(me->m_bev, buffer) != 0)
    {
        LOGE("read buffer error on fd %d", SOCKFD(self));
        return -1;
    }
    return (int)(evbuffer_get_length(buffer) - start_len);
}

static int
sockfd_i(RpIoHandle* self)
{
    NOISY_MSG_("(%p)", self);
    return SOCKFD(self);
}

static void
io_handle_iface_init(RpIoHandleInterface* iface)
{
    LOGD("(%p)", iface);
    iface->activate_file_events = activate_file_events_i;
    iface->close = close_i;
    iface->connect = connect_i;
    iface->enable_file_events = enable_file_events_i;
    iface->initialize_file_event = initialize_file_event_i;
    iface->interface_name = interface_name_i;
    iface->is_open = is_open_i;
    iface->local_address = local_address_i;
    iface->peer_address = peer_address_i;
    iface->reset_file_events = reset_file_events_i;
    iface->shutdown = shutdown_i;
    iface->was_connected = was_connected_i;
    iface->read = read_i;
    iface->write = write_i;
    iface->sockfd = sockfd_i;
}

static void
read_buffer_cb(evbuf_t* buffer, const struct evbuffer_cb_info* info, gpointer arg)
{
    NOISY_MSG_("(%p(%zu), %p, %p(fd %d))", buffer, evbuffer_get_length(buffer), info, arg, SOCKFD(arg));
}

static void
write_buffer_cb(evbuf_t* buffer, const struct evbuffer_cb_info* info, gpointer arg)
{
    NOISY_MSG_("(%p(%zu), %p, %p(fd %d))", buffer, evbuffer_get_length(buffer), info, arg, SOCKFD(arg));
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpIoBevSocketHandleImpl* self = RP_IO_BEV_SOCKET_HANDLE_IMPL(obj);
    g_clear_pointer(&self->m_bev, bufferevent_free);

    rp_schedulable_callback_cancel(self->m_activation_cb);
    g_clear_object(&self->m_activation_cb);

    G_OBJECT_CLASS(rp_io_bev_socket_handle_impl_parent_class)->dispose(obj);
}

static void
rp_io_bev_socket_handle_impl_class_init(RpIoBevSocketHandleImplClass* klass)
{
    NOISY_MSG_("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_io_bev_socket_handle_impl_init(RpIoBevSocketHandleImpl* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_was_connected = false;
    self->m_initialized = false;
}

static inline RpIoBevSocketHandleImpl*
constructed(RpIoBevSocketHandleImpl* self)
{
    NOISY_MSG_("(%p)", self);

    bufferevent_enable(self->m_bev, EV_READ|EV_WRITE);
    bufferevent_setcb(self->m_bev, readcb, NULL, eventcb, self);

    evbuffer_add_cb(bufferevent_get_input(self->m_bev), read_buffer_cb, self);
    evbuffer_add_cb(bufferevent_get_output(self->m_bev), write_buffer_cb, self);
    return self;
}

RpIoBevSocketHandleImpl*
rp_io_bev_socket_handle_impl_new(RpHandleType_e type, evbev_t* bev, evdns_base_t* dns_base)
{
    LOGD("(%d, %p, %p)", type, bev, dns_base);
    g_return_val_if_fail(bev != NULL, NULL);
    RpIoBevSocketHandleImpl* self = g_object_new(RP_TYPE_IO_BEV_SOCKET_HANDLE_IMPL, NULL);
    self->m_bev = bev;
    self->m_type = type;
    self->m_dns_base = dns_base;
    return constructed(self);
}
