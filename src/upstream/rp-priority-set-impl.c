/*
 * rp-priority-set-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "macrologger.h"

#if (defined(rp_priority_set_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_priority_set_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-host-map-snap.h"
#include "rp-host-set-ptr-vector.h"
#include "common/common/rp-callback-impl.h"
#include "upstream/rp-upstream-impl.h"

typedef struct _RpPrioritySetImplPrivate RpPrioritySetImplPrivate;
struct _RpPrioritySetImplPrivate {

    RpHostSetPtrVector* /* std::vector<HostSetPtr> */ m_host_sets;
    RpHostMapSnap* m_const_cross_priority_host_map; // published frozen snapshot; never null
    RpPrioritySetImpl* m_parent;
    RpCallbackManager* /* <const HostVector&, const HostVector&> */ m_member_update_cb_helper;
    RpCallbackManager* /* <uint32_t, const HostVector&, const HostVector&> */ m_priority_update_cb_helper;
    GPtrArray* m_host_sets_priority_update_cbs;
    bool m_batch_update;
};

static void priority_set_iface_init(RpPrioritySetInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpPrioritySetImpl, rp_priority_set_impl, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpPrioritySetImpl)
    G_IMPLEMENT_INTERFACE(RP_TYPE_PRIORITY_SET, priority_set_iface_init)
)

#define PRIV(obj) \
    ((RpPrioritySetImplPrivate*) rp_priority_set_impl_get_instance_private(RP_PRIORITY_SET_IMPL(obj)))

static inline GPtrArray*
ensure_host_sets_priority_update_cbs(RpPrioritySetImpl* self)
{
    NOISY_MSG_("(%p)", self);
    RpPrioritySetImplPrivate* me = PRIV(self);
    if (me->m_host_sets_priority_update_cbs)
    {
        NOISY_MSG_("pre-allocated host sets priority update cbs %p", me->m_host_sets_priority_update_cbs);
        return me->m_host_sets_priority_update_cbs;
    }
    me->m_host_sets_priority_update_cbs = g_ptr_array_new_with_free_func(g_object_unref);
    NOISY_MSG_("allocated host sets priority update cbs %p", me->m_host_sets_priority_update_cbs);
    return me->m_host_sets_priority_update_cbs;
}

static const RpHostSetPtrVector*
host_sets_per_priority_i(RpPrioritySet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_host_sets;
}

static RpHostMapSnap*
cross_priority_host_map_i(RpPrioritySet* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_const_cross_priority_host_map;
}

static void
update_hosts_i(RpPrioritySet* self, guint32 priority, RpPrioritySetUpdateHostsParams* params /* transfer full */,
                const RpHostVector* hosts_added, const RpHostVector* hosts_removed, bool* weighted_priority_health_opt, guint32* overprovisioning_factor_opt,
                RpHostMapSnapSharedPtr cross_priority_host_map)
{
    NOISY_MSG_("(%p, %u, %p, %p, %p, %p, %p, %p)",
        self, priority, params, hosts_added, hosts_removed, weighted_priority_health_opt, overprovisioning_factor_opt, cross_priority_host_map);
    RpPrioritySetImplPrivate* me = PRIV(self);
    if (cross_priority_host_map)
    {
        // Atomically set our const cross priority host map to the user-supplied
        // |cross_priority_host_map. rp_host_map_snap_ataomic_set_ref() will
        // ensure ownership rules are followed.
        rp_host_map_snap_atomic_set_ref(&me->m_const_cross_priority_host_map, cross_priority_host_map);
    }
    rp_priority_set_impl_get_or_create_host_set(RP_PRIORITY_SET_IMPL(self), priority);
    RpHostSetImpl* host_set = RP_HOST_SET_IMPL(rp_host_set_ptr_vector_get(me->m_host_sets, priority));
    rp_host_set_impl_update_hosts(host_set,
                                    params,
                                    hosts_added,
                                    hosts_removed,
                                    weighted_priority_health_opt,
                                    overprovisioning_factor_opt);
    if (!me->m_batch_update)
    {
        NOISY_MSG_("running callbacks");
        rp_priority_set_impl_run_update_callbacks(RP_PRIORITY_SET_IMPL(self), hosts_added, hosts_removed);
    }
}

static RpCallbackHandlePtr
add_member_update_cb_i(RpPrioritySet* self, RpPrioritySetMemberUpdateCb cb, gpointer arg)
{
    NOISY_MSG_("(%p, %p, %p)", self, cb, arg);
    return rp_callback_manager_add(PRIV(self)->m_member_update_cb_helper, (RpCallback)cb, arg);
}

static RpCallbackHandlePtr
add_priority_update_cb_i(RpPrioritySet* self, RpPrioritySetPriorityUpdateCb cb, gpointer arg)
{
    NOISY_MSG_("(%p, %p, %p)", self, cb, arg);
    return rp_callback_manager_add(PRIV(self)->m_priority_update_cb_helper, (RpCallback)cb, arg);
}

static void
priority_set_iface_init(RpPrioritySetInterface* iface)
{
    LOGD("(%p)", iface);
    iface->host_sets_per_priority = host_sets_per_priority_i;
    iface->cross_priority_host_map = cross_priority_host_map_i;
    iface->update_hosts = update_hosts_i;
    iface->add_member_update_cb = add_member_update_cb_i;
    iface->add_priority_update_cb = add_priority_update_cb_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpPrioritySetImplPrivate* me = PRIV(obj);
    g_clear_pointer(&me->m_host_sets, rp_host_set_ptr_vector_unref);
    g_clear_pointer(&me->m_const_cross_priority_host_map, rp_host_map_snap_unref);

#if 0
RpHostMapSnap* prev =
  g_atomic_pointer_exchange((gpointer*)&me->m_const_cross_priority_host_map, NULL);
if (prev) rp_host_map_snap_unref(prev);
#endif//0

    g_clear_pointer(&me->m_host_sets_priority_update_cbs, g_ptr_array_unref);
    g_clear_object(&me->m_member_update_cb_helper);
    g_clear_object(&me->m_priority_update_cb_helper);

    G_OBJECT_CLASS(rp_priority_set_impl_parent_class)->dispose(obj);
}

