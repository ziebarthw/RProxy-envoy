/*
 * rp-basic-resource-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_basic_resource_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_basic_resource_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include <stdio.h>
#include "rproxy.h"
#include "common/common/rp-basic-resource-impl.h"

typedef struct _RpBasicResourceLimitImplPrivate RpBasicResourceLimitImplPrivate;
struct _RpBasicResourceLimitImplPrivate {

    char* m_runtime_key;
    guint64 m_max;
    guint64 m_current;
};

enum
{
    PROP_0, // Reserved.
    PROP_MAX,
    PROP_RUNTIME_KEY,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void resource_limit_iface_init(RpResourceLimitInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpBasicResourceLimitImpl, rp_basic_resource_limit_impl, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpBasicResourceLimitImpl)
    G_IMPLEMENT_INTERFACE(RP_TYPE_RESOURCE_LIMIT, resource_limit_iface_init)
)

#define PRIV(obj) \
    ((RpBasicResourceLimitImplPrivate*) rp_basic_resource_limit_impl_get_instance_private(RP_BASIC_RESOURCE_LIMIT_IMPL(obj)))

static bool
can_create_i(RpResourceLimit* self)
{
    NOISY_MSG_("(%p)", self);
    RpBasicResourceLimitImplPrivate* me = PRIV(self);
    return me->m_current < me->m_max;
}

static void
inc_i(RpResourceLimit* self)
{
    NOISY_MSG_("(%p)", self);
    ++PRIV(self)->m_current;
}

static void
dec_i(RpResourceLimit* self)
{
    NOISY_MSG_("(%p)", self);
    rp_resource_limit_dec_by(self, 1);
}

static void
dec_by_i(RpResourceLimit* self, guint64 amount)
{
    NOISY_MSG_("(%p, %zu)", self, amount);
    //TODO...ASSERT(current_ >= amount)
    PRIV(self)->m_current -= amount;
}

static guint64
max_i(RpResourceLimit* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_max;
}

static guint64
count_i(RpResourceLimit* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_current;
}

static void
resource_limit_iface_init(RpResourceLimitInterface* iface)
{
    LOGD("(%p)", iface);
    iface->can_create = can_create_i;
    iface->inc = inc_i;
    iface->dec = dec_i;
    iface->dec_by = dec_by_i;
    iface->max = max_i;
    iface->count = count_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_MAX:
            g_value_set_uint64(value, PRIV(obj)->m_max);
            break;
        case PROP_RUNTIME_KEY:
            g_value_set_string(value, PRIV(obj)->m_runtime_key);
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
        case PROP_MAX:
            PRIV(obj)->m_max = g_value_get_uint64(value);
            break;
        case PROP_RUNTIME_KEY:
            g_clear_pointer(&PRIV(obj)->m_runtime_key, g_free);
            PRIV(obj)->m_runtime_key = g_strdup(g_value_get_string(value));
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

    RpBasicResourceLimitImplPrivate* me = PRIV(obj);
    g_clear_pointer(&me->m_runtime_key, g_free);

    G_OBJECT_CLASS(rp_basic_resource_limit_impl_parent_class)->dispose(obj);
}

OVERRIDE void
finalize(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_basic_resource_limit_impl_parent_class)->finalize(obj);
}

static void
rp_basic_resource_limit_impl_class_init(RpBasicResourceLimitImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;
    object_class->finalize = finalize;

    obj_properties[PROP_MAX] = g_param_spec_uint64("max",
                                                    "Max",
                                                    "Max",
                                                    0,
                                                    G_MAXUINT64,
                                                    0,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_RUNTIME_KEY] = g_param_spec_string("runtime-key",
                                                    "Runtime key",
                                                    "Runtime Key",
                                                    "",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_basic_resource_limit_impl_init(RpBasicResourceLimitImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

void
rp_basic_resource_limit_impl_set_max(RpBasicResourceLimitImpl* self, guint64 new_max)
{
    LOGD("(%p, %zu)", self, new_max);
    g_return_if_fail(RP_IS_BASIC_RESOURCE_LIMIT_IMPL(self));
    PRIV(self)->m_max = new_max;
}
