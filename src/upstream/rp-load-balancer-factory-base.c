/*
 * rp-load-balancer-factory-base.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_load_balancer_factory_base_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_load_balancer_factory_base_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-load-balancer-factory-base.h"

#define PRIV(obj) \
    ((RpTypedLoadBalancerFactoryBasePrivate*)rp_typed_load_balancer_factory_base_get_instance_private(RP_TYPED_LOAD_BALANCER_FACTORY_BASE(obj)))

typedef struct _RpTypedLoadBalancerFactoryBasePrivate RpTypedLoadBalancerFactoryBasePrivate;
struct _RpTypedLoadBalancerFactoryBasePrivate {
    const char* m_name;
};

enum
{
    PROP_0, // Reserved.
    PROP_NAME,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void untyped_factory_iface_init(RpUntypedFactoryInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpTypedLoadBalancerFactoryBase, rp_typed_load_balancer_factory_base, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpTypedLoadBalancerFactoryBase)
    G_IMPLEMENT_INTERFACE(RP_TYPE_UNTYPED_FACTORY, untyped_factory_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_TYPED_FACTORY, NULL)
    G_IMPLEMENT_INTERFACE(RP_TYPE_TYPED_LOAD_BALANCER_FACTORY, NULL)
)

static const char*
name_i(RpUntypedFactory* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_name;
}

static void
untyped_factory_iface_init(RpUntypedFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->name = name_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_NAME:
            g_value_set_static_string(value, PRIV(obj)->m_name);
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
        case PROP_NAME:
            PRIV(obj)->m_name = g_value_get_string(value);
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
    G_OBJECT_CLASS(rp_typed_load_balancer_factory_base_parent_class)->dispose(obj);
}

static void
rp_typed_load_balancer_factory_base_class_init(RpTypedLoadBalancerFactoryBaseClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    obj_properties[PROP_NAME] = g_param_spec_string("name",
                                                    "Name",
                                                    "Name",
                                                    "",
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_typed_load_balancer_factory_base_init(RpTypedLoadBalancerFactoryBase* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}