OVERRIDE RpHostSetImplPtr
create_host_set(RpPrioritySetImpl* self, guint32 priority)
{
    NOISY_MSG_("(%p, %u)", self, priority);
    return g_object_new(RP_TYPE_HOST_SET_IMPL, "priority", priority, NULL);
}

OVERRIDE RpStatusCode_e
run_update_callbacks(RpPrioritySetImpl* self, const RpHostVector* hosts_added, const RpHostVector* hosts_removed)
{
    NOISY_MSG_("(%p, %p, %p)", self, hosts_added, hosts_removed);
    RpPrioritySetImplPrivate* me = PRIV(self);
    GList* callbacks = rp_callback_manager_callbacks(me->m_member_update_cb_helper);
    for (GList* itr = callbacks; itr; itr = itr->next)
    {
        RpCallbackHandlePtr handle = itr->data;
        RpPrioritySetMemberUpdateCb cb = (RpPrioritySetMemberUpdateCb)rp_callback_handle_cb(handle);
        gpointer arg = rp_callback_handle_arg(handle);
        RpStatusCode_e status = cb(hosts_added, hosts_removed, arg);
        if (status != RpStatusCode_Ok)
        {
            LOGE("failed");
            return status;
        }
    }
    return RpStatusCode_Ok;
}

OVERRIDE RpStatusCode_e
run_reference_update_callbacks(RpPrioritySetImpl* self, guint32 priority, const RpHostVector* hosts_added, const RpHostVector* hosts_removed)
{
    NOISY_MSG_("(%p, %u, %p, %p)", self, priority, hosts_added, hosts_removed);
    RpPrioritySetImplPrivate* me = PRIV(self);
    GList* callbacks = rp_callback_manager_callbacks(me->m_priority_update_cb_helper);
    for (GList* itr = callbacks; itr; itr = itr->next)
    {
        RpCallbackHandlePtr handle = itr->data;
        RpPrioritySetPriorityUpdateCb cb = (RpPrioritySetPriorityUpdateCb)rp_callback_handle_cb(handle);
        gpointer arg = rp_callback_handle_arg(handle);
        RpStatusCode_e status = cb(priority, hosts_added, hosts_removed, arg);
        if (status != RpStatusCode_Ok)
        {
            LOGE("failed");
            return status;
        }
    }
    return RpStatusCode_Ok;
}

static void
rp_priority_set_impl_class_init(RpPrioritySetImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;

    klass->create_host_set = create_host_set;
    klass->run_update_callbacks = run_update_callbacks;
    klass->run_reference_update_callbacks = run_reference_update_callbacks;
}

static void
rp_priority_set_impl_init(RpPrioritySetImpl* self)
{
    LOGD("(%p)", self);
    RpPrioritySetImplPrivate* me = PRIV(self);
    me->m_host_sets = rp_host_set_ptr_vector_new();
    me->m_const_cross_priority_host_map = rp_host_map_snap_new_empty_frozen();
    me->m_member_update_cb_helper = rp_callback_manager_new();
    me->m_priority_update_cb_helper = rp_callback_manager_new();
    me->m_batch_update = false;
}

RpPrioritySetImpl*
rp_priority_set_impl_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_PRIORITY_SET_IMPL, NULL);
}

static RpStatusCode_e
run_reference_update_callbacks_cb(guint32 priority, const RpHostVector* hosts_added, const RpHostVector* hosts_removed, gpointer arg)
{
    NOISY_MSG_("(%u, %p, %p, %p)", priority, hosts_added, hosts_removed, arg);
    RpPrioritySetImpl* self = arg;
    return rp_priority_set_impl_run_reference_update_callbacks(self, priority, hosts_added, hosts_removed);
}

const RpHostSet*
rp_priority_set_impl_get_or_create_host_set(RpPrioritySetImpl* self, guint32 priority)
{
    LOGD("(%p, %u)", self, priority);
    g_return_val_if_fail(RP_IS_PRIORITY_SET_IMPL(self), NULL);
    RpPrioritySetImplPrivate* me = PRIV(self);
    if (rp_host_set_ptr_vector_size(me->m_host_sets) < priority + 1)
    {
        for (guint i = rp_host_set_ptr_vector_size(me->m_host_sets); i <= priority; ++i)
        {
            RpHostSetImplPtr host_set = rp_priority_set_impl_create_host_set(self, i);
NOISY_MSG_("%p, adding new host set %p to host sets %p", self, host_set, me->m_host_sets);
            g_ptr_array_add(ensure_host_sets_priority_update_cbs(self),
                g_object_ref(
                    rp_host_set_impl_add_priority_update_cb(host_set, run_reference_update_callbacks_cb, self))
            );
            rp_host_set_ptr_vector_set(me->m_host_sets, i, (RpHostSetPtr)g_steal_pointer(&host_set));
        }
    }
    return rp_host_set_ptr_vector_get(me->m_host_sets, priority);
}

RpHostMapSnap**
rp_priority_set_impl_const_cross_priority_host_map_(RpPrioritySetImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_PRIORITY_SET_IMPL(self), NULL);
    return &PRIV(self)->m_const_cross_priority_host_map;
}
