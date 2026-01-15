/*
 * rp-net-conn-impl-base.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_net_conn_impl_base_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_net_conn_impl_base_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rproxy.h"
#include "rp-net-conn-impl-base.h"

#define SOCKFD(s) rp_network_connection_sockfd(RP_NETWORK_CONNECTION(s))

typedef struct _RpNetworkConnectionImplBasePrivate RpNetworkConnectionImplBasePrivate;
struct _RpNetworkConnectionImplBasePrivate {

    RpDelayedCloseState_e m_delayed_close_state;
    //TODO...Event::TimePtr delayed_close_timer_;
    //TODO...milliseconds delayed_close_timeout_;
    RpDispatcher* m_dispatcher;
    const char* m_local_close_reason;
    guint64 m_id;
    GSList* m_callbacks;
    //TODO..ConnectionStats connection_stats_;

};

enum
{
    PROP_0, // Reserved.
    PROP_ID,
    PROP_DISPATCHER,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void network_connection_iface_init(RpNetworkConnectionInterface* iface);
static void filter_manager_connection_iface_init(RpFilterManagerConnectionInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpNetworkConnectionImplBase, rp_network_connection_impl_base, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpNetworkConnectionImplBase)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_CONNECTION, network_connection_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_FILTER_MANAGER_CONNECTION, filter_manager_connection_iface_init)
)

#define PRIV(obj) \
    ((RpNetworkConnectionImplBasePrivate*) rp_network_connection_impl_base_get_instance_private(RP_NETWORK_CONNECTION_IMPL_BASE(obj)))

static void
raw_write_i(RpFilterManagerConnection* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, evbuf_length(data), end_stream);
}

static void
filter_manager_connection_iface_init(RpFilterManagerConnectionInterface* iface)
{
    LOGD("(%p)", iface);
    iface->raw_write = raw_write_i;
}

static void
add_connection_callbacks_i(RpNetworkConnection* self, RpNetworkConnectionCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    RpNetworkConnectionImplBasePrivate* me = PRIV(self);
    me->m_callbacks = g_slist_append(me->m_callbacks, callbacks);
}

static void
remove_connection_callbacks_i(RpNetworkConnection* self, RpNetworkConnectionCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    RpNetworkConnectionImplBasePrivate* me = PRIV(self);
    for (GSList* entry = me->m_callbacks; entry; entry = entry->next)
    {
        if (entry->data && RP_NETWORK_CONNECTION_CALLBACKS(entry->data) == callbacks)
        {
            entry->data = NULL;
            break;
        }
    }
}

static RpDispatcher*
dispatcher_i(RpNetworkConnection* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_dispatcher;
}

static const char*
local_close_reason_i(RpNetworkConnection* self)
{
    NOISY_MSG_("(%p)", self);
return ""; //TODO...
}

static guint64
id_i(RpNetworkConnection* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_id;
}

static void
network_connection_iface_init(RpNetworkConnectionInterface* iface)
{
    LOGD("(%p)", iface);
    iface->dispatcher = dispatcher_i;
    iface->add_connection_callbacks = add_connection_callbacks_i;
    iface->remove_connection_callbacks = remove_connection_callbacks_i;
    iface->local_close_reason = local_close_reason_i;
    iface->id = id_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_ID:
            g_value_set_uint64(value, PRIV(obj)->m_id);
            break;
        case PROP_DISPATCHER:
            g_value_set_pointer(value, PRIV(obj)->m_dispatcher);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_ID:
            PRIV(obj)->m_id = g_value_get_uint64(value);
            break;
        case PROP_DISPATCHER:
            PRIV(obj)->m_dispatcher = g_value_get_pointer(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_network_connection_impl_base_parent_class)->dispose(obj);
}

static void
rp_network_connection_impl_base_class_init(RpNetworkConnectionImplBaseClass* klass)
{
    LOGD("(%p)", klass);
LOGD("iface %p", g_type_interface_peek(klass, RP_TYPE_NETWORK_CONNECTION));
//network_connection_iface_init(g_type_interface_peek(klass, RP_TYPE_NETWORK_CONNECTION));

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_ID] = g_param_spec_uint64("id",
                                                    "id",
                                                    "Unique connection id",
                                                    0,
                                                    G_MAXUINT64,
                                                    0,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_DISPATCHER] = g_param_spec_pointer("dispatcher",
                                                    "Dispatcher",
                                                    "Event Dispatcher Instance (evhtp)",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_network_connection_impl_base_init(RpNetworkConnectionImplBase* self)
{
    NOISY_MSG_("(%p)", self);

    RpNetworkConnectionImplBasePrivate* me = PRIV(self);
    me->m_delayed_close_state = RpDelayedCloseState_None;
    me->m_local_close_reason = NULL;
}

bool
rp_network_connection_impl_base_in_delayed_close(RpNetworkConnectionImplBase* self)
{
    NOISY_MSG_("(%p(fd %d))", self, SOCKFD(self));
    g_return_val_if_fail(RP_IS_NETWORK_CONNECTION_IMPL_BASE(self), false);
    return PRIV(self)->m_delayed_close_state != RpDelayedCloseState_None;
}

void
rp_network_connection_impl_base_raise_connection_event(RpNetworkConnectionImplBase* self, RpNetworkConnectionEvent_e event)
{
    LOGD("(%p(fd %d), %d)", self, SOCKFD(self), event);
    g_return_if_fail(RP_IS_NETWORK_CONNECTION_IMPL_BASE(self));
    RpNetworkConnectionImplBasePrivate* me = PRIV(self);
    for (GSList* entry = me->m_callbacks; entry; entry = entry->next)
    {
        NOISY_MSG_("entry %p(%p)", entry, entry->data);
        if (event != RpNetworkConnectionEvent_LocalClose &&
            event != RpNetworkConnectionEvent_RemoteClose &&
            rp_network_connection_state(RP_NETWORK_CONNECTION(self)) != RpNetworkConnectionState_Open)
        {
            NOISY_MSG_("returning");
            return;
        }

        if (entry->data)
        {
            RpNetworkConnectionCallbacks* callback = RP_NETWORK_CONNECTION_CALLBACKS(entry->data);
            NOISY_MSG_("calling rp_network_connection_callbacks_on_event(%p, %d)", callback, event);
            rp_network_connection_callbacks_on_event(callback, event);
        }
    }
}

void
rp_network_connection_impl_base_set_local_close_reason(RpNetworkConnectionImplBase* self, const char* local_close_reason)
{
    LOGD("(%p, %p(%s))", self, local_close_reason, local_close_reason);
    g_return_if_fail(RP_IS_NETWORK_CONNECTION_IMPL_BASE(self));
    PRIV(self)->m_local_close_reason = local_close_reason;
}

RpDispatcher*
rp_network_connection_impl_base_dispatcher_(RpNetworkConnectionImplBase* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_dispatcher;
}
