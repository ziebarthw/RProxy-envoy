/*
 * rp-pending-stream.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_pending_stream_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_pending_stream_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-upstream.h"
#include "rp-pending-stream.h"

typedef struct _RpPendingStreamPrivate RpPendingStreamPrivate;
struct _RpPendingStreamPrivate {
    RpConnPoolImplBase* m_parent;
    bool m_can_send_early_data;
};

enum
{
    PROP_0, // Reserved.
    PROP_PARENT,
    PROP_CAN_SEND_EARLY_DATA,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void cancellable_iface_init(RpCancellableInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpPendingStream, rp_pending_stream, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpPendingStream)
    G_IMPLEMENT_INTERFACE(RP_TYPE_CANCELLABLE, cancellable_iface_init)
)

#define PRIV(obj) \
    ((RpPendingStreamPrivate*) rp_pending_stream_get_instance_private(RP_PENDING_STREAM(obj)))

static void
cancel_i(RpCancellable* self, RpCancelPolicy_e policy)
{
    NOISY_MSG_("(%p, %d)", self, policy);
    rp_conn_pool_impl_base_on_pending_stream_cancel(PRIV(self)->m_parent, RP_PENDING_STREAM(self), policy);
}

static void
cancellable_iface_init(RpCancellableInterface* iface)
{
    LOGD("(%p)", iface);
    iface->cancel = cancel_i;
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
        case PROP_CAN_SEND_EARLY_DATA:
            g_value_set_boolean(value, PRIV(obj)->m_can_send_early_data);
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
        case PROP_CAN_SEND_EARLY_DATA:
            PRIV(obj)->m_can_send_early_data = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_pending_stream_parent_class)->constructed(obj);

    RpPendingStreamPrivate* me = PRIV(obj);
    //TODO...
    RpHostPtr host = rp_conn_pool_impl_base_host(me->m_parent);
    RpClusterInfoConstSharedPtr cluster_info = rp_host_description_cluster(RP_HOST_DESCRIPTION(host));
    RpResourcePriority_e priority = rp_conn_pool_impl_base_priority(me->m_parent);
    RpResourceManager* resource_manager = rp_cluster_info_resource_manager(cluster_info, priority);
    RpResourceLimit* pending_requests = rp_resource_manager_pending_requests(resource_manager);

    rp_resource_limit_inc(pending_requests);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_pending_stream_parent_class)->dispose(obj);
}

static void
rp_pending_stream_class_init(RpPendingStreamClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    obj_properties[PROP_PARENT] = g_param_spec_object("parent",
                                                    "Parent",
                                                    "Parent ConnPoolImplBase Instance",
                                                    RP_TYPE_CONN_POOL_IMPL_BASE,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_CAN_SEND_EARLY_DATA] = g_param_spec_boolean("can-send-early-data",
                                                    "Can send early data",
                                                    "Can Send Early Data Flag",
                                                    true,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_pending_stream_init(RpPendingStream* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

bool
rp_pending_stream_can_send_early_data_(RpPendingStream* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_can_send_early_data;
}
