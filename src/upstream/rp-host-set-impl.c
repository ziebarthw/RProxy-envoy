/*
 * rp-host-set-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_host_set_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_host_set_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "upstream/rp-upstream-impl.h"

const guint32 kDefaultOverProvisioningFactor = 140;

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

    RpHostVector* m_hosts;
    RpHostVector* m_healthy_hosts;
    RpHostVector* m_degraded_hosts;
    RpHostVector* m_excluded_hosts;
    RpHostsPerLocality* m_hosts_per_locality;//empty
    RpHostsPerLocality* m_healthy_hosts_per_locality;//empty
    RpHostsPerLocality* m_degraded_hosts_per_locality;//empty
    RpHostsPerLocality* m_excluded_hosts_per_locality;//empty
    //TODO...callbackmanager...
    RpCallbackManager* m_member_update_cb_helper;
    RpLocalityWeightsPtr m_locality_weights;
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

G_DEFINE_TYPE_WITH_CODE(RpHostSetImpl, rp_host_set_impl, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpHostSetImpl)
    G_IMPLEMENT_INTERFACE(RP_TYPE_HOST_SET, host_set_iface_init)
)

#define PRIV(obj) \
    ((RpHostSetImplPrivate*) rp_host_set_impl_get_instance_private(RP_HOST_SET_IMPL(obj)))

static const RpHostVector*
get_hosts_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_hosts;
}

static const RpHostVector*
get_healthy_hosts_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_healthy_hosts;
}

static const RpHostVector*
get_degraded_hosts_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_degraded_hosts;
}

static const RpHostVector*
get_excluded_hosts_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_excluded_hosts;
}

static RpHostsPerLocality*
get_hosts_per_locality_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_hosts_per_locality;
}

static RpHostsPerLocality*
get_healthy_hosts_per_locality_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_hosts_per_locality;
}

static RpHostsPerLocality*
get_degraded_hosts_per_locality_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_degraded_hosts_per_locality;
}

static RpHostsPerLocality*
get_excluded_hosts_per_locality_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_excluded_hosts_per_locality;
}

static RpLocalityWeightsPtr
get_locality_weights_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_locality_weights;
}

static guint32
get_priority_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_priority;
}

static guint32
get_overprovisioning_factor_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_overprovisioning_factor;
}

static bool
get_weighted_priority_health_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_weighted_priority_health;
}

static RpHostVector*
ref_hosts_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    RpHostSetImplPrivate* me = PRIV(self);
    return me->m_hosts ? rp_host_vector_ref(me->m_hosts) : NULL;
}

static RpHostVector*
ref_healthy_hosts_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    RpHostSetImplPrivate* me = PRIV(self);
    return me->m_healthy_hosts ? rp_host_vector_ref(me->m_healthy_hosts) : NULL;
}

static RpHostVector*
ref_degraded_hosts_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    RpHostSetImplPrivate* me = PRIV(self);
    return me->m_degraded_hosts ? rp_host_vector_ref(me->m_degraded_hosts) : NULL;
}

static RpHostVector*
ref_excluded_hosts_i(RpHostSet* self)
{
    NOISY_MSG_("(%p)", self);
    RpHostSetImplPrivate* me = PRIV(self);
    return me->m_excluded_hosts ? rp_host_vector_ref(me->m_excluded_hosts) : NULL;
}

static void
host_set_iface_init(RpHostSetInterface* iface)
{
    LOGD("(%p)", iface);
    iface->get_hosts = get_hosts_i;
    iface->ref_hosts = ref_hosts_i;
    iface->get_healthy_hosts = get_healthy_hosts_i;
    iface->get_degraded_hosts = get_degraded_hosts_i;
    iface->get_excluded_hosts = get_excluded_hosts_i;
    iface->get_hosts_per_locality = get_hosts_per_locality_i;
    iface->get_healthy_hosts_per_locality = get_healthy_hosts_per_locality_i;
    iface->get_degraded_hosts_per_locality = get_degraded_hosts_per_locality_i;
    iface->get_excluded_hosts_per_locality = get_excluded_hosts_per_locality_i;
    iface->get_locality_weights = get_locality_weights_i;
    iface->get_priority = get_priority_i;
    iface->get_overprovisioning_factor = get_overprovisioning_factor_i;
    iface->get_weighted_priority_health = get_weighted_priority_health_i;
    iface->ref_healthy_hosts = ref_healthy_hosts_i;
    iface->ref_degraded_hosts = ref_degraded_hosts_i;
    iface->ref_excluded_hosts = ref_excluded_hosts_i;
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

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpHostSetImplPrivate* me = PRIV(obj);

    g_clear_pointer(&me->m_hosts, rp_host_vector_unref);
    g_clear_pointer(&me->m_healthy_hosts, rp_host_vector_unref);
    g_clear_pointer(&me->m_degraded_hosts, rp_host_vector_unref);
    g_clear_pointer(&me->m_excluded_hosts, rp_host_vector_unref);

    g_clear_object(&me->m_member_update_cb_helper);

    G_OBJECT_CLASS(rp_host_set_impl_parent_class)->dispose(obj);
}

OVERRIDE RpStatusCode_e
run_update_callbacks(RpHostSetImpl* self, const RpHostVector* hosts_added, const RpHostVector* hosts_removed)
{
    NOISY_MSG_("(%p, %p, %p)", self, hosts_added, hosts_removed);
    RpHostSetImplPrivate* me = PRIV(self);
    GList* callbacks = rp_callback_manager_callbacks(me->m_member_update_cb_helper);
    for (GList* itr = callbacks; itr; itr = itr->next)
    {
        RpCallbackHandlePtr handle = itr->data;
        RpPrioritySetPriorityUpdateCb cb = (RpPrioritySetPriorityUpdateCb)rp_callback_handle_cb(handle);
        gpointer arg = rp_callback_handle_arg(handle);
        RpStatusCode_e status = cb(me->m_priority, hosts_added, hosts_removed, arg);
        if (status != RpStatusCode_Ok)
        {
            LOGE("failed");
            return status;
        }
    }
    return RpStatusCode_Ok;
}

static void
rp_host_set_impl_class_init(RpHostSetImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    klass->run_update_callbacks = run_update_callbacks;

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
rp_host_set_impl_init(RpHostSetImpl* self)
{
    LOGD("(%p)", self);
    RpHostSetImplPrivate* me = PRIV(self);
    me->m_hosts = rp_host_vector_new();
    me->m_healthy_hosts = rp_host_vector_new();
    me->m_degraded_hosts = rp_host_vector_new();
    me->m_excluded_hosts = rp_host_vector_new();
    me->m_member_update_cb_helper = rp_callback_manager_new();
}

RpCallbackHandlePtr
rp_host_set_impl_add_priority_update_cb(RpHostSetImpl* self, RpPrioritySetPriorityUpdateCb cb, gpointer arg)
{
    LOGD("(%p, %p, %p)", self, cb, arg);
    g_return_val_if_fail(RP_IS_HOST_SET_IMPL(self), NULL);
    return rp_callback_manager_add(PRIV(self)->m_member_update_cb_helper, (RpCallback)cb, arg);
}

static inline RpHostVector*
steal_and_ref_hosts(RpHostVector** dst, RpHostVector** src)
{
    NOISY_MSG_("(%p, %p)", dst, src);
    g_clear_pointer(dst, rp_host_vector_unref);
    RpHostVector* hosts = g_steal_pointer(src);
    return hosts ? rp_host_vector_ref(hosts) : NULL;
}

void
rp_host_set_impl_update_hosts(RpHostSetImpl* self, RpPrioritySetUpdateHostsParams* params /* transfer full */,
                                const RpHostVector* hosts_added, const RpHostVector* hosts_removed,
                                bool* weighted_priority_health_opt, guint32* overprovisioning_factor_opt)
{
    LOGD("(%p, %p, %p, %p, %p, %p)",
        self, params, hosts_added, hosts_removed, weighted_priority_health_opt, overprovisioning_factor_opt);

    g_return_if_fail(RP_IS_HOST_SET_IMPL(self));
    g_return_if_fail(params != NULL);

    RpHostSetImplPrivate* me = PRIV(self);

    //TODO...if (weighted_priority_health.has_value)
    //TODO...if (overprovisioning_factor)

    me->m_hosts = steal_and_ref_hosts(&me->m_hosts, &params->hosts);
    me->m_healthy_hosts = steal_and_ref_hosts(&me->m_healthy_hosts, &params->healthy_hosts);
    me->m_degraded_hosts = steal_and_ref_hosts(&me->m_degraded_hosts, &params->degraded_hosts);
    me->m_excluded_hosts = steal_and_ref_hosts(&me->m_excluded_hosts, &params->excluded_hosts);

    //TODO...

    RpStatusCode_e status = rp_host_set_impl_run_update_callbacks(self, hosts_added, hosts_removed);
    if (status != RpStatusCode_Ok)
    {
        LOGE("update callbacks failed");
    }
}

