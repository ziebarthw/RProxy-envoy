/*
 * rp-pending-resolution.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_pending_resolution_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_pending_resolution_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "network/dns_resolver/libevent/rp-dns-impl.h"

#define PRIV(obj) \
    ((RpPendingResolutionPrivate*)rp_pending_resolution_get_instance_private(RP_PENDING_RESOLUTION(obj)))

typedef struct _RpPendingResolutionPrivate RpPendingResolutionPrivate;
struct _RpPendingResolutionPrivate {

    RpDnsResolverImpl* m_parent;
    RpDispatcher* m_dispatcher;
    RpNetworkDnsResolveCb m_cb;
    gpointer m_arg;

    RpPendingResponse m_pending_response;

    char* m_dns_name;
    char* m_key;

    RpCancelReason_e m_cancel_reason;

    bool m_cancelled : 1;
    bool m_completed : 1;
    bool m_owned : 1;
};

enum
{
    PROP_0, // Reserved.
    PROP_PARENT,
    PROP_CB,
    PROP_ARG,
    PROP_DISPATCHER,
    PROP_DNS_NAME,
    PROP_KEY,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void network_active_dns_query(RpNetworkActiveDnsQueryInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpPendingResolution, rp_pending_resolution, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpPendingResolution)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_ACTIVE_DNS_QUERY, network_active_dns_query)
)

static void
cancel_i(RpNetworkActiveDnsQuery* self, RpCancelReason_e reason)
{
    NOISY_MSG_("(%p, %d)", self, reason);
    RpPendingResolutionPrivate* me = PRIV(self);
    me->m_cancelled = true;
    me->m_cancel_reason = reason;
}

static void
add_trace_i(RpNetworkActiveDnsQuery* self G_GNUC_UNUSED, guint8 trace G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p, %u)", self, trace);
}

static char*
get_traces_i(RpNetworkActiveDnsQuery* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return NULL;
}

static void
network_active_dns_query(RpNetworkActiveDnsQueryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->cancel = cancel_i;
    iface->add_trace = add_trace_i;
    iface->get_traces = get_traces_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_PARENT:
            g_value_set_object(value, PRIV(obj)->m_parent);
            break;
        case PROP_CB:
            g_value_set_pointer(value, PRIV(obj)->m_cb);
            break;
        case PROP_ARG:
            g_value_set_pointer(value, PRIV(obj)->m_arg);
            break;
        case PROP_DISPATCHER:
            g_value_set_object(value, PRIV(obj)->m_dispatcher);
            break;
        case PROP_DNS_NAME:
            g_value_set_string(value, PRIV(obj)->m_dns_name);
            break;
        case PROP_KEY:
            g_value_set_string(value, PRIV(obj)->m_key);
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
        case PROP_PARENT:
            PRIV(obj)->m_parent = g_value_get_object(value);
            break;
        case PROP_CB:
            PRIV(obj)->m_cb = g_value_get_pointer(value);
            break;
        case PROP_ARG:
            PRIV(obj)->m_arg = g_value_get_pointer(value);
            break;
        case PROP_DISPATCHER:
            PRIV(obj)->m_dispatcher = g_value_get_object(value);
            break;
        case PROP_DNS_NAME:
            PRIV(obj)->m_dns_name = g_strdup(g_value_get_string(value));
            break;
        case PROP_KEY:
            PRIV(obj)->m_key = g_strdup(g_value_get_string(value));
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

    RpPendingResolutionPrivate* me = PRIV(obj);
    g_list_free_full(g_steal_pointer(&me->m_pending_response.m_address_list), g_object_unref);
    g_list_free(g_steal_pointer(&me->m_pending_response.m_waiters));
    g_clear_pointer(&me->m_dns_name, g_free);
    g_clear_pointer(&me->m_key, g_free);

    G_OBJECT_CLASS(rp_pending_resolution_parent_class)->dispose(obj);
}

static void
rp_pending_resolution_class_init(RpPendingResolutionClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_PARENT] = g_param_spec_object("parent",
                                                        "Parent",
                                                        "Parent RpDnsResolverImpl instance",
                                                        RP_TYPE_DNS_RESOLVER_IMPL,
                                                        G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_CB] = g_param_spec_pointer("cb",
                                                    "Callback",
                                                    "On resolve complete callback",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_ARG] = g_param_spec_pointer("arg",
                                                    "Arg",
                                                    "On resolve complete callback argument",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_DISPATCHER] = g_param_spec_object("dispatcher",
                                                            "Dispatcher",
                                                            "Dispatcher Instance",
                                                            RP_TYPE_DISPATCHER,
                                                            G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_DNS_NAME] = g_param_spec_string("dns-name",
                                                        "DNS name",
                                                        "DNS name to resolve",
                                                        "",
                                                        G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_KEY] = g_param_spec_string("key",
                                                    "key",
                                                    "key",
                                                    "",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_pending_resolution_init(RpPendingResolution* self)
{
    NOISY_MSG_("(%p)", self);
    RpPendingResolutionPrivate* me = PRIV(self);
    me->m_completed = false;
    me->m_cancelled = false;
    me->m_owned = false;
    me->m_pending_response = rp_pending_response_ctor(RpDnsResolutionStatus_FAILURE, NULL, NULL, "not set");
}

bool
rp_pending_resolution_completed_(RpPendingResolution* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_PENDING_RESOLUTION(self), false);
    return PRIV(self)->m_completed;
}

const char*
rp_pending_resolution_dns_name_(RpPendingResolution* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_PENDING_RESOLUTION(self), "");
    return PRIV(self)->m_dns_name;
}

void
rp_pending_resolution_set_completed_(RpPendingResolution* self, bool completed)
{
    LOGD("(%p, %u)", self, completed);
    g_return_if_fail(RP_IS_PENDING_RESOLUTION(self));
    PRIV(self)->m_completed = completed;
}

RpDnsResolverImpl*
rp_pending_resolution_parent_(RpPendingResolution* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_PENDING_RESOLUTION(self), NULL);
    return PRIV(self)->m_parent;
}

RpPendingResponse*
rp_pending_resolution_pending_response_(RpPendingResolution* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_PENDING_RESOLUTION(self), NULL);
    return &PRIV(self)->m_pending_response;
}

const char*
rp_pending_resolution_key_(RpPendingResolution* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_PENDING_RESOLUTION(self), NULL);
    return PRIV(self)->m_key;
}

void
rp_pending_resolution_add_waiter(RpPendingResolution* self, RpPendingResolution* waiter)
{
    LOGD("(%p, %p)", self, waiter);
    g_return_if_fail(RP_IS_PENDING_RESOLUTION(self));
    g_return_if_fail(RP_IS_PENDING_RESOLUTION(waiter));
    RpPendingResponse* pending_response = &PRIV(self)->m_pending_response;
    pending_response->m_waiters = g_list_prepend(pending_response->m_waiters, waiter);
}

static void
trigger_callback_cb(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    RpPendingResolution* self = arg;
    RpPendingResolutionPrivate* me = PRIV(self);
    RpPendingResponse* pending_response = &me->m_pending_response;

    me->m_cb(pending_response->m_status,
                pending_response->m_details/*TODO...std::move(pending_response_.details)*/,
                g_steal_pointer(&pending_response->m_address_list),
                me->m_arg);
}

