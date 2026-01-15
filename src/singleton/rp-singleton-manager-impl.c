/*
 * rp-singleton-manager-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_singleton_manager_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_singleton_manager_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "singleton/rp-singleton-manager-impl.h"

struct _RpSingletonManagerImpl {
    GObject parent_instance;

    GHashTable*/*<std::string, std::week_ptr<Instance>>*/ m_singletons;
    GPtrArray*/*<InstanceSharedPtr>*/ m_pinned_singletons;
};

static void singleton_manager_iface_init(RpSingletonManagerInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpSingletonManagerImpl, rp_singleton_manager_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_SINGLETON_MANAGER, singleton_manager_iface_init)
)

static RpSingletonInstanceSharedPtr
get_i(RpSingletonManager* self, const char* name, RpSingletonFactoryCb cb, bool pinned)
{
    NOISY_MSG_("(%p, %p(%s), %p, %u)", self, name, name, cb, pinned);

    RpSingletonManagerImpl* me = RP_SINGLETON_MANAGER_IMPL(self);
    RpSingletonInstanceSharedPtr existing_singleton = g_hash_table_lookup(me->m_singletons, name);

    if (!existing_singleton)
    {
        RpSingletonInstanceSharedPtr singleton = cb();
        g_hash_table_insert(me->m_singletons, g_strdup(name), singleton);
        if (pinned && singleton)
        {
            NOISY_MSG_("pinning %p", singleton);
            g_ptr_array_add(me->m_pinned_singletons, g_object_ref(singleton));
        }
        return singleton;
    }
    else
    {
        NOISY_MSG_("found %p", existing_singleton);
        return existing_singleton;
    }
}

static void
singleton_manager_iface_init(RpSingletonManagerInterface* iface)
{
    LOGD("(%p)", iface);
    iface->get = get_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpSingletonManagerImpl* self = RP_SINGLETON_MANAGER_IMPL(obj);
    g_clear_pointer(&self->m_singletons, g_hash_table_unref);
    g_ptr_array_free(g_steal_pointer(&self->m_pinned_singletons), true);

    G_OBJECT_CLASS(rp_singleton_manager_impl_parent_class)->dispose(obj);
}

static void
rp_singleton_manager_impl_class_init(RpSingletonManagerImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_singleton_manager_impl_init(RpSingletonManagerImpl* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_singletons = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    self->m_pinned_singletons = g_ptr_array_new();
}

RpSingletonManagerImpl*
rp_singleton_manager_impl_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_SINGLETON_MANAGER_IMPL, NULL);
}
