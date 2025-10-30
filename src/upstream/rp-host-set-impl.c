/*
 * rp-host-set-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_host_set_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_host_set_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-host-set-impl.h"

static guint32 kDefaultOverProvisioningFactor = 140;

typedef struct _RpLocalityEntry RpLocalityEntry;
struct _RpLocalityEntry {
    guint32 m_index;
    double m_effective_weight;
};

static inline RpLocalityEntry
rp_locality_entry_ctor(guint32 index, double effective_weight)
{
    RpLocalityEntry self = {
        .m_index = index,
        .m_effective_weight = effective_weight
    };
    return self;
}

typedef struct _RpHostSetImplPrivate RpHostSetImplPrivate;
struct _RpHostSetImplPrivate {

    RpHostVector m_hosts;
    RpHostVector m_healthy_hosts;
    RpHostVector m_degraded_hosts;
    RpHostVector m_excluded_hosts;
    RpHostsPerLocality* m_hosts_per_locality;//empty
    RpHostsPerLocality* m_healthy_hosts_per_locality;//empty
    RpHostsPerLocality* m_degraded_hosts_per_locality;//empty
    RpHostsPerLocality* m_excluded_hosts_per_locality;//empty
    //TODO...callbackmanager...
    RpLocalityWeights m_locality_weights;
    //TODO...std::vector<std::shared_ptr<LocalityEntry>> healthy...

    guint32 m_priority;
    guint32 m_overprovisioning_factor;
    bool m_weighted_priority_health;
};

enum
{
    PROP_0, // Reserved.
    PROP_PRIORITY,
    PROP_WEIGHTED_PRIORITY_HEALTH,
    PROP_OVERPROVISIONING_FACTOR,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void host_set_iface_init(RpHostSetInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpHostSetImpl, rp_host_set_impl, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpHostSetImpl)
)

#define PRIV(obj) \
    ((RpHostSetImplPrivate*) rp_host_set_impl_get_instance_private(RP_HOST_SET_IMPL(obj)))

static RpHostVector
hosts_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_hosts;
}

static RpHostVector
healthy_hosts_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_healthy_hosts;
}

static RpHostVector
degraded_hosts_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_degraded_hosts;
}

static RpHostVector
excluded_hosts_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_excluded_hosts;
}

static RpHostsPerLocality*
hosts_per_locality_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_hosts_per_locality;
}

static RpHostsPerLocality*
healthy_hosts_per_locality_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_hosts_per_locality;
}

static RpHostsPerLocality*
degraded_hosts_per_locality_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_degraded_hosts_per_locality;
}

static RpHostsPerLocality*
excluded_hosts_per_locality_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_excluded_hosts_per_locality;
}

static RpLocalityWeights
locality_weights_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_locality_weights;
}

static guint32
priority_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_priority;
}

static guint32
overprovisioning_factor_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_overprovisioning_factor;
}

static bool
weighted_priority_health_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_weighted_priority_health;
}

static void
host_set_iface_init(RpHostSetInterface* iface)
{
    LOGD("(%p)", iface);
    iface->hosts = hosts_i;
    iface->healthy_hosts = healthy_hosts_i;
    iface->degraded_hosts = degraded_hosts_i;
    iface->excluded_hosts = excluded_hosts_i;
    iface->hosts_per_locality = hosts_per_locality_i;
    iface->healthy_hosts_per_locality = healthy_hosts_per_locality_i;
    iface->degraded_hosts_per_locality = degraded_hosts_per_locality_i;
    iface->excluded_hosts_per_locality = excluded_hosts_per_locality_i;
    iface->locality_weights = locality_weights_i;
    iface->priority = priority_i;
    iface->overprovisioning_factor = overprovisioning_factor_i;
    iface->weighted_priority_health = weighted_priority_health_i;
}

OVERRIDE void
get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec)
{
    LOGD("(%p, %u, %p, %p)", obj, prop_id, value, pspec);
    switch (prop_id)
    {
        case PROP_OVERPROVISIONING_FACTOR:
            g_value_set_uint(value, PRIV(obj)->m_overprovisioning_factor);
            break;
        case PROP_PRIORITY:
            g_value_set_uint(value, PRIV(obj)->m_priority);
            break;
        case PROP_WEIGHTED_PRIORITY_HEALTH:
            g_value_set_boolean(value, PRIV(obj)->m_weighted_priority_health);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE void
set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    LOGD("(%p, %u, %p, %p)", obj, prop_id, value, pspec);
    switch (prop_id)
    {
        case PROP_OVERPROVISIONING_FACTOR:
            PRIV(obj)->m_overprovisioning_factor = g_value_get_uint(value);
            break;
        case PROP_PRIORITY:
            PRIV(obj)->m_priority = g_value_get_uint(value);
            break;
        case PROP_WEIGHTED_PRIORITY_HEALTH:
            PRIV(obj)->m_weighted_priority_health = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

OVERRIDE GObject*
constructor(GType type, guint n_construct_properties, GObjectConstructParam *construct_properties)
{
    LOGD("(%lu, %u, %p)", type, n_construct_properties, construct_properties);

    GObject* obj = G_OBJECT_CLASS(rp_host_set_impl_parent_class)->constructor(type, n_construct_properties, construct_properties);
    NOISY_MSG_("obj %p", obj);

    RpHostSetImpl* self = RP_HOST_SET_IMPL(obj);
    RpHostSetImplPrivate* me = PRIV(self);
    me->m_hosts = g_array_new(false, true, sizeof(RpHostSetImpl));
    //TODO...

    return obj;
}

OVERRIDE void
dispose(GObject* object)
{
    LOGD("(%p)", object);
    G_OBJECT_CLASS(rp_host_set_impl_parent_class)->dispose(object);
}

OVERRIDE RpStatusCode_e
run_upate_callbacks(RpHostSetImpl* self, RpHostVector hosts_added, RpHostVector hosts_removed)
{
    NOISY_MSG_("(%p, %p, %p)", self, hosts_added, hosts_removed);
//TODO...
    return RpStatusCode_Ok;
}

static void
rp_host_set_impl_class_init(RpHostSetImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructor = constructor;
    object_class->dispose = dispose;

    klass->run_update_callbacks = run_upate_callbacks;

    obj_properties[PROP_OVERPROVISIONING_FACTOR] = g_param_spec_uint("overprovisioning-factor",
                                                    "Overprovisioning factor",
                                                    "Overprovisioning Factor",
                                                    0,
                                                    G_MAXUINT,
                                                    kDefaultOverProvisioningFactor,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_PRIORITY] = g_param_spec_uint("priority",
                                                    "Priority",
                                                    "Priority",
                                                    RpResourcePriority_Default,
                                                    RpResourcePriority_High,
                                                    RpResourcePriority_Default,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_WEIGHTED_PRIORITY_HEALTH] = g_param_spec_boolean("weighted-priority-health",
                                                    "Weighted priority health",
                                                    "Weighted Priority Health",
                                                    false,
                                                    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_host_set_impl_init(RpHostSetImpl* self G_GNUC_UNUSED)
{
    LOGD("(%p)", self);
}
