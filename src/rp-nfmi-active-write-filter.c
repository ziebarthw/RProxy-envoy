/*
 * rp-nfmi-active-write-filter.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#if (defined(rp_nfmi_active_write_filter_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_nfmi_active_write_filter_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-fixed-write-buffer-source.h"
#include "rp-net-filter-mgr-impl.h"
#include "rp-nfmi-active-write-filter.h"

struct _RpNfmiActiveWriteFilter {
    GObject rp_nfmi_instance;

    RpNetworkFilterManagerImpl* m_parent;
    RpNetworkWriteFilter* m_filter;
    GSList* m_entry;
};

enum
{
    PROP_0, // Reserved.
    PROP_PARENT,
    PROP_FILTER,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void network_filter_callbacks_iface_init(RpNetworkFilterCallbacksInterface* iface);
static void network_write_filter_callbacks_iface_init(RpNetworkWriteFilterCallbacksInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpNfmiActiveWriteFilter, rp_nfmi_active_write_filter, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_FILTER_CALLBACKS, network_filter_callbacks_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_WRITE_FILTER_CALLBACKS, network_write_filter_callbacks_iface_init)
)

extern RpNetworkConnection* rp_nfmi_get_connection(RpNetworkFilterManagerImpl*);
extern RpSocket* rp_nfmi_get_socket(RpNetworkFilterManagerImpl*);
extern void rp_nfmi_on_resume_writing(RpNetworkFilterManagerImpl*, RpNfmiActiveWriteFilter*, RpWriteBufferSource*);

static RpNetworkConnection*
connection_i(RpNetworkFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_nfmi_get_connection(RP_NFMI_ACTIVE_WRITE_FILTER(self)->m_parent);
}

static RpSocket*
socket_i(RpNetworkFilterCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_nfmi_get_socket(RP_NFMI_ACTIVE_WRITE_FILTER(self)->m_parent);
}

static void
network_filter_callbacks_iface_init(RpNetworkFilterCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->connection = connection_i;
    iface->socket = socket_i;
}

static void
inject_write_data_to_filter_chain_i(RpNetworkWriteFilterCallbacks* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    g_autoptr(RpFixedWriteBufferSource) buffer_source = rp_fixed_write_buffer_source_new(data, end_stream);
    RpNfmiActiveWriteFilter* me = RP_NFMI_ACTIVE_WRITE_FILTER(self);
    rp_nfmi_on_resume_writing(me->m_parent, me, RP_WRITE_BUFFER_SOURCE(buffer_source));
}

static void
network_write_filter_callbacks_iface_init(RpNetworkWriteFilterCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->inject_write_data_to_filter_chain = inject_write_data_to_filter_chain_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_PARENT:
            g_value_set_object(value, RP_NFMI_ACTIVE_WRITE_FILTER(obj)->m_parent);
            break;
        case PROP_FILTER:
            g_value_set_object(value, RP_NFMI_ACTIVE_WRITE_FILTER(obj)->m_filter);
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
            RP_NFMI_ACTIVE_WRITE_FILTER(obj)->m_parent = g_value_get_object(value);
            break;
        case PROP_FILTER:
            RP_NFMI_ACTIVE_WRITE_FILTER(obj)->m_filter = g_value_get_object(value);
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
    G_OBJECT_CLASS(rp_nfmi_active_write_filter_parent_class)->dispose(obj);
}

static void
rp_nfmi_active_write_filter_class_init(RpNfmiActiveWriteFilterClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_PARENT] = g_param_spec_object("parent",
                                                    "Parent",
                                                    "Parent Instance",
                                                    RP_TYPE_NETWORK_FILTER_MANAGER_IMPL,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_FILTER] = g_param_spec_object("filter",
                                                    "Filter",
                                                    "Filter Instance",
                                                    RP_TYPE_NETWORK_WRITE_FILTER,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_nfmi_active_write_filter_init(RpNfmiActiveWriteFilter* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpNfmiActiveWriteFilter*
rp_nfmi_active_write_filter_new(RpNetworkFilterManagerImpl* parent, RpNetworkWriteFilter* filter)
{
    LOGD("(%p, %p)", parent, filter);
    g_return_val_if_fail(RP_IS_NETWORK_FILTER_MANAGER_IMPL(parent), NULL);
    g_return_val_if_fail(RP_IS_NETWORK_WRITE_FILTER(filter), NULL);
    return g_object_new(RP_TYPE_NFMI_ACTIVE_WRITE_FILTER,
                        "parent", parent,
                        "filter", filter,
                        NULL);
}

GSList*
rp_nfmi_active_write_filter_get_entry(RpNfmiActiveWriteFilter* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_NFMI_ACTIVE_WRITE_FILTER(self), NULL);
    return self->m_entry;
}

void
rp_nfmi_active_write_filter_set_entry(RpNfmiActiveWriteFilter* self, GSList* entry)
{
    LOGD("(%p, %p)", self, entry);
    g_return_if_fail(RP_IS_NFMI_ACTIVE_WRITE_FILTER(self));
    self->m_entry = entry;
}

RpNetworkWriteFilter*
rp_nfmi_active_write_filter_get_filter(RpNfmiActiveWriteFilter* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_NFMI_ACTIVE_WRITE_FILTER(self), NULL);
    return self->m_filter;
}