void
rp_pending_resolution_finish_resolve(RpPendingResolution* self)
{
    LOGD("(%p)", self);

    g_return_if_fail(RP_IS_PENDING_RESOLUTION(self));

    RpPendingResolutionPrivate* me = PRIV(self);
    RpDnsResolverImpl* parent_ = me->m_parent;
    char* key = g_steal_pointer(&me->m_key);
    if (!me->m_cancelled) // REVISIT: may not work post refactor.(?)
    {
        RpPendingResponse* pending_response = &me->m_pending_response;
        RpDnsResolutionStatus_e status = pending_response->m_status;
        GList* waiters = g_list_reverse(g_steal_pointer(&pending_response->m_waiters));
        GList* address_list = g_steal_pointer(&pending_response->m_address_list);
        char* details = g_steal_pointer(&pending_response->m_details);

        for (GList* itr = waiters; itr; itr = itr->next)
        {
            RpPendingResolutionPrivate* me_ = PRIV(itr->data);
            me_->m_pending_response.m_status = status;
            me_->m_pending_response.m_details = details ? g_strdup(details) : NULL;
            me_->m_pending_response.m_address_list = address_list ? g_list_copy(address_list) : NULL;
            for (GList* itr_ = address_list; itr_; itr_ = itr_->next)
            {
                NOISY_MSG_("referencing %p", itr_->data);
                g_object_ref(itr_->data);
            }
            rp_dispatcher_base_post(RP_DISPATCHER_BASE(me_->m_dispatcher), trigger_callback_cb, itr->data);
        }

        if (details) g_free(g_steal_pointer(&details));
        if (address_list) g_list_free_full(g_steal_pointer(&address_list), g_object_unref);
        g_list_free(g_steal_pointer(&waiters));
    }
    else
    {
        LOGD("evdns_dns_callback_cancelled");//TODO...expand to be more informative.
        if (me->m_owned)
        {
            NOISY_MSG_("clearing object");
            g_clear_object(&self);
        }
    }

    GHashTable* inflight = rp_dns_resolver_impl_inflight_(parent_);
    g_hash_table_remove(inflight, key);
    g_free(key);
}

void
rp_pending_resolution_set_owned(RpPendingResolution* self, bool owned)
{
    LOGD("(%p, %u)", self, owned);
    g_return_if_fail(RP_PENDING_RESOLUTION(self));
    g_object_ref(self);
    PRIV(self)->m_owned = owned;
}