RpPrioritySetUpdateHostsParams
rp_host_set_impl_partition_hosts_take(RpHostVector** hosts /* Transfer full. */)
{
    LOGD("(%p)", hosts);

    RpPrioritySetUpdateHostsParams params = {0};
    g_return_val_if_fail(hosts && *hosts, params);

    RpPartitionedHostListTuple partitioned_hosts =
        rp_cluster_impl_base_partition_host_list(*hosts);

    params.hosts = g_steal_pointer(hosts);
    params.healthy_hosts = partitioned_hosts.healthy_list;
    params.degraded_hosts = partitioned_hosts.degraded_list;
    params.excluded_hosts = partitioned_hosts.excluded_list;

    return params;
}

RpPrioritySetUpdateHostsParams
rp_host_set_impl_update_hosts_params(RpHostVector* hosts, RpHostVector* healthy_hosts,
                                        RpHostVector* degraded_hosts, RpHostVector* excluded_hosts)
{
    LOGD("(%p, %p, %p, %p)", hosts, healthy_hosts, degraded_hosts, excluded_hosts);
    return (RpPrioritySetUpdateHostsParams) {
        .hosts = g_steal_pointer(&hosts),
        .healthy_hosts = g_steal_pointer(&healthy_hosts),
        .degraded_hosts = g_steal_pointer(&degraded_hosts),
        .excluded_hosts = g_steal_pointer(&excluded_hosts)
    };
}

RpPrioritySetUpdateHostsParams
rp_host_set_impl_update_hosts_params_2(RpHostSetImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HOST_SET_IMPL(self), (RpPrioritySetUpdateHostsParams){0});
    return rp_host_set_impl_update_hosts_params(rp_host_set_ref_hosts(RP_HOST_SET(self)),
                                                (RpHostVector*)rp_host_set_get_healthy_hosts(RP_HOST_SET(self)),
                                                (RpHostVector*)rp_host_set_get_degraded_hosts(RP_HOST_SET(self)),
                                                (RpHostVector*)rp_host_set_get_excluded_hosts(RP_HOST_SET(self)));
}
