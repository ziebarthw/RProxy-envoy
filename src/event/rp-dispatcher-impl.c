/*
 * rp-dispatcher-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_dispatch_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_dispatch_impl_NOISY)
#   define NOISY_MSG_ LOGD
#   define IF_NOISY_
#else
#   define NOISY_MSG_(x, ...)
#   define IF_NOISY_(x, ...)
#endif

#include "network/rp-default-client-conn-factory.h"
#include "thread_local/rp-thread-local-impl.h"
#include "event/rp-libevent-scheduler.h"
#include "event/rp-schedulable-cb-impl.h"
#include "event/rp-signal-impl.h"
#include "event/rp-dispatcher-impl.h"

typedef struct _pointer_node pointer_node;
struct _pointer_node {
    gpointer mem;
    GDestroyNotify destroy_cb;
};

struct _RpDispatcherImpl {
    GObject parent_instance;

    UNIQUE_PTR(gchar) m_name;
    UNIQUE_PTR(evdns_base_t) m_dns_base;
    SHARED_PTR(evthr_t) m_thr;

    UNIQUE_PTR(RpTimeSystem) m_time_system;
    RpLibeventScheduler* m_base_scheduler;
    RpScheduler* m_scheduler;
    RpSchedulableCallback* m_deferred_delete_cb;
    RpSchedulableCallback* m_deferred_destroy_cb;

    RpSchedulableCallbackPtr m_post_cb;
    GMutex m_post_lock;
    GList* m_post_callbacks;

    GArray* m_to_delete_1;
    GArray* m_to_delete_2;
    GArray** m_current_to_delete;

    GPtrArray* m_to_destroy_1;
    GPtrArray* m_to_destroy_2;
    GPtrArray** m_current_to_destroy;
    GPtrArray* m_free_nodes;

    bool m_deferred_deleting : 1;
    bool m_deferred_destroying : 1;
    bool m_shutdown_called : 1;
};

static void dispatcher_base_iface_init(RpDispatcherBaseInterface* iface);
static void dispatcher_iface_init(RpDispatcherInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpDispatcherImpl, rp_dispatcher_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_DISPATCHER_BASE, dispatcher_base_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_DISPATCHER, dispatcher_iface_init)
)

static inline pointer_node*
ensure_pointer_node(RpDispatcherImpl* self)
{
    NOISY_MSG_("(%p)", self);
    // Try to pop the head.
    pointer_node* node;
    if (self->m_free_nodes->len)
    {
        node = g_ptr_array_remove_index_fast(self->m_free_nodes, 0);
        NOISY_MSG_("pre-allocated node %p", node);
        return node;
    }
    node = g_new(struct _pointer_node, 1);
    NOISY_MSG_("allocated node %p", node);
    return node;
}

static void
internal_clear_deferred_delete_list(RpDispatcherImpl* self)
{
    NOISY_MSG_("(%p)", self);
    GArray** to_delete = self->m_current_to_delete;

    guint num_to_delete = (*to_delete)->len;
    if (self->m_deferred_deleting || !num_to_delete)
    {
        NOISY_MSG_("returning");
        return;
    }

    NOISY_MSG_("clearing deferred deletion list %u", num_to_delete);

    if (self->m_current_to_delete == &self->m_to_delete_1)
    {
        NOISY_MSG_("delete 2");
        self->m_current_to_delete = &self->m_to_delete_2;
    }
    else
    {
        NOISY_MSG_("delete 1");
        self->m_current_to_delete = &self->m_to_delete_1;
    }

    //TODO...touchWatchdog();
    self->m_deferred_deleting = true;

    for (guint i=0; i < num_to_delete; ++i)
    {
        GObject* obj = g_array_index(*to_delete, gpointer, i);
        NOISY_MSG_("%u/%u) %p", i+1, num_to_delete, obj);
        g_object_unref(obj);
    }

    g_array_set_size(*to_delete, 0);
    self->m_deferred_deleting = false;
}

static void
internal_clear_deferred_destroy_list(RpDispatcherImpl* self)
{
    NOISY_MSG_("(%p)", self);
    GPtrArray** to_destroy = self->m_current_to_destroy;

    guint num_to_destroy = (*to_destroy)->len;
    if (self->m_deferred_destroying || !num_to_destroy)
    {
        NOISY_MSG_("returning");
        return;
    }

    NOISY_MSG_("clearing deferred destroy list %u", num_to_destroy);

    if (self->m_current_to_destroy == &self->m_to_destroy_1)
    {
        NOISY_MSG_("destroy 2");
        self->m_current_to_destroy = &self->m_to_destroy_2;
    }
    else
    {
        NOISY_MSG_("destroy 1");
        self->m_current_to_destroy = &self->m_to_destroy_1;
    }

    //TODO...touchWatchdog();
    self->m_deferred_destroying = true;

    for (guint i=0; i < num_to_destroy; ++i)
    {
        pointer_node* node = g_ptr_array_index(*to_destroy, i);
        NOISY_MSG_("%u/%u) %p/%p", i+1, num_to_destroy, node, node->mem);
        node->destroy_cb(g_steal_pointer(&node->mem));
        node->destroy_cb = NULL;
        g_ptr_array_add(self->m_free_nodes, node);
    }

    g_ptr_array_set_size(*to_destroy, 0);
    self->m_deferred_destroying = false;
}

static void
clear_deferred_delete_list(RpSchedulableCallback* self G_GNUC_UNUSED, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", self, arg);
    internal_clear_deferred_delete_list(RP_DISPATCHER_IMPL(arg));
}

static void
clear_deferred_destroy_list(RpSchedulableCallback* self G_GNUC_UNUSED, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", self, arg);
    internal_clear_deferred_destroy_list(RP_DISPATCHER_IMPL(arg));
}

static RpTimer*
create_timer_internal(RpDispatcherImpl* self, RpTimerCb cb, gpointer arg)
{
    NOISY_MSG_("(%p, %p, %p)", self, cb, arg);
    return rp_scheduler_create_timer(self->m_scheduler, cb, arg, RP_DISPATCHER(self));
}

static bool
is_thread_safe_i(RpDispatcherBase* self)
{
    NOISY_MSG_("(%p)", self);
    return true;//TODO...
}

typedef struct _RpPostCbCtx RpPostCbCtx;
struct _RpPostCbCtx {
    RpPostCb cb;
    gpointer arg;
};
static inline RpPostCbCtx
rp_post_cb_ctx_ctor(RpPostCb cb, gpointer arg)
{
    RpPostCbCtx self = {
        .cb = cb,
        .arg = arg
    };
    return self;
}
static inline RpPostCbCtx*
rp_post_cb_ctx_new(RpPostCb cb, gpointer arg)
{
    RpPostCbCtx* self = g_new(RpPostCbCtx, 1);
    *self = rp_post_cb_ctx_ctor(cb, arg);
    return self;
}

static void
run_post_callbacks(RpSchedulableCallback *self G_GNUC_UNUSED, gpointer arg)
{
    NOISY_MSG_("(%p, %p)", self, arg);

    RpDispatcherImpl* me = RP_DISPATCHER_IMPL(arg);

    internal_clear_deferred_delete_list(me);
    internal_clear_deferred_destroy_list(me);

    GList* callbacks;
    {
        G_MUTEX_AUTO_LOCK(&me->m_post_lock, locker);
        callbacks = g_steal_pointer(&me->m_post_callbacks);
    }
    while (callbacks)
    {
        //TODO...touchWatchdog();
        RpPostCbCtx* ctx = callbacks->data;
        ctx->cb(ctx->arg);
        g_free(ctx);
        callbacks = g_list_remove_link(callbacks, callbacks);
    }
}

static void
post_i(RpDispatcherBase* self, RpPostCb cb, gpointer arg)
{
    NOISY_MSG_("(%p, %p, %p)", self, cb, arg);
    bool do_post = false;
    RpDispatcherImpl* me = RP_DISPATCHER_IMPL(self);
    {
        G_MUTEX_AUTO_LOCK(&me->m_post_lock, locker);
        do_post = !me->m_post_callbacks;
        me->m_post_callbacks = g_list_append(me->m_post_callbacks, rp_post_cb_ctx_new(cb, arg));
    }

    if (do_post)
    {
        NOISY_MSG_("doing post");
        rp_schedulable_callback_schedule_callback_current_iteration(me->m_post_cb);
    }
}

static void
dispatcher_base_iface_init(RpDispatcherBaseInterface* iface)
{
    LOGD("(%p)", iface);
    iface->is_thread_safe = is_thread_safe_i;
    iface->post = post_i;
}

static const char*
name_i(RpDispatcher* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_DISPATCHER_IMPL(self)->m_name;
}

static RpTimer*
create_timer_i(RpDispatcher* self, RpTimerCb cb, gpointer arg)
{
    NOISY_MSG_("(%p, %p, %p)", self, cb, arg);
    return create_timer_internal(RP_DISPATCHER_IMPL(self), cb, arg);
}

static void
deferred_delete_i(RpDispatcher* self, GObject* to_delete)
{
    NOISY_MSG_("(%p, %p)", self, to_delete);
    if (G_IS_OBJECT(to_delete))
    {
        //TODO...to_delete->deleteIsPending();
        //REVISIT - inc ref count (i.e. ::move()?)
        RpDispatcherImpl* me = RP_DISPATCHER_IMPL(self);
        GArray* current_to_delete_ = *me->m_current_to_delete;
        g_array_append_val(current_to_delete_, to_delete);
        NOISY_MSG_("item added to deferred delete list (size=%u)", current_to_delete_->len);
        if (current_to_delete_->len == 1)
        {
            NOISY_MSG_("scheduling...");
            rp_schedulable_callback_schedule_callback_current_iteration(me->m_deferred_delete_cb);
        }
    }
}

static void
clear_deferred_delete_list_i(RpDispatcher* self)
{
    NOISY_MSG_("(%p)", self);
    internal_clear_deferred_delete_list(RP_DISPATCHER_IMPL(self));
}

static RpSchedulableCallback*
create_schedulable_callback_i(RpDispatcher* self, RpSchedulableCallbackCb cb, gpointer arg)
{
    NOISY_MSG_("(%p, %p, %p)", self, cb, arg);
    return rp_callback_scheduler_create_schedulable_callback(
        RP_CALLBACK_SCHEDULER(RP_DISPATCHER_IMPL(self)->m_base_scheduler), cb, arg);
}

static void
deferred_destroy_i(RpDispatcher* self, gpointer mem, GDestroyNotify cb)
{
    NOISY_MSG_("(%p, %p, %p)", self, mem, cb);
    g_return_if_fail(cb != NULL); // |mem| can be silent "error"; |cb| can never be null.
    if (mem)
    {
        RpDispatcherImpl* me = RP_DISPATCHER_IMPL(self);
        GPtrArray* current_to_destroy_ = *me->m_current_to_destroy;
        pointer_node* node = ensure_pointer_node(me);
        node->mem = mem;
        node->destroy_cb = cb;
        g_ptr_array_add(current_to_destroy_, node);
        NOISY_MSG_("item added to deferred destroy list (size=%u)", current_to_destroy_->len);
        if (current_to_destroy_->len == 1)
        {
            NOISY_MSG_("scheduling...");
            rp_schedulable_callback_schedule_callback_current_iteration(me->m_deferred_destroy_cb);
        }
    }
}

static RpTimeSource*
time_source_i(RpDispatcher* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_TIME_SOURCE(RP_DISPATCHER_IMPL(self)->m_time_system);
}

static evthr_t*
thr_i(RpDispatcher* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_DISPATCHER_IMPL(self)->m_thr;
}

static evbase_t*
base_i(RpDispatcher* self)
{
    NOISY_MSG_("(%p)", self);
    return evthr_get_base(thr_i(self));
}

static evdns_base_t*
dns_base_i(RpDispatcher* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_DISPATCHER_IMPL(self)->m_dns_base;
}

static RpSignalEventPtr
listen_for_signal_i(RpDispatcher* self, signal_t signal_num, RpSignalCb cb, gpointer arg)
{
    NOISY_MSG_("(%p, %d, %p, %p)", self, signal_num, cb, arg);
    g_assert(rp_dispatcher_base_is_thread_safe(RP_DISPATCHER_BASE(self)));
    return rp_signal_event_impl_new(RP_DISPATCHER_IMPL(self), signal_num, cb, arg);
}

static void
run_i(RpDispatcher* self, RpRunType_e type)
{
    NOISY_MSG_("(%p, %d)", self, type);
    RpDispatcherImpl* me = RP_DISPATCHER_IMPL(self);
    run_post_callbacks(NULL, self);
    rp_libevent_scheduler_run(me->m_base_scheduler, type);
}

static void
shutdown_i(RpDispatcher* self)
{
    NOISY_MSG_("(%p)", self);
    g_assert(rp_dispatcher_base_is_thread_safe(RP_DISPATCHER_BASE(self)));
    RpDispatcherImpl* me = RP_DISPATCHER_IMPL(self);
    IF_NOISY_(guint deferred_deletables_size = (*me->m_current_to_delete)->len;)
    IF_NOISY_(guint deferred_destroyables_size = (*me->m_current_to_destroy)->len;)
    IF_NOISY_(guint post_callbacks_size;)
    {
        G_MUTEX_AUTO_LOCK(&me->m_post_lock, locker);
        IF_NOISY_(post_callbacks_size = g_list_length(me->m_post_callbacks);)
    }

    //TODO...std::list<DispatcherThreadDeletableConstPtr> local_deletables;

    g_assert(!me->m_shutdown_called);
    me->m_shutdown_called = true;
    NOISY_MSG_("%s detroyed %u thread local objects. %u thread local pointers. %u post callbacks",
        G_STRFUNC, deferred_deletables_size, deferred_destroyables_size, post_callbacks_size);
}

static void
exit_i(RpDispatcher* self)
{
    NOISY_MSG_("(%p)", self);
    rp_libevent_scheduler_loop_exit(RP_DISPATCHER_IMPL(self)->m_base_scheduler);
}

static RpNetworkClientConnectionPtr
create_client_connection_i(RpDispatcher* self, RpNetworkAddressInstanceConstSharedPtr address,
                            RpNetworkAddressInstanceConstSharedPtr source_address, RpNetworkTransportSocketPtr transport_socket)
{
    extern RpDefaultClientConnectionFactory* default_client_connection_factory;
    NOISY_MSG_("(%p, %p, %p, %p)", self, address, source_address, transport_socket);
    RpClientConnectionFactory* factory = RP_CLIENT_CONNECTION_FACTORY(default_client_connection_factory);
    return rp_client_connection_factory_create_client_connection(factory, self, address, source_address, transport_socket);
}

static void
dispatcher_iface_init(RpDispatcherInterface* iface)
{
    LOGD("(%p)", iface);
    iface->name = name_i;
    iface->create_timer = create_timer_i;
    iface->deferred_delete = deferred_delete_i;
    iface->clear_deferred_delete_list = clear_deferred_delete_list_i;
    iface->create_schedulable_callback = create_schedulable_callback_i;
    iface->deferred_destroy = deferred_destroy_i;
    iface->time_source = time_source_i;
    iface->thr = thr_i;
    iface->base = base_i;
    iface->dns_base = dns_base_i;
    iface->listen_for_signal = listen_for_signal_i;
    iface->run = run_i;
    iface->shutdown = shutdown_i;
    iface->exit = exit_i;
    iface->create_client_connection = create_client_connection_i;
}

// REVISIT - this is "hokey".(?)
static inline RpScheduler*
reinit_scheduler(RpDispatcherImpl* self)
{
    NOISY_MSG_("(%p)", self);

    RpScheduler* rval = self->m_scheduler;
    if (!self->m_time_system)
    {
        NOISY_MSG_("null");
    }
    else if (!self->m_scheduler)
    {
        NOISY_MSG_("null");
    }
    else if (G_OBJECT(self->m_time_system) != G_OBJECT(self->m_scheduler))
    {
        NOISY_MSG_("setting null");
        rval = NULL;
    }
    return rval;
}

static void
evdns_free(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    evdns_base_free((evdns_base_t*)arg, 0);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpDispatcherImpl* self = RP_DISPATCHER_IMPL(obj);

    self->m_scheduler = reinit_scheduler(self);//RIVISIT...

    g_clear_pointer(&self->m_name, g_free);
    g_clear_pointer(&self->m_dns_base, evdns_free);

    g_clear_object(&self->m_deferred_delete_cb);
    g_clear_object(&self->m_deferred_destroy_cb);
    g_clear_object(&self->m_post_cb);

    g_clear_object(&self->m_time_system);
    g_clear_object(&self->m_base_scheduler);
    g_clear_object(&self->m_scheduler);

    g_array_free(g_steal_pointer(&self->m_to_delete_1), true);
    g_array_free(g_steal_pointer(&self->m_to_delete_2), true);
    self->m_current_to_delete = NULL;

    g_ptr_array_free(g_steal_pointer(&self->m_to_destroy_1), true);
    g_ptr_array_free(g_steal_pointer(&self->m_to_destroy_2), true);
    self->m_current_to_destroy = NULL;

    while (self->m_free_nodes->len > 0)
    {
        NOISY_MSG_("%u free nodes", self->m_free_nodes->len);
        g_free(g_ptr_array_remove_index_fast(self->m_free_nodes, 0));
    }
    g_ptr_array_free(g_steal_pointer(&self->m_free_nodes), true);

    G_OBJECT_CLASS(rp_dispatcher_impl_parent_class)->dispose(obj);
}

static void
rp_dispatcher_impl_class_init(RpDispatcherImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_dispatcher_impl_init(RpDispatcherImpl* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_deferred_deleting = false;
    self->m_deferred_destroying = false;
    self->m_shutdown_called = false;
    g_mutex_init(&self->m_post_lock);
}

static inline RpDispatcherImpl*
constructed(RpDispatcherImpl* self)
{
    NOISY_MSG_("(%p)", self);

    self->m_base_scheduler = rp_libevent_scheduler_new(self->m_thr);
    self->m_scheduler = rp_time_system_create_scheduler(self->m_time_system, RP_SCHEDULER(self->m_base_scheduler), NULL);

    self->m_dns_base = evdns_base_new(evthr_get_base(self->m_thr), 1);
    g_assert(self->m_dns_base != NULL);

    self->m_deferred_delete_cb = RP_SCHEDULABLE_CALLBACK(
        rp_schedulable_callback_impl_new(rp_libevent_scheduler_base(self->m_base_scheduler), clear_deferred_delete_list, self));
    self->m_deferred_destroy_cb = RP_SCHEDULABLE_CALLBACK(
        rp_schedulable_callback_impl_new(rp_libevent_scheduler_base(self->m_base_scheduler), clear_deferred_destroy_list, self));
    self->m_post_cb = RP_SCHEDULABLE_CALLBACK(
        rp_schedulable_callback_impl_new(rp_libevent_scheduler_base(self->m_base_scheduler), run_post_callbacks, self));

    self->m_to_delete_1 = g_array_new(false, false, sizeof(GObject*));
    self->m_to_delete_2 = g_array_new(false, false, sizeof(GObject*));
    self->m_current_to_delete = &self->m_to_delete_1;

    self->m_to_destroy_1 = g_ptr_array_new();
    self->m_to_destroy_2 = g_ptr_array_new();
    self->m_current_to_destroy = &self->m_to_destroy_1;
    self->m_free_nodes = g_ptr_array_new_full(3, NULL);

    return self;
}

RpDispatcherPtr
rp_dispatcher_impl_new(const char* name, UNIQUE_PTR(RpTimeSystem) time_system, SHARED_PTR(evthr_t) thr)
{
    LOGD("(%p(%s), %p, %p)", name, name, time_system, thr);
    g_return_val_if_fail(RP_IS_TIME_SYSTEM(time_system), NULL);
    g_return_val_if_fail(thr != NULL, NULL);
    RpDispatcherImpl* self = g_object_new(RP_TYPE_DISPATCHER_IMPL, NULL);
    self->m_name = name ? g_strdup(name) : NULL;
    self->m_time_system = time_system;
    self->m_thr = thr;
    return RP_DISPATCHER(constructed(self));
}

evbase_t*
rp_dispatcher_impl_base(RpDispatcherImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_DISPATCHER_IMPL(self), NULL);
    return rp_libevent_scheduler_base(self->m_base_scheduler);
}
